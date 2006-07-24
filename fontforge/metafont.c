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
#include "edgelist.h"
#include <gwidget.h>
#include <ustring.h>
#include <math.h>
#include <gkeysym.h>

static int counterwarned = false;

/* This module is designed to detect certain features of a character (like stems
 *  and counters, and then modify them in a useful way.
 * The most obvious examples are:
 *	To make all the stems thicker (thereby emboldening the font)
 *	To make all the counters narrower (thereby condensing the font)
 *	To change the xheight (thereby changing the feel of the font)
 *
 * So first we find all the stems (and I mean all, not just the horizontal/
 *  vertical stems). From this list we then look at just the hv stems and
 *  find the hv counters, (if a glyph has few vertical stems (like "k" or "w")
 *  we also look at the diagonals. Pretend a diagonal expresses a counter
 *  half its expected size) based on this we figure how to position the
 *  centers of all stems.
 *	(we build a map based on vertical stems and h counters. We figure
 *	 how each stem and counter should expand, and based on that where
 *	 each should end up. We then extrapolate intermediate values.
 *	We do the same for the horizontal stems, except in most cases they
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
 * Any points which do not lie on stems get interpolated.
 * We now need to figure out the control points.
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

/* ************ Data structures active during the entire command ************ */
enum counterchoices { cc_same,	/* counters have the same width until scaled */
	cc_centerfixed,		/* stem centers remain fixed, stems expand around, then scale */
	cc_edgefixed,		/* outer edges are fixed, outer stems only expand, then scale */
				/*  inward, while inner stems have their centers */
			        /*  fixed */
	cc_zones		/* specify a mapping directly */
};

struct scale {
    real factor;
    real add;
};

struct zonemap {
    real from;
    real to;
};

struct stemcntl {
    unsigned int onlyh: 1;	/* this entry only controls hstems */
    unsigned int onlyv: 1;
    real small, wide;		/* Only controls stems whose width is between small and wide (inclusive) */
    real factor;
    real add;
};

typedef struct metafont {
    unsigned int done: 1;
    unsigned int scalewidth: 1;		/* If false then scale l/rbearing */
    struct scale width;			/* if scalewidth scale width by this */
    struct scale lbearing, rbearing;	/* if !scalewidth, do these */
    struct counters {
	unsigned int counterchoices: 2;
	struct scale counter;		/* if counterchoices==cc_scale */
	int zonecnt;
	struct zonemap *zones;
	real widthmin;		/* No counter may drop below this */
			/* Nor will a counter smaller than this be detected */
    } counters[2];			/* 0 is horizontal, 1 is vertical */
    int stemcnt;
    struct stemcntl *stems;
    real blues[6];			/* Contains the result of QuickBlues */
    int bcnt, bxh;			/* number of entries and location of xh*/
    FontView *fv;
    CharView *cv;
    SplineChar *sc;
    SplineFont *sf;
    GWindow gw;
} MetaFontDlg;

/* ************ Data structures active on a per character basis ************* */

typedef struct splineinfo {
    Spline *spline1, *spline2;		/* spline1 will be leftmost (bottommost) */
    SplinePoint *from, *to;		/* Using the direction of spline1 */
    		/* Does spline1 or spline2 end first? so from will be either */
		/*  spline1->from, or spline2->to, and to either spline1->to */
		/*  or spline2->from */
    real fromlen, tolen;		/* The distance from spline1 to spline2*/
		/* (or spline2 to spline1 depending on whether from belongs */
		/*  to spline1 or spline2), perpendicular to the point there */
		/* if the len is -1 the we've decided that this doesn't */
		/*  qualify as a stem, probably because the two edges are */
		/*  nothing like parallel */
    BasePoint fromvec, tovec;		/* Vector to the point mid-way between the two splines */
    		/* splines */
    unsigned int spline2backwards: 1;
    unsigned int figured: 1;
    unsigned int freepasscnt: 1;
    unsigned int major: 1;		/* Direction in which found */
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
    real bottom, top;		/* Range in the other coordinate over which this map is valid */
    int scnt, counternumber;
    StemInfo **stems;
    struct countergroup *next;
};

struct map {
    real bottom, top;		/* Range in the other coordinate over which this map is valid */
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
    real rbearing;		/* Only used if !meta->scalewidth */
    real width;
} SCI;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */


void SCCopyFgToBg(SplineChar *sc, int show) {
    SplinePointList *fore, *end;

    SCPreserveBackground(sc);
    fore = SplinePointListCopy(sc->layers[ly_fore].splines);
    if ( fore!=NULL ) {
	SplinePointListsFree(sc->layers[ly_back].splines);
	sc->layers[ly_back].splines = NULL;
	for ( end = fore; end->next!=NULL; end = end->next );
	end->next = sc->layers[ly_back].splines;
	sc->layers[ly_back].splines = fore;
	if ( show )
	    SCCharChangedUpdate(sc);
    }
}

#ifdef FONTFORGE_CONFIG_COPY_BG_TO_FG
void SCCopyBgToFg(SplineChar *sc, int show) {
    SplinePointList *back, *end;

    SCPreserveState(sc, true);
    back = SplinePointListCopy(sc->layers[ly_back].splines);
    if ( back!=NULL ) {
	SplinePointListsFree(sc->layers[ly_fore].splines);
	sc->layers[ly_fore].splines = NULL;
	for ( end = back; end->next!=NULL; end = end->next );
	end->next = sc->layers[ly_fore].splines;
	sc->layers[ly_fore].splines = back;
	if ( show )
	    SCCharChangedUpdate(sc);
    }
}
#endif /* FONTFORGE_CONFIG_COPY_BG_TO_FG */
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI

static SCI *SCIinit(SplineChar *sc,MetaFontDlg *meta) {
    /* Does five or six things:
	Sets the undoes
	Calculates hints (if we need to)
	Copies the forground to the background
	Closes any open paths (remove any single points)
	numbers the points
	creates the pointinfo list
    */
    SplinePointList *ss, *ssnext, *prev;
    SplinePoint *sp;
    int cnt;
    SCI *sci;
    static struct simplifyinfo smpl = { sf_cleanup };

    SCPreserveState(sc,true);

    SplinePointListSimplify(sc,sc->layers[ly_fore].splines,&smpl);		/* Get rid of two points at the same location, they cause us problems */
    if ( sc->manualhints || sc->changedsincelasthinted )
	SplineCharAutoHint(sc,NULL);

    SCCopyFgToBg(sc,false);

    for ( prev=NULL, ss=sc->layers[ly_fore].splines; ss!=NULL; ss = ssnext ) {
	ssnext = ss->next;
	if ( ss->first->next==NULL ) {	/* Single point */
	    if ( prev==NULL )
		sc->layers[ly_fore].splines = ssnext;
	    else
		prev->next = ssnext;
	    SplinePointListFree(ss);
	} else
	    prev = ss;
    }
    for ( ss = sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	if ( ss->first->prev==NULL ) {
	    /* It's not a loop, Close it! */
	    SplineMake3( ss->last,ss->first );
	    ss->last = ss->first;
	}
    }

    for ( ss = sc->layers[ly_fore].splines, cnt=0; ss!=NULL; ss=ss->next ) {
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
    for ( ss = sc->layers[ly_fore].splines, cnt=0; ss!=NULL; ss=ss->next ) {
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

static SplineInfo *SCIAddStem(SCI *sci,Spline *spline1,Spline *spline2,int major) {
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
    cur->major = major;
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
	    if ( EISkipExtremum(apt,i+el->low,major)) {
		e = apt->aenext;
	continue;
	    }
	    if ( !apt->hv && apt->aenext!=NULL && apt->aenext->hv &&
		    EISameLine(apt,apt->aenext,i+el->low,major))
		apt = apt->aenext;
	    e = p = EIActiveEdgesFindStem(apt, i+el->low, major);
	    if ( e==NULL )
	break;
	    SCIAddStem(sci,apt->spline,e->spline,major);
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

static int SCIReasonableConnections(SCI *sci) {
    int i;

    for ( i=0; i<sci->ptcnt; ++i )
	if ( sci->pts[i].next==NULL )
return( false );

return( true );
}

/* Generate a line through spline1[t1] perpendicular to it at that point */
/*  Intersect the line with spline2. If the intersection is on the spline */
/*  segment, then fill in the len and vec values (vec is the vector from the spline to the midpoint between the two)*/
static int FindPerpDistance(real t1,Spline *spline1, Spline *spline2,
	BasePoint *vec, real *len) {
    BasePoint pt, slope, pslope, end[3], ss2;
    bigreal x, y, slope1, slope2;
    extended ts[3], lens[3], angle;
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
	    if ( lens[2]>0 && lens[2]<lens[j] )
		j = 2;
    }

    vec->x = (pt.x+end[j].x)/2 - pt.x;
    vec->y = (pt.y+end[j].y)/2 - pt.y;
    if ( lens[j]<.001 ) lens[j]=0;
    else {
	real len = rint(lens[j]);
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
    int v;

#if 0
 printf( "\nspline1=(%g,%g) -> (%g,%g), spline2=(%g,%g) -> (%g,%g)\n",
     si->spline1->from->me.x, si->spline1->from->me.y, si->spline1->to->me.x, si->spline1->to->me.y,
     si->spline2->from->me.x, si->spline2->from->me.y, si->spline2->to->me.x, si->spline2->to->me.y );
#endif
    if ( si->spline1->from->me.x==410 && si->spline2->from->me.x==408 )
 si->spline2->from->me.x=408;

    si->fromlen = si->tolen = -1;
    if (( foundfrom = (FindPerpDistance(0,si->spline1,si->spline2,&si->fromvec,&si->fromlen)>0) ))
	si->from = si->spline1->from;
    if (( foundto = (FindPerpDistance(1,si->spline1,si->spline2,&si->tovec,&si->tolen)>0) ))
	si->to = si->spline1->to;
    if ( !si->major ) {
	up1 = si->spline1->from->me.x<si->spline1->to->me.x;
	up2 = si->spline2->from->me.x<si->spline2->to->me.x;
    } else {
	up1 = si->spline1->from->me.y<si->spline1->to->me.y;
	up2 = si->spline2->from->me.y<si->spline2->to->me.y;
    }
    si->spline2backwards = (up1!=up2);
    if ( true || !foundfrom || !foundto ) {			/* !!! Debug */
	if ( up1==up2 ) {
	    if ( !foundfrom ) {
		if ( FindPerpDistance(0,si->spline2,si->spline1,&si->fromvec,&si->fromlen)>0 )
		    si->from = si->spline2->from;
	    }
	    if ( !foundto ) {
		if ( FindPerpDistance(1,si->spline2,si->spline1,&si->tovec,&si->tolen)>0 )
		    si->to = si->spline2->to;
	    }
#if 0
 printf( "(%g,%g) <-> (%g,%g) mid=(%g,%g) len=%g\n",
si->spline1->from->me.x, si->spline1->from->me.y,
si->spline2->from->me.x, si->spline2->from->me.y,
si->fromvec.x, si->fromvec.y, si->fromlen);
 printf( "(%g,%g) <-> (%g,%g) mid=(%g,%g) len=%g\n",
si->spline1->to->me.x, si->spline1->to->me.y,
si->spline2->to->me.x, si->spline2->to->me.y,
si->tovec.x, si->tovec.y, si->tolen);
#endif
	} else {
	    if ( !foundfrom ) {
		if ( FindPerpDistance(1,si->spline2,si->spline1,&si->fromvec,&si->fromlen) )
		    si->from = si->spline2->to;
	    }
	    if ( !foundto ) {
		if ( FindPerpDistance(0,si->spline2,si->spline1,&si->tovec,&si->tolen) )
		    si->to = si->spline2->from;
	    }
#if 0
 printf( "(%g,%g) <-> (%g,%g) mid=(%g,%g) len=%g\n",
si->spline1->from->me.x, si->spline1->from->me.y,
si->spline2->to->me.x, si->spline2->to->me.y,
si->fromvec.x, si->fromvec.y, si->fromlen);
 printf( "(%g,%g) <-> (%g,%g) mid=(%g,%g) len=%g\n",
si->spline1->to->me.x, si->spline1->to->me.y,
si->spline2->from->me.x, si->spline2->from->me.y,
si->tovec.x, si->tovec.y, si->tolen);
#endif
	}
	if ( si->from!=NULL && si->from != si->spline1->from ) {
	    si->fromvec.x = -si->fromvec.x;
	    si->fromvec.y = -si->fromvec.y;
	}
	if ( si->to!=NULL && si->to != si->spline1->to ) {
	    si->tovec.x = -si->tovec.x;
	    si->tovec.y = -si->tovec.y;
	}
    }
    si->figured = true;

/* If it's close to an integer, round it to the integer */
    v = rint(si->fromlen); if ( si->fromlen>v-.05 && si->fromlen<v+.05 ) si->fromlen = v;
    v = rint(si->tolen); if ( si->tolen>v-.05 && si->tolen<v+.05 ) si->tolen = v;
    v = rint(si->fromvec.x); if ( si->fromvec.x>v-.05 && si->fromvec.x<v+.05 ) si->fromvec.x = v;
    v = rint(si->fromvec.y); if ( si->fromvec.y>v-.05 && si->fromvec.y<v+.05 ) si->fromvec.y = v;
    v = rint(si->tovec.x); if ( si->tovec.x>v-.05 && si->tovec.x<v+.05 ) si->tovec.x = v;
    v = rint(si->tovec.y); if ( si->tovec.y>v-.05 && si->tovec.y<v+.05 ) si->tovec.y = v;

    if ( (si->fromlen>0 && si->tolen>=0 ) || (si->fromlen>=0 && si->tolen>0))
	si->spline1->touched = si->spline2->touched = true;
    else
	si->fromlen = si->tolen = -1;
}

/* We weren't able to find a matching edge for this spline (to make a stem) */
/*  by doing a horizontal or vertical traverse of the glyph. Let's try to */
/*  traverse perpendicular to the spline itself */
static void SplineFindOtherEdge(SCI *sci,Spline *spline) {
    real angle, s, c, xval, yval;
    BasePoint pt, mid;
    Spline *right, *left, *test, *first;
    SplineSet *ss;
    real rlen, llen, len;
    int bcnt;
    int bad = false, lbad = false, rbad = false;

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
    for ( ss = sci->sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	first = NULL;
	for ( test = ss->first->next; test!=first; test = test->to->next ) {
	  if ( first==NULL ) first = test;
	  if ( test!=spline ) {
	    real f,t,c1,c2;
	    f = -test->from->me.x*s + test->from->me.y*c;
	    c1 = -test->from->nextcp.x*s + test->from->nextcp.y*c;
	    t = -test->to->me.x*s + test->to->me.y*c;
	    c2 = -test->to->prevcp.x*s + test->to->prevcp.y*c;
	    if ( (f<yval-.001 && t<yval-.001 && c1<yval-.001 && c2<yval-.001 ) ||
		    (f>yval+.001 && t>yval+.001 && c1>yval+.001 && c2>yval+.001 ) )
	continue;		/* Can't intersect us */
	    if ( FindPerpDistance(.5,spline,test,&mid,&len)) {
		if ( (bad = len<0) ) len = -len;
		if ( mid.x*c + mid.y*s < xval ) {
		    /* If we intersect a spline right at its endpoint then */
		    /*  we can't add the full amount because we'll find another*/
		    /*  spline later (or earlier) that intersects at the */
		    /*  same place. So in that case we add half the normal */
		    /*  amount */
		    if ( RealNear(-test->from->me.x*s - test->from->me.y*c,yval) ||
			    RealNear(-test->to->me.x*s - test->to->me.y*c,yval) )
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
	}
    }

    if ( bcnt&1 )
	IError( "Odd number of Point intersections in SplineFindOtherEdge" );
    if ( bcnt&2 ) {
	/* Odd number of splines left */
	right = left;
	rlen = llen;
	rbad = lbad;
    }
    if ( !rbad && right!=NULL ) {
	SplineInfo *si = SCIAddStem(sci,(bcnt&2)?right:spline,(bcnt&2)?spline:right,2);
	if ( si!=NULL && !si->figured )
	    SIFigureWidth(si);
    }
    spline->touched = true;		/* Even if we didn't find anything */
}

static int NotParallelHere(SplineInfo *si,SplinePoint *sp) {
    Spline *s1, *s2;
    real t1, len;
    BasePoint mid;

    if ( si->spline1->from==sp ) {
	s1 = si->spline1; s2 = si->spline2;
	t1 = 0;
    } else if ( si->spline1->to==sp ) {
	s1 = si->spline1; s2 = si->spline2;
	t1 = 1;
    } else if ( si->spline2->from==sp ) {
	s1 = si->spline2; s2 = si->spline1;
	t1 = 0;
    } else if ( si->spline2->to==sp ) {
	s1 = si->spline2; s2 = si->spline1;
	t1 = 1;
    } else
return( true );		/* It's not parallel. Actually we've no idea what's going on, this shouldn't happen */
return( FindPerpDistance(t1,s1,s2,&mid,&len)<=0 );
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

static real MetaFontFindWidth(MetaFontDlg *meta,real width,int isvert) {
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

static int MetaRecognizedStemWidth(MetaFontDlg *meta,real width,int isvert) {
    int i;

    /* First look for matches where isvert matches */
    if ( width==0 )
return( false );

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

static void MapFromCounterGroup(struct map *map,MetaFontDlg *meta,
	struct countergroup *cg, struct zonemap *extra, int ecnt, int isvert,
	char *charname ) {
    real offset=0;
    int i=0,j,k;
    StemInfo *stem;
    real newwidth, lastwidth, counterwidth, newcwidth;

    map->bottom = cg->bottom;
    map->top = cg->top;

    map->cnt = ecnt+2*cg->scnt+ (meta->counters[isvert].counterchoices==cc_zones?meta->counters[isvert].zonecnt:0);
    map->mapping = galloc(map->cnt*sizeof(struct zonemap));

    if ( ecnt!=0 ) {
	memcpy(map->mapping,extra,ecnt*sizeof(struct zonemap));
	i = ecnt;
	offset = extra[ecnt-1].to - extra[ecnt-1].from;
    }

    if ( meta->counters[isvert].counterchoices==cc_zones ) {
	struct zonemap *zones = meta->counters[isvert].zones;
	int cnt = meta->counters[isvert].zonecnt;
	real mid, newmid;

	for ( j=k=0; k<cnt; ++k ) {
	    for ( ; j<cg->scnt ; ++j ) {
		stem = cg->stems[j];
		if ( stem->start+stem->width>=zones[k].from )
	    break;
		mid = stem->start+stem->width/2;
		if ( k==0 ) {
		    newmid = mid + (zones[0].to-zones[0].from);
		} else {
		    newmid = zones[k-1].to + (mid-zones[k-1].from)*
			     (zones[k].to-zones[k-1].to)/(zones[k].from-zones[k-1].from);
		}
		newwidth = MetaFontFindWidth(meta,stem->width,isvert);
		if ( newwidth==0 ) {
		    map->mapping[i].from = mid;
		    map->mapping[i++].to = newmid;
		} else if ( stem->width==20 && stem->ghost ) {
		    map->mapping[i].from = stem->start+20;
		    map->mapping[i++].to = rint(newmid+10);
		} else if ( stem->width==21 && stem->ghost ) {
		    map->mapping[i].from = stem->start;
		    map->mapping[i++].to = rint(newmid-10.5);
		} else {
		    map->mapping[i].from = stem->start;
		    map->mapping[i++].to = rint(newmid-newwidth/2);
		    map->mapping[i].from = stem->start+stem->width;
		    map->mapping[i].to = map->mapping[i-1].to+newwidth;
		    ++i;
		}
	    }
	    if ( j<cg->scnt && stem->start<=zones[k].from ) {
		newwidth = MetaFontFindWidth(meta,stem->width,isvert);
		if ( newwidth==0 || stem->ghost )
		    map->mapping[i++] = zones[k];
		else if ( stem->start==zones[k].from ) {
		    map->mapping[i++] = zones[k];
		    map->mapping[i].from = stem->start+stem->width;
		    map->mapping[i++].to = zones[k].to+newwidth;
		} else if ( stem->start+stem->width==zones[k].from ) {
		    map->mapping[i].from = stem->start;
		    map->mapping[i++].to = zones[k].to-newwidth;
		    map->mapping[i++] = zones[k];
		} else if ( stem->start<zones[k].from && stem->start+stem->width>zones[k].from ) {
		    map->mapping[i].from = stem->start;
		    map->mapping[i++].to = zones[k].to-(newwidth*(zones[k].from-stem->start)/stem->width);
		    map->mapping[i++] = zones[k];
		    map->mapping[i].from = stem->start+stem->width;
		    map->mapping[i].to = map->mapping[i-2].to+newwidth;
		    ++i;
		} else
		    map->mapping[i++] = zones[k];
		++j;
	    } else
		map->mapping[i++] = zones[k];
	}
	map->cnt = i;
    } else {
	newwidth = 0;		/* Irrelevant, but it means compiler doesn't think newwidth uninitialized */
	for ( j=0; j<cg->scnt; ++j ) {
	    lastwidth = newwidth;
	    newwidth = MetaFontFindWidth(meta,cg->stems[j]->width,isvert);
	    if ( newwidth==0 )
	continue;
	    if ( j!=0 ) {
		newcwidth = counterwidth =
			cg->stems[j]->start - (cg->stems[j-1]->start+cg->stems[j-1]->width);
		switch ( meta->counters[isvert].counterchoices ) {
		  case cc_same:
		    newcwidth = counterwidth;
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
		  break;
		  case cc_zones:
		  default:
		    IError("Shouldn't get here in MapFromCounterGroup" );
		  break;
		}
		newcwidth = meta->counters[isvert].counter.factor*newcwidth +
			meta->counters[isvert].counter.add;
		if ( (newcwidth<meta->counters[isvert].widthmin && counterwidth>=meta->counters[isvert].widthmin) ||
			newcwidth<0 ) {
		    if ( !counterwarned )
			gwwv_post_error(_("Counter Too Small"),_("A counter in %.30s was requested to be too small, it has been pegged at its minimum value"), charname);
		    counterwarned = true;
		    if ( counterwidth>=meta->counters[isvert].widthmin )
			newcwidth = meta->counters[isvert].widthmin;
		    else
			newcwidth = 0;
		}
		offset += newcwidth-counterwidth;
	    }
	    if ( cg->stems[j]->width==20 && cg->stems[j]->ghost ) {
		map->mapping[i].from = cg->stems[j]->start+20;
		map->mapping[i++].to = rint(cg->stems[j]->start+20+offset);
	    } else if ( cg->stems[j]->width==21 && cg->stems[j]->ghost ) {
		map->mapping[i].from = cg->stems[j]->start;
		map->mapping[i++].to = rint(cg->stems[j]->start+offset);
	    } else {
		map->mapping[i].from = cg->stems[j]->start;
		map->mapping[i++].to = rint(cg->stems[j]->start+offset);
		map->mapping[i].from = cg->stems[j]->start+cg->stems[j]->width;
		map->mapping[i].to = map->mapping[i-1].to+newwidth;
		++i;
		offset += newwidth-cg->stems[j]->width;
	    }
	}
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
    int cnt, allhi, ctest, anyconflicts, anynonconflicts, i;
    struct countergroup *cg = NULL, *test, *cur, *prev, *next;
    int cnum=0;

    anyconflicts=anynonconflicts = false;
    for ( s=stems, cnt=0; s!=NULL; s=s->next, ++cnt ) {
	for ( hi=s->where; hi!=NULL; hi=hi->next )
	    hi->counternumber = -1;
	if ( s->hasconflicts ) anyconflicts = true;
	else anynonconflicts = true;
    }

    /* if there are no conflicts, then one counter group should hold everything */
    /* If everything conflicts then pick a stem, there are no counters */
    if ( !anyconflicts ) {
	cur = gcalloc(1,sizeof(struct countergroup));
	cur->scnt = cnt;
	cur->stems = galloc(cnt*sizeof(StemInfo *));
	cur->bottom = -1e8; cur->top = 1e8;
	cur->counternumber = 0;
	for ( i=0, s=stems; i<cnt; ++i, s=s->next )
	    cur->stems[i] = s;
return( cur );
    } else if ( !anynonconflicts ) {
	StemInfo *best = stems;
	for ( s=stems->next; s!=NULL; s=s->next ) {
	    if ( s->width<best->width )
		best = s;
	}
	cur = gcalloc(1,sizeof(struct countergroup));
	cur->scnt = 1;
	cur->stems = galloc(1*sizeof(StemInfo *));
	cur->bottom = -1e8; cur->top = 1e8;
	cur->counternumber = 0;
	cur->stems[0] = best;
return( cur );
    } else if ( cnt==3 && anynonconflicts ) {
	StemInfo *non, *first=NULL, *second=NULL;
	for ( s=stems; s!=NULL; s=s->next ) {
	    if ( !s->hasconflicts || s->where!=NULL )
		non=s;
	    else if ( first==NULL )
		first = s;
	    else
		second = s;
	}
	if ( first!=NULL && second!=NULL ) {
	    if ( first->where->begin>second->where->end ) {
		StemInfo *temp = first; first = second; second = temp;
	    }
	    cg = cur = gcalloc(1,sizeof(struct countergroup));
	    cur->scnt = 2;
	    cur->stems = galloc(2*sizeof(StemInfo *));
	    cur->bottom = -1e8; cur->top = ceil(first->where->end);
	    cur->counternumber = 0;
	    if ( non->start< first->start ) {
		cur->stems[0] = non; cur->stems[1] = first;
	    } else {
		cur->stems[0] = first; cur->stems[1] = non;
	    }
	    cur = gcalloc(1,sizeof(struct countergroup));
	    cur->scnt = 2;
	    cur->stems = galloc(2*sizeof(StemInfo *));
	    cur->bottom = floor(second->where->begin); cur->top = 1e8;
	    cur->counternumber = 1;
	    if ( non->start< second->start ) {
		cur->stems[0] = non; cur->stems[1] = second;
	    } else {
		cur->stems[0] = second; cur->stems[1] = non;
	    }
	    cg->next = cur;
return( cg );
	}
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
	cur->bottom = floor(best->begin); cur->top = ceil(best->end);
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
	struct zonemap *extras, int ecnt, char *charname) {
    struct map *map = gcalloc(1,sizeof(struct map));
    real minx, maxx, minxw, maxxw, olen, orthwidth;
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
	    minxw = orthwidth;
	}
	if ( minx>dstem->leftedgebottom.x ) {
	    minx = dstem->leftedgebottom.x;
	    minxw = orthwidth;
	}
	if ( maxx<dstem->rightedgetop.x ) {
	    maxx = dstem->rightedgetop.x;
	    maxxw = orthwidth;
	}
	if ( maxx<dstem->rightedgebottom.x ) {
	    maxx = dstem->rightedgebottom.x;
	    maxxw = orthwidth;
	}
	dstem = dstem->next;
    }
    if ( vstem!=NULL ) {	/* At most one */
	if ( vstem->start+vstem->width >= minx && vstem->start < minx ) {
	    minx = vstem->start;
	    minxw = vstem->width;
	    vstem = NULL;
	} else if ( vstem->start<=maxx && vstem->start+vstem->width > maxx ) {
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
    MapFromCounterGroup(map,meta,&cg, extras,ecnt,false, charname);
return( map );
}

static void MapCleanup(struct mapd *mapd) {
    int i, j, k;
    struct map *map;

    for ( i=0; i<mapd->mapcnt; ++i ) {
	map = &mapd->maps[i];
	for ( j=0; j<map->cnt-1; ++j ) {
	    if ( map->mapping[j].from>=map->mapping[j+1].from ) {
		--map->cnt;
		for ( k=j ; k<map->cnt; ++k )
		    map->mapping[k] = map->mapping[k+1];
	    }
	}
    }
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
	    sci->width = meta->width.factor*sci->sc->width + meta->width.add;
	} else {
	    extras[0].to = meta->lbearing.factor*b.minx + meta->lbearing.add;
	    sci->rbearing = meta->rbearing.factor*(sci->sc->width-b.maxx) + meta->rbearing.add;
	}
	ecnt = 1;
    }

    if ( !isvert &&
	    (sci->sc->vstem==NULL || sci->sc->vstem->next==NULL) &&
	    sci->sc->dstem!=NULL ) {
	/* If we're looking for horizontal counters, but we've got no (or at */
	/*  most one) vertical stem, then see if we can generate any counters */
	/*  from the diagonal stems */
	sci->mapd[0].mapcnt = 1;
	sci->mapd[0].maps = MapFromDiags(meta,sci->sc->vstem,
		sci->sc->dstem,extras,1,sci->sc->name);
    } else {
	/* Horizontal counters need vertical stems, and vice versa */
	countergroups = SCIFindCounterGroups(isvert ? sci->sc->hstem : sci->sc->vstem);
	for ( i=0, cg=countergroups; cg!=NULL; ++i, cg=cg->next );
	if ( i==0 ) i=1;
	sci->mapd[isvert].mapcnt = i;
	sci->mapd[isvert].maps = gcalloc(i,sizeof(struct map));
	for ( i=0, cg=countergroups; cg!=NULL; ++i, cg=cg->next )
	    MapFromCounterGroup(&sci->mapd[isvert].maps[i],meta,cg,
		    extras,ecnt,isvert,sci->sc->name);
	CounterGroupsFree(countergroups);
    }
    MapCleanup(&sci->mapd[isvert]);
}

static real _MapCoord(struct map *map, real coord) {
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

static real MapCoord(struct mapd *map, real coord, real other) {
    int i, top=-1, bottom=-1;
    real topdiff = 1e8, bottomdiff = 1e8;

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

static int _IsOnKnownEdge(struct map *map, real coord) {
    int i;

    for ( i=0; i<map->cnt; ++i ) {
	if ( coord<map->mapping[i].from-.1 )
return( false );
	if ( coord<map->mapping[i].from+.1 )
return( true );
    }
return( false );
}

static int IsOnKnownEdge(struct mapd *map, real coord, real other) {
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

static int IsHVSpline(Spline *spline, SplinePoint *sp) {
    real rat;

    if ( !spline->knownlinear ) {
	if ( sp==spline->from ) {
	    if ( spline->splines[0].c==0 )
return( 1 );
	    else if ( spline->splines[1].c==0 )
return( 0 );
	    else if ( ( rat = spline->splines[0].c/spline->splines[1].c) < .05 &&
		    rat>-.05 )
return( 1 );
	    else if ( ( rat = spline->splines[1].c/spline->splines[0].c) < .05 &&
		    rat>-.05 )
return( 0 );
	} else if ( sp==spline->to ) {
	    real xs, ys;
	    xs = 3*spline->splines[0].a+2*spline->splines[0].b+spline->splines[0].c;
	    ys = 3*spline->splines[1].a+2*spline->splines[1].b+spline->splines[1].c;
	    if ( xs==0 )
return( 1 );
	    else if ( ys==0 )
return( 0 );
	    else if ( ( rat = xs/ys) < .05 && rat>-.05 )
return( 1 );
	    else if ( ( rat = ys/xs) < .05 && rat>-.05 )
return( 0 );
	}
return( 2 );
    } else if ( spline->from->me.x==spline->to->me.x )
return( 1 );
    else if ( spline->from->me.y==spline->to->me.y )
return( 0 );

return( 2 );
}

static real SCIFindMidPoint(SCI *sci,int pt, SplineList *spl, BasePoint *mid) {
    SplinePoint *sp = sci->pts[pt].cur;
    SplineList *sl;
    real len;
    SplineInfo *only, *exact;
    int isvert;
    Spline *spline;

    if ( spl->cur->spline1==sp->next || spl->cur->spline2==sp->next )
	isvert = IsHVSpline( spline = sp->next, sp );
    else
	isvert = IsHVSpline( spline = sp->prev, sp );

    /* First check to see if there's a Stem in the splinelist that's valid */
    len = -1; only = exact = NULL;
    for ( sl=spl; sl!=NULL; sl=sl->next ) {
	/* A curved spline may form a stem with another spline/line at one */
	/*  location, while at another location the two are not a good match */
	/*  So not all the stems we figured will be good matches at all times */
	if (( !sl->cur->spline1->knownlinear || !sl->cur->spline2->knownlinear ) &&
		NotParallelHere(sl->cur,sp))
    continue;
	if ( MetaRecognizedStemWidth(sci->meta,sl->cur->fromlen,isvert) ||
		MetaRecognizedStemWidth(sci->meta,sl->cur->tolen,isvert)) {
	    if ( len==-1 ) {
		if ( sl->cur->from==sp && MetaRecognizedStemWidth(sci->meta,sl->cur->fromlen,isvert))
		    len = sl->cur->fromlen;
		else if ( sl->cur->to==sp && MetaRecognizedStemWidth(sci->meta,sl->cur->tolen,isvert))
		    len = sl->cur->tolen;
		else
		    len = MetaRecognizedStemWidth(sci->meta,sl->cur->fromlen,isvert)?
			    sl->cur->fromlen:sl->cur->tolen;
		only = sl->cur;
	    } else if ( RealApprox(len,sl->cur->fromlen) ||
		    RealApprox(len,sl->cur->tolen)) {
		/* Same size as last, it's ok */
	    } else
		len = -2;
	    if ( sl->cur->from == sp || sl->cur->to == sp )
		exact = sl->cur;
	}
    }
    if ( exact==NULL && len!=-2 ) exact = only;
    if ( exact!=NULL ) {
	if ( exact->spline1->from==sp ) {
	    mid->x = sp->me.x + exact->fromvec.x;
	    mid->y = sp->me.y + exact->fromvec.y;
	    len = exact->fromlen;
	} else if ( exact->spline1->to==sp ) {
	    mid->x = sp->me.x + exact->tovec.x;
	    mid->y = sp->me.y + exact->tovec.y;
	    len = exact->tolen;
	} else if (( exact->spline2->from==sp && !exact->spline2backwards) ||
		(exact->spline2->to==sp && exact->spline2backwards)) {
	    mid->x = sp->me.x - exact->fromvec.x;
	    mid->y = sp->me.y - exact->fromvec.y;
	    len = exact->fromlen;
	} else {
	    mid->x = sp->me.x - exact->tovec.x;
	    mid->y = sp->me.y - exact->tovec.y;
	    len = exact->tolen;
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
	real len, Spline *s) {
    BasePoint v, other, temp;
    int isvert = IsHVSpline(s,sp);

    v.x = mid->x-sp->me.x; v.y = mid->y-sp->me.y;
    other.x = mid->x+v.x; other.y = mid->y+v.y;
    len = MetaFontFindWidth(sci->meta,len,isvert)/2;	/* Get half the stem width */
    len /= sqrt(v.x*v.x + v.y*v.y);			/* Normalize vector which is half current width */
    v.x *= len; v.y *= len;
    if ( isvert!=2 ) {
	if ( SCIIsOnKnownEdge(sci,&sp->me,0) ) {
	    temp = sp->me;
	    SCIMapPoint(sci,&temp);
	    new->x = rint(temp.x);
	} else if ( SCIIsOnKnownEdge(sci,&other,0)) {
	    temp = other;
	    SCIMapPoint(sci,&temp);
	    new->x = rint(temp.x)-2*v.x;
	} else {
	    temp = *mid;
	    SCIMapPoint(sci,&temp);
	    if ( v.x>0 )
		new->x = rint(temp.x-v.x);
	    else
		new->x = rint(temp.x+v.x)-2*v.x;
	}
	if ( SCIIsOnKnownEdge(sci,&sp->me,1) ) {
	    temp = sp->me;
	    SCIMapPoint(sci,&temp);
	    new->y = rint(temp.y);
	} else if ( SCIIsOnKnownEdge(sci,&other,1)) {
	    temp = other;
	    SCIMapPoint(sci,&temp);
	    new->y = rint(temp.y)-2*v.y;
	} else {
	    temp = *mid;
	    SCIMapPoint(sci,&temp);
	    if ( v.y>0 )
		new->y = rint(temp.y-v.y);
	    else
		new->y = rint(temp.y+v.y)-2*v.y;
	}
    } else {
	temp = *mid;
	SCIMapPoint(sci,&temp);
	if ( v.y>0 || (v.x>0 && v.y==0)) {
	    new->x = rint(temp.x-v.x);
	    new->y = rint(temp.y-v.y);
	} else {
	    new->x = rint(temp.x+v.x)-2*v.x;
	    new->y = rint(temp.y+v.y)-2*v.y;
	}
    }
}

static void SplineIntersectWithEdge(SCI *sci,SplinePoint *sp,Spline *s,
	BasePoint *new,BasePoint *mid,BasePoint *v1) {
    BasePoint new1 = *new/*, new2, other*/;
    /*BasePoint v2;*/
    real mapped;

    if ( s->knownlinear ) {
	/* if the other spline is a line then we want the edge */
	/*  between us and the previous point to be parallel to */
	/*  that line. If the line is hor/vert we need to check it */
	/*  against the zones */
	if ( s->from->me.y==s->to->me.y && SCIIsOnKnownEdge(sci,&sp->me,1)) {
	    mapped = MapCoord(&sci->mapd[1],sp->me.y,sp->me.x);
	    if ( RealApprox(new1.y,mapped))
		new->y = mapped;
	    else if ( v1->y==0 )
		IError("Two Parallel vertical lines" );
	    else {
		new->y = mapped;
		new->x = new1.x + (mapped-new1.y)*v1->x/v1->y;
	    }
	} else if ( s->from->me.x==s->to->me.x && SCIIsOnKnownEdge(sci,&sp->me,0)) {
	    mapped = MapCoord(&sci->mapd[0],sp->me.x,sp->me.y);
	    if ( RealApprox(new1.x,mapped))
		new->x = mapped;
	    else if ( v1->x==0 )
		IError("Two Parallel horizontal lines" );
	    else {
		new->x = mapped;
		new->y = new1.y + (mapped-new1.x)*v1->y/v1->x;
	    }
	} else {
#if 0
	    v2.y = s->to->me.x-s->from->me.x; v2.x = -(s->to->me.y-s->from->me.y);
	    len = sqrt(v2.x*v2.x+v2.y*v2.y);
	    v2.y /= len; v2.x /= len;
	    off = (v2.x*(
#endif
	}
    }
}

static void SCIPositionPts(SCI *sci) {
    int i;
    real len1, len2;
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
		    if ( v2.x==0 ) {
			if ( !RealApprox(new1.x,new2.x) || !RealApprox(new1.y,new2.y))
			    IError("Two Parallel horizontal lines" );
		    } else
			/* Inherit x from new1 */
			new.y = new2.y + v2.y*(new.x-new2.x)/v2.x;
		} else if ( v2.x==0 ) {
		    new.x = new2.x;
		    new.y = new1.y + v1.y*(new.x-new1.x)/v1.x;
		} else if ( v2.y/v2.x == v1.y/v1.x ) {
		    if ( !RealApprox(new1.x,new2.x) || !RealApprox(new1.y,new2.y))
			IError("Two Parallel lines" );
		} else {
/* new1.y + v1.y*(X-new1.x)/v1.x = new2.y + v2.y*(X-new2.x)/v2.x */
/* new1.y-new2.y - v1.y/v1.x*new1.x + v2.y/v2.x*new2.x = (v2.y/v2.x-v1.y/v1.x)*X */
		    new.x = (new1.y-new2.y - v1.y/v1.x*new1.x + v2.y/v2.x*new2.x)/(v2.y/v2.x-v1.y/v1.x);
		    new.y = new1.y+ v1.y/v1.x*(new.x-new1.x);
		}
	    } else if ( len1!=0 ) {
		v1.y = mid1.x-sp->me.x; v1.x = -(mid1.y-sp->me.y);	/* This vector should be tangent to the spline */
		PositionFromMidLen(sci,&new,&mid1,sp,len1,sp->next);
		SplineIntersectWithEdge(sci,sp,sp->prev,&new,&mid1,&v1);
	    } else if ( len2!=0 ) {
		v2.y = mid2.x-sp->me.x; v2.x = -(mid2.y-sp->me.y);	/* (it's purp to something that's purp to the spline) */
		PositionFromMidLen(sci,&new,&mid2,sp,len2,sp->prev);
		SplineIntersectWithEdge(sci,sp,sp->next,&new,&mid2,&v2);
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
	    scale.x = ( sci->pts[i].newme.x - sci->pts[j].newme.x )/(sp->me.x-nsp->me.x);
	if ( sp->me.y!=nsp->me.y )
	    scale.y = ( sci->pts[i].newme.y - sci->pts[j].newme.y )/(sp->me.y-nsp->me.y);
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
    for ( i=0; i<sci->ptcnt; ++i ) {
	sp = sci->pts[i].cur;
	SplineRefigure(sp->next);
    }
}

static void SCIFixupHV(SCI *sci) {
    int i;
    SplinePoint *sp, *nsp;

return;		/* Debug !!!! */
    for ( i=0; i<sci->ptcnt; ++i ) {
	sp = sci->pts[i].cur;
	nsp = sp->next->to;
	if ( sp->me.x==nsp->me.x && sci->pts[i].newme.x!=sci->pts[nsp->ptindex].newme.x ) {
	    fprintf(stderr, "Vertical line no longer vertical (%g,%g) <-> (%g,%g) becomes (%g,%g) <-> (%g,%g)\n",
		    (double) sp->me.x, (double) sp->me.y, (double) nsp->me.x, (double) nsp->me.y,
		    (double) sci->pts[i].newme.x, (double) sci->pts[i].newme.y,
		    (double) sci->pts[nsp->ptindex].newme.x, (double) sci->pts[nsp->ptindex].newme.y );
	    sci->pts[i].newme.x = sci->pts[nsp->ptindex].newme.x =
		rint((sci->pts[i].newme.x + sci->pts[nsp->ptindex].newme.x)/2);
	}
	if ( sp->me.y==nsp->me.y && sci->pts[i].newme.y!=sci->pts[nsp->ptindex].newme.y ) {
	    fprintf(stderr, "Horizontal line no longer horizontal (%g,%g) <-> (%g,%g) becomes (%g,%g) <-> (%g,%g)\n",
		    (double) sp->me.x, (double) sp->me.y, (double) nsp->me.x, (double) nsp->me.y,
		    (double) sci->pts[i].newme.x, (double) sci->pts[i].newme.y,
		    (double) sci->pts[nsp->ptindex].newme.x, (double) sci->pts[nsp->ptindex].newme.y );
	    sci->pts[i].newme.y = sci->pts[nsp->ptindex].newme.y =
		rint((sci->pts[i].newme.y + sci->pts[nsp->ptindex].newme.y)/2);
	}
    }
}

static void MovePointToInter(SCI *sci,int i,int j, real val, int isvert) {
    /* Move the new version of point i to the place where the spline between */
    /*  it and point j intersects the horizontal/vertical line through val */
    SplinePoint pti, ptj, *midsp;
    extended ts[3], t;
    int k;
    Spline *spline;

    memset(&pti,'\0',sizeof(pti));
    memset(&ptj,'\0',sizeof(ptj));
    pti.me = sci->pts[i].newme;
    ptj.me = sci->pts[j].newme;
    if ( sci->pts[i].cur->prev->from->ptindex==j ) {
	pti.nextcp = sci->pts[i].newprev;
	ptj.prevcp = sci->pts[j].newnext;
    } else {
	pti.nextcp = sci->pts[i].newnext;
	ptj.prevcp = sci->pts[j].newprev;
    }
    spline = SplineMake3(&pti,&ptj);
    SplineSolveFull(&spline->splines[isvert],val,ts);
    t = 2;
    for ( k=0; k<3 ; ++k ) if ( ts[k]!=-1 )
	if ( t>ts[k] ) t=ts[k];
    if ( t==2 ) {
	SplineFree(spline);
return;
    }
    midsp = SplineBisect(spline,t);
    sci->pts[i].newme = midsp->me;
    if ( sci->pts[i].cur->prev->from->ptindex==j ) {
	sci->pts[i].newprev = midsp->nextcp;
	sci->pts[j].newnext = ptj.prevcp;
    } else {
	sci->pts[i].newnext = midsp->nextcp;
	sci->pts[j].newprev = ptj.prevcp;
    }
    SplineFree(midsp->prev); SplineFree(midsp->next);
    SplinePointFree(midsp);
}
    
static void SCICornerFixups(SCI *sci) {
    int i,j;
    SplinePoint *sp, *nsp;
    BasePoint *me, *nme;
    real mid, oldmid;
return;
    for ( i=0; i<sci->ptcnt; ++i ) {
	sp = sci->pts[i].cur;
	nsp = sp->next->to;
	me = &sci->pts[i].newme;
	nme = &sci->pts[nsp->ptindex].newme;
	if ( sp->me.x==nsp->me.x &&
		((sp->me.y>nsp->me.y && me->y<nme->y) ||
		    (sp->me.y<nsp->me.y && me->y>nme->y)) ) {
	    mid = (me->y+nme->y)/2, oldmid = (sp->me.y+nsp->me.y)/2;
	    j = nsp->ptindex;
	    MovePointToInter(sci,i,sp->prev->from->ptindex,mid+(sp->me.y-oldmid),1);
	    MovePointToInter(sci,j,nsp->next->to->ptindex,mid+(nsp->me.y-oldmid),1);
	    mid = rint((me->x+nme->x)/2);
	    sci->pts[i].newnext.x += mid-me->x;
	    sci->pts[i].newprev.x += mid-me->x;
	    sci->pts[j].newnext.x += mid-nme->x;
	    sci->pts[j].newprev.x += mid-nme->x;
	    me->x = nme->x = mid;
	}
	if ( sp->me.y==nsp->me.y &&
		((sp->me.x>nsp->me.x && me->x<nme->x) ||
		    (sp->me.x<nsp->me.x && me->x>nme->x)) ) {
	    mid = (me->x+nme->x)/2, oldmid = (sp->me.x+nsp->me.x)/2;
	    MovePointToInter(sci,i,sp->prev->from->ptindex,mid+(sp->me.x-oldmid),0);
	    MovePointToInter(sci,nsp->ptindex,nsp->next->to->ptindex,mid+(nsp->me.x-oldmid),0);
	    mid = rint((me->y+nme->y)/2);
	    sci->pts[i].newnext.y += mid-me->y;
	    sci->pts[i].newprev.y += mid-me->y;
	    sci->pts[j].newnext.y += mid-nme->y;
	    sci->pts[j].newprev.y += mid-nme->y;
	    me->y = nme->y = mid;
	}
    }
}

static void _MetaFont(MetaFontDlg *meta,SplineChar *sc) {
    SCI *sci;
    DBounds b;

    if ( sc==NULL || sc->layers[ly_fore].refs!=NULL )
return;

    sci = SCIinit(sc,meta);
    SCIFindStems(sci);
    if ( !SCIReasonableConnections(sci)) {
	IError( "Could not deal with %s, missing stems", sc->name );
	SCIFree(sci);
return;
    }
    SCIFigureWidths(sci);
    SCIBuildMaps(sci,0);		/* Horizontal maps */
    SCIBuildMaps(sci,1);		/* Vertical maps */
    SCIPositionPts(sci);
    SCIFixupHV(sci);
    SCIPositionControls(sci);
    SCICornerFixups(sci);
    SCISet(sci);
    if ( !meta->scalewidth ) {
	SplineCharFindBounds(sc,&b);
	sci->width = sci->rbearing + b.maxx;
    }
    SCSynchronizeWidth(sc,sci->width,sc->width,meta->fv);
    StemInfosFree(sc->hstem); sc->hstem = NULL;
    StemInfosFree(sc->vstem); sc->vstem = NULL;
    DStemInfosFree(sc->dstem); sc->dstem = NULL;
    SCOutOfDateBackground(sc);
    SCCharChangedUpdate(sc);
    SCIFree(sci);
}

static int lastdlgtype=0;

static GTextInfo dlgtypes[] = {
    { (unichar_t *) N_("Simple"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Advanced"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

static GTextInfo simplefuncs[] = {
    { (unichar_t *) N_("Embolden"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 1, 0, 1},
    { (unichar_t *) N_("Thin"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Condense"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Expand"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

#define CID_DlgType		1001
#define CID_SimpleFuncs		1002
#define CID_AdvancedTabs	1003
#define CID_StemScale		1004
#define CID_CounterScale	1005
#define CID_StemScaleTxt	1006
#define CID_CounterScaleTxt	1007
#define CID_StemScalePer	1008
#define CID_CounterScalePer	1009
#define CID_XH_From		1010
#define CID_XH_OldVal		1011
#define CID_XH_To		1012
#define CID_XH_Val		1013


static int MT_OK(GGadget *g, GEvent *e) {
    int type, func;
    MetaFontDlg *meta;
    int i, cnt, gid;
    real stems, counters, xh=0;
    int err;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	meta = GDrawGetUserData(GGadgetGetWindow(g));
	type = GGadgetGetFirstListSelectedItem(GWidgetGetControl(meta->gw,CID_DlgType));
	if ( type==0 ) {
	    func = GGadgetGetFirstListSelectedItem(GWidgetGetControl(meta->gw,CID_SimpleFuncs));
	    err = false;
	    stems = GetReal8(meta->gw,CID_StemScale, _("Scale Stems By:"),&err)/100;
	    counters = GetReal8(meta->gw,CID_CounterScale,_("Scale Counters By:"),&err)/100;
	    if ( meta->bxh!=-1 )
		xh = GetReal8(meta->gw,CID_XH_Val,_("_X-Height"),&err);
	    if ( err )
return( true );
	    meta->counters[0].counterchoices = cc_edgefixed;
	    if ( stems!=1 ) {
		meta->stemcnt = 1;
		meta->stems = gcalloc(1,sizeof(struct stemcntl));
		meta->stems->small = 0;
		meta->stems->wide = (meta->sf->ascent+meta->sf->descent)/4;
		meta->stems->factor = stems;
	    }
	    meta->counters[0].counter.factor = counters;
	    if ( func==2 || func==3 )
		meta->lbearing.factor = meta->rbearing.factor = meta->counters[0].counter.factor;
	    else
		meta->lbearing.factor = meta->rbearing.factor = 1;

	    meta->counters[1].counterchoices = cc_zones;
	    meta->counters[1].zonecnt = meta->bcnt;
	    meta->counters[1].zones = galloc(meta->bcnt*sizeof(struct zonemap));
	    for ( i=0; i<meta->bcnt; ++i )
		meta->counters[1].zones[i].from = meta->counters[1].zones[i].to = meta->blues[i];
	    if ( meta->bxh!=-1 )
		meta->counters[1].zones[meta->bxh].to = xh;
	    if ( meta->cv!=NULL )
		_MetaFont(meta,meta->cv->sc);
	    else if ( meta->sc!=NULL )
		_MetaFont(meta,meta->sc);
	    else {
		for ( cnt=i=0; i<meta->fv->map->enccount; ++i )
		    if ( meta->fv->selected[i] && (gid=meta->fv->map->map[i])!=-1 &&
			    meta->fv->sf->glyphs[gid]!=NULL )
			++cnt;
#if defined(FONTFORGE_CONFIG_GDRAW)
		gwwv_progress_start_indicator(10,_("Metamorphosing Font..."),_("Metamorphosing Font..."),0,cnt,1);
#elif defined(FONTFORGE_CONFIG_GTK)
		gwwv_progress_start_indicator(10,_("Metamorphosing Font..."),_("Metamorphosing Font..."),0,cnt,1);
#endif
		SFUntickAll(meta->fv->sf);
		for ( i=0; i<meta->fv->map->enccount; ++i )
		    if ( meta->fv->selected[i] && (gid=meta->fv->map->map[i])!=-1 &&
			    meta->fv->sf->glyphs[gid]!=NULL &&
			    !meta->fv->sf->glyphs[gid]->ticked ) {
			_MetaFont(meta,meta->fv->sf->glyphs[gid]);
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
	    }
	}
	lastdlgtype = type;
	meta->done = true;
    }
return( true );
}

static int MT_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MetaFontDlg *meta = GDrawGetUserData(GGadgetGetWindow(g));
	meta->done = true;
    }
return( true );
}

static void FuncSet(MetaFontDlg *meta) {
    int func = GGadgetGetFirstListSelectedItem(GWidgetGetControl(meta->gw,CID_SimpleFuncs));
    double stemval = func==0?170:func==1?70:100;
    double counterval = func==0?110:func==1?95:func==2?75:125;
    char buffer[10];
    unichar_t ustem[10], ucounter[10];

    sprintf(buffer,"%g",stemval);
    uc_strcpy(ustem,buffer);
    sprintf(buffer,"%g",counterval);
    uc_strcpy(ucounter,buffer);
    GGadgetSetTitle(GWidgetGetControl(meta->gw,CID_StemScale),ustem);
    GGadgetSetTitle(GWidgetGetControl(meta->gw,CID_CounterScale),ucounter);
}    
    
static int MT_FuncChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	MetaFontDlg *meta = GDrawGetUserData(GGadgetGetWindow(g));
	FuncSet(meta);
    }
return( true );
}

static int MT_AspectChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
    }
return( true );
}

static void DlgSetup(MetaFontDlg *meta) {
    int type = GGadgetGetFirstListSelectedItem(GWidgetGetControl(meta->gw,CID_DlgType));
    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_SimpleFuncs),type==0);
    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_StemScale),type==0);
    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_StemScaleTxt),type==0);
    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_StemScalePer),type==0);
    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_CounterScale),type==0);
    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_CounterScaleTxt),type==0);
    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_CounterScalePer),type==0);
    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_XH_From),type==0 && meta->bxh!=-1);
    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_XH_OldVal),type==0 && meta->bxh!=-1);
    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_XH_To),type==0 && meta->bxh!=-1);
    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_XH_Val),type==0 && meta->bxh!=-1);

    GGadgetSetVisible(GWidgetGetControl(meta->gw,CID_AdvancedTabs),type==1);
}

static int GFI_DlgTypeChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	MetaFontDlg *meta = GDrawGetUserData(GGadgetGetWindow(g));
	DlgSetup(meta);
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	MetaFontDlg *meta = GDrawGetUserData(gw);
	meta->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("MetaFont.html");
return( true );
	}
return( false );
    }
return( true );
}

void MetaFont(FontView *fv,CharView *cv,SplineChar *sc) {
    GRect pos;
    GWindowAttrs wattrs;
    GTabInfo aspects[8];
    GGadgetCreateData mgcd[18];
    GTextInfo mlabel[18];
    MetaFontDlg meta;
    int i;
    BlueData bd;
    char buffer[20];
    static int done = false;

    if ( !done ) {
	done = true;
	for ( i=0; dlgtypes[i].text!=NULL; ++i )
	    dlgtypes[i].text = (unichar_t *) _((char *) dlgtypes[i].text);
	for ( i=0; simplefuncs[i].text!=NULL; ++i )
	    simplefuncs[i].text = (unichar_t *) _((char *) simplefuncs[i].text);
    }

    memset(&meta,'\0',sizeof(meta));
    meta.fv = fv; meta.cv = cv; meta.sc = sc;
    counterwarned = false;
    if ( cv!=NULL )
	meta.sf = cv->sc->parent;
    else
	meta.sf = fv->sf;
    QuickBlues(meta.sf,&bd);
    meta.bcnt = 0; meta.bxh = -1;
    if ( bd.descent<0 ) meta.blues[meta.bcnt++] = bd.descent;
    meta.blues[meta.bcnt++] = 0;
    if ( bd.xheight>0 ) {
	meta.bxh = meta.bcnt;
	meta.blues[meta.bcnt++] = bd.xheight;
    }
    if ( bd.ascent>bd.caph ) {
	if ( bd.caph!=0 ) meta.blues[meta.bcnt++] = bd.caph;
	if ( bd.ascent>bd.caph + bd.caph/50 ) meta.blues[meta.bcnt++] = bd.ascent;
    } else {
	if ( bd.caph>bd.ascent + bd.ascent/50 ) meta.blues[meta.bcnt++] = bd.ascent;
	if ( bd.caph!=0 ) meta.blues[meta.bcnt++] = bd.caph;
    }

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Meta Font...");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,268));
    pos.height = GDrawPointsToPixels(NULL,330);
    meta.gw = GDrawCreateTopWindow(NULL,&pos,e_h,&meta,&wattrs);

    memset(&mlabel,0,sizeof(mlabel));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&aspects,'\0',sizeof(aspects));
    for ( i=0; dlgtypes[i].text!=NULL; ++i )
	dlgtypes[i].selected = ( i==lastdlgtype );

    i = 0;
    aspects[i].text = (unichar_t *) _("Stems");
    aspects[i].selected = true;
    aspects[i++].text_is_1byte = true;
    /*aspects[i++].gcd = ngcd;*/

    aspects[i].text = (unichar_t *) _("H Counters");
    aspects[i++].text_is_1byte = true;

    aspects[i].text = (unichar_t *) _("V Counters");
    aspects[i++].text_is_1byte = true;

    mgcd[0].gd.pos.x = 6; mgcd[0].gd.pos.y = 6;
    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].gd.cid = CID_DlgType;
    mgcd[0].gd.u.list = dlgtypes;
    mgcd[0].gd.handle_controlevent = GFI_DlgTypeChange;
    mgcd[0].creator = GListButtonCreate;

    mgcd[1].gd.pos.x = 16; mgcd[1].gd.pos.y = 36;
    mgcd[1].gd.flags = gg_enabled;
    mgcd[1].gd.cid = CID_SimpleFuncs;
    mgcd[1].gd.u.list = simplefuncs;
    mgcd[1].gd.handle_controlevent = MT_FuncChange;
    mgcd[1].creator = GListButtonCreate;

    mgcd[2].gd.pos.x = 4; mgcd[2].gd.pos.y = 34;
    mgcd[2].gd.pos.width = 260;
    mgcd[2].gd.pos.height = 260;
    mgcd[2].gd.u.tabs = aspects;
    mgcd[2].gd.flags = gg_enabled;
    mgcd[2].gd.handle_controlevent = MT_AspectChange;
    mgcd[2].gd.cid = CID_AdvancedTabs;
    mgcd[2].creator = GTabSetCreate;

    mgcd[1+lastdlgtype].gd.flags |= gg_visible;

    mgcd[3].gd.pos.x = 16; mgcd[3].gd.pos.y = mgcd[1].gd.pos.y+36+6;
    mgcd[3].gd.flags = gg_enabled;
    mlabel[3].text = (unichar_t *) _("Scale Stems By:");
    mlabel[3].text_is_1byte = true;
    mgcd[3].gd.label = &mlabel[3];
    mgcd[3].gd.cid = CID_StemScaleTxt;
    mgcd[3].creator = GLabelCreate;

    mgcd[4].gd.pos.x = 108; mgcd[4].gd.pos.y = mgcd[3].gd.pos.y-6; mgcd[4].gd.pos.width=50;
    mgcd[4].gd.flags = gg_enabled;
    mgcd[4].gd.cid = CID_StemScale;
    mgcd[4].creator = GTextFieldCreate;

    mgcd[5].gd.pos.x = mgcd[4].gd.pos.x+mgcd[4].gd.pos.width+3; mgcd[5].gd.pos.y = mgcd[3].gd.pos.y;
    mgcd[5].gd.flags = gg_enabled;
    mlabel[5].text = (unichar_t *) "%";
    mlabel[5].text_is_1byte = true;
    mgcd[5].gd.label = &mlabel[5];
    mgcd[5].gd.cid = CID_StemScalePer;
    mgcd[5].creator = GLabelCreate;

    mgcd[6].gd.pos.x = 16; mgcd[6].gd.pos.y = mgcd[4].gd.pos.y+26+6;
    mgcd[6].gd.flags = gg_enabled;
    mlabel[6].text = (unichar_t *) _("Scale Counters By:");
    mlabel[6].text_is_1byte = true;
    mgcd[6].gd.label = &mlabel[6];
    mgcd[6].gd.cid = CID_CounterScaleTxt;
    mgcd[6].creator = GLabelCreate;

    mgcd[7].gd.pos.x = mgcd[4].gd.pos.x; mgcd[7].gd.pos.y = mgcd[6].gd.pos.y-6; mgcd[7].gd.pos.width=50;
    mgcd[7].gd.flags = gg_enabled;
    mgcd[7].gd.cid = CID_CounterScale;
    mgcd[7].creator = GTextFieldCreate;

    mgcd[8].gd.pos.x = mgcd[7].gd.pos.x+mgcd[7].gd.pos.width+3; mgcd[8].gd.pos.y = mgcd[6].gd.pos.y;
    mgcd[8].gd.flags = gg_enabled;
    mlabel[8].text = (unichar_t *) "%";
    mlabel[8].text_is_1byte = true;
    mgcd[8].gd.label = &mlabel[8];
    mgcd[8].gd.cid = CID_CounterScalePer;
    mgcd[8].creator = GLabelCreate;

    mgcd[9].gd.pos.x = 16; mgcd[9].gd.pos.y = mgcd[7].gd.pos.y+26+6;
    mgcd[9].gd.flags = gg_enabled;
    mlabel[9].text = (unichar_t *) _("XHeight From:");
    mlabel[9].text_is_1byte = true;
    mgcd[9].gd.label = &mlabel[9];
    mgcd[9].gd.cid = CID_XH_From;
    mgcd[9].creator = GLabelCreate;

    sprintf( buffer, "%g", (double) bd.xheight );
    mgcd[10].gd.pos.x = mgcd[4].gd.pos.x; mgcd[10].gd.pos.y = mgcd[9].gd.pos.y;
    mgcd[10].gd.flags = gg_enabled;
    mlabel[10].text = (unichar_t *) buffer;
    mlabel[10].text_is_1byte = true;
    mgcd[10].gd.label = &mlabel[10];
    mgcd[10].gd.cid = CID_XH_OldVal;
    mgcd[10].creator = GLabelCreate;

    mgcd[11].gd.pos.x = mgcd[10].gd.pos.x+30; mgcd[11].gd.pos.y = mgcd[9].gd.pos.y;
    mgcd[11].gd.flags = gg_enabled;
    mlabel[11].text = (unichar_t *) _("To:");
    mlabel[11].text_is_1byte = true;
    mgcd[11].gd.label = &mlabel[11];
    mgcd[11].gd.cid = CID_XH_To;
    mgcd[11].creator = GLabelCreate;

    mgcd[12].gd.pos.x = mgcd[11].gd.pos.x+30; mgcd[12].gd.pos.y = mgcd[9].gd.pos.y-6; mgcd[12].gd.pos.width=50;
    mgcd[12].gd.flags = gg_enabled;
    mgcd[12].gd.label = &mlabel[10];	/* Initialized same as old value */
    mgcd[12].gd.cid = CID_XH_Val;
    mgcd[12].creator = GTextFieldCreate;

    mgcd[13].gd.pos.x = 30-3; mgcd[13].gd.pos.y = 330-35-3;
    mgcd[13].gd.pos.width = -1; mgcd[13].gd.pos.height = 0;
    mgcd[13].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[13].text = (unichar_t *) _("_OK");
    mlabel[13].text_is_1byte = true;
    mlabel[13].text_in_resource = true;
    mgcd[13].gd.label = &mlabel[13];
    mgcd[13].gd.handle_controlevent = MT_OK;
    mgcd[13].creator = GButtonCreate;

    mgcd[14].gd.pos.x = -30; mgcd[14].gd.pos.y = mgcd[13].gd.pos.y+3;
    mgcd[14].gd.pos.width = -1; mgcd[14].gd.pos.height = 0;
    mgcd[14].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[14].text = (unichar_t *) _("_Cancel");
    mlabel[14].text_is_1byte = true;
    mlabel[14].text_in_resource = true;
    mgcd[14].gd.label = &mlabel[14];
    mgcd[14].gd.handle_controlevent = MT_Cancel;
    mgcd[14].creator = GButtonCreate;

    mgcd[15].gd.pos.x = 2; mgcd[15].gd.pos.y = 2;
    mgcd[15].gd.pos.width = pos.width-4; mgcd[15].gd.pos.height = pos.height-2;
    mgcd[15].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    mgcd[15].creator = GGroupCreate;

    GGadgetsCreate(meta.gw,mgcd);
    DlgSetup(&meta);
    FuncSet(&meta);

    GDrawSetVisible(meta.gw,true);
    while ( !meta.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(meta.gw);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
