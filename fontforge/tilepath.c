/* Copyright (C) 2002-2006 by George Williams */
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
#include <math.h>
#include <gkeysym.h>

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#ifdef FONTFORGE_CONFIG_TILEPATH
/* Given a path and a splineset */
/* Treat the splineset as a tile and lay it down on the path until we reach the*/
/*  end of the path */
/* More precisely, find the length of the path */
/* Find the height of the tile */
/* We'll need length/height tiles */
/* For each tile */
/*  For a point on the central (in x) axis of the tile */
/*   Use its y-position to figure out how far along the path we are ( y-pos/length ) */
/*   Then this point should be moved to exactly that point */
/*  For a point off the central axis */
/*   Perform the above calculation, and */
/*   Find the normal vector to the path */
/*   Our new location should be: */
/*	the location found above + our xoffset * (normal vector) */
/*  Do that for a lot of points on each spline of the tile and then */
/*   use approximate spline from points to find the new splines */
/* Complications: */
/*  There may not be an integral number of tiles, so we must be prepared to truncate some splines */

typedef struct tiledata {
    SplineSet *basetile;	/* Moved so that ymin==0, and x is adjusted */
				/*  about the x-axis as implied by tilepos */
    SplineSet *tileset;		/* As many copies of the basetile as we are */
				/*  going to need. Each successive one bb.ymax */
			        /*  higher than the last */
    SplineSet *result;		/* Final result after transformation */
    DBounds bb;			/* Of the basetile */

    SplineSet *path;
    double plength;		/* Length of path */
    int pcnt;			/* Number of splines in path */
    int nsamples;
    struct tdsample {
	real dx, dy;		/* offset from path->first->me */
	real c,s;		/* cos/sin of normal vector pointing right of path */
    } *samples;			/* an array of [nsamples+1] actually */
    int njoins;
    struct jsample {
	real dx, dy;
	real c1,s1;
	real c2,s2;
	real sofar;
    } *joins;			/* an array of [pcnt or pcnt-1], one of each join */

    enum tilepos { tp_left, tp_center, tp_right } tilepos;
    enum tilescale { ts_tile, ts_tilescale, ts_scale } tilescale;
    /* ts_scale means that we scale the one tile until it height is the same */
    /*	 as plength */
    /* ts_tile means that we lay down as many tiles as we need so that */
    /*	 n*tile-height == plength. Note: n need not be an integer, so we */
    /*   may be an incomplete tile => incomplete splines (a spline may even */
    /*   get cut so that it becomes two splines) */
    /* ts_tilescale means that we find n = floor(plength/tile-height) and */
    /*	 scale = plength/(n*tile-height). We scale the tile by "scale", and */
    /*   then lay down n of them */

    int doallpaths;
} TD;

static int TDMakeSamples(TD *td) {
    Spline *spline, *first;
    double len, slen, sofar, t, toff, dt_per_sample;
    int i,end, base, pcnt;
    double sx, sy, angle;

    first = NULL; len = 0; pcnt = 0;
    for ( spline=td->path->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	if ( first==NULL ) first = spline;
	len += SplineLength(spline);
	++pcnt;
    }
    if ( len==0 )
return( false );
    td->plength = len;
    td->pcnt = pcnt;

    td->nsamples = ceil(len)+10;
    td->samples = galloc((td->nsamples+1)*sizeof(struct tdsample));
    td->joins = galloc(td->pcnt*sizeof(struct jsample));

    i = 0; pcnt = 0;
    first = NULL; sofar = 0;
    for ( spline=td->path->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	if ( first==NULL ) first = spline;
	slen = SplineLength(spline);
	/* I'm assuming that length is approximately linear in t */
	toff = (i - td->nsamples*sofar/len)/slen;
	base = i;
	end = floor(td->nsamples*(sofar+slen)/len);
	dt_per_sample = end==i?1:(1.0-toff)/(end-i);
	if ( spline->to->next==NULL || spline->to->next==first )
	    end = td->nsamples;
	while ( i<=end ) {
	    t = toff + (i-base)*dt_per_sample;
	    if ( i==td->nsamples || t>1 ) t = 1;
	    td->samples[i].dx = ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t + spline->splines[0].d /*-
		    td->path->first->me.x*/;
	    td->samples[i].dy = ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t + spline->splines[1].d /*-
		    td->path->first->me.y*/;
	    sx = (3*spline->splines[0].a*t+2*spline->splines[0].b)*t+spline->splines[0].c;
	    sy = (3*spline->splines[1].a*t+2*spline->splines[1].b)*t+spline->splines[1].c;
	    if ( sx==0 && sy==0 ) {
		sx = spline->to->me.x - spline->from->me.x;
		sy = spline->to->me.y - spline->from->me.y;
	    }
	    angle = atan2(sy,sx) - 3.1415926535897932/2;
	    td->samples[i].c = cos(angle);
	    td->samples[i].s = sin(angle);
	    if ( td->samples[i].s>-.00001 && td->samples[i].s<.00001 ) { td->samples[i].s=0; td->samples[i].c = ( td->samples[i].c>0 )? 1 : -1; }
	    if ( td->samples[i].c>-.00001 && td->samples[i].c<.00001 ) { td->samples[i].c=0; td->samples[i].s = ( td->samples[i].s>0 )? 1 : -1; }
	    ++i;
	}
	sofar += slen;
	if (( pcnt<td->pcnt-1 || td->path->first==td->path->last ) &&
		!((spline->to->pointtype==pt_curve && !spline->to->nonextcp && !spline->to->noprevcp) ||
		  (spline->to->pointtype==pt_tangent && spline->to->nonextcp+spline->to->noprevcp==1 )) ) {
	    /* We aren't interested in joins where the two splines are tangent */
	    Spline *next = spline->to->next;
	    td->joins[pcnt].sofar = sofar;
	    td->joins[pcnt].dx = spline->to->me.x;
	    td->joins[pcnt].dy = spline->to->me.y;
	    /* there are two normals at a join, one for each spline */
	    /*  it should bisect the normal vectors of the two splines */
	    sx = next->splines[0].c; sy = next->splines[1].c;
	    angle = atan2(sy,sx) - 3.1415926535897932/2;
	    td->joins[pcnt].c1 = cos(angle);
	    td->joins[pcnt].s1 = sin(angle);
	    if ( td->joins[pcnt].s1>-.00001 && td->joins[pcnt].s1<.00001 ) { td->joins[pcnt].s1=0; td->joins[pcnt].c1 = ( td->joins[pcnt].c1>0 )? 1 : -1; }
	    if ( td->joins[pcnt].c1>-.00001 && td->joins[pcnt].c1<.00001 ) { td->joins[pcnt].c1=0; td->joins[pcnt].s1 = ( td->joins[pcnt].s1>0 )? 1 : -1; }

	    sx = (3*spline->splines[0].a+2*spline->splines[0].b)+spline->splines[0].c;
	    sy = (3*spline->splines[1].a+2*spline->splines[1].b)+spline->splines[1].c;
	    angle = atan2(sy,sx) - 3.1415926535897932/2;
	    td->joins[pcnt].c2 = cos(angle);
	    td->joins[pcnt].s2 = sin(angle);
	    if ( td->joins[pcnt].s2>-.00001 && td->joins[pcnt].s2<.00001 ) { td->joins[pcnt].s2=0; td->joins[pcnt].c2 = ( td->joins[pcnt].c2>0 )? 1 : -1; }
	    if ( td->joins[pcnt].c2>-.00001 && td->joins[pcnt].c2<.00001 ) { td->joins[pcnt].c2=0; td->joins[pcnt].s2 = ( td->joins[pcnt].s2>0 )? 1 : -1; }
	    ++pcnt;
	}
    }
    td->njoins = pcnt;
    if ( i!=td->nsamples+1 )
	IError("Sample failure %d is not %d", i, td->samples+1 );
return( true );
}

static void TDAddPoints(TD *td) {
    /* Insert additional points in the tileset roughly at the locations */
    /*  corresponding to the ends of the splines in the path */
    SplineSet *spl;
    Spline *spline, *first, *tsp;
    double len;
    double ts[3];

    first = NULL; len = 0;
    for ( spline=td->path->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	if ( first==NULL ) first = spline;
	if ( spline->to->next==NULL || spline->to->next==first )
    break;
	len += SplineLength(spline);

	for ( spl=td->tileset; spl!=NULL; spl=spl->next ) {
	    for ( tsp=spl->first->next; tsp!=NULL ; tsp = tsp->to->next ) {
		if ( RealApprox(tsp->to->me.y,len) || RealApprox(tsp->from->me.y,len))
		    /* Do Nothing, already broken here */;
		else if ( (tsp->to->me.y>len || tsp->to->prevcp.y>len || tsp->from->me.y>len || tsp->from->nextcp.y>len) &&
			  (tsp->to->me.y<len || tsp->to->prevcp.y<len || tsp->from->me.y<len || tsp->from->nextcp.y<len) &&
			  SplineSolveFull(&tsp->splines[1],len,ts) )
		    SplineBisect(tsp,ts[0]);
		if ( tsp->to == spl->first )
	    break;
	    }
	}
    }
}

static void SplineSplitAtY(Spline *spline,real y) {
    double ts[3];
    SplinePoint *last;

    if ( spline->from->me.y<=y && spline->from->nextcp.y<=y &&
	    spline->to->me.y<=y && spline->to->prevcp.y<=y )
return;
    if ( spline->from->me.y>=y && spline->from->nextcp.y>=y &&
	    spline->to->me.y>=y && spline->to->prevcp.y>=y )
return;


    if ( !SplineSolveFull(&spline->splines[1],y,ts) )
return;

    last = spline->to;
    spline = SplineSplit(spline,ts);
    while ( spline->to!=last ) {
	if ( spline->to->me.y!=y ) {
	    real diff = y-spline->to->me.y;
	    spline->to->me.y = y;
	    spline->to->prevcp.y += diff;
	    spline->to->nextcp.y += diff;
	    SplineRefigure(spline); SplineRefigure(spline->to->next);
	}
	spline = spline->to->next;
    }
}

static void _SplinesRemoveBetween( Spline *spline, Spline *beyond, SplineSet *spl ) {
    Spline *next;

    while ( spline!=NULL && spline!=beyond ) {
	next = spline->to->next;
	if ( spline->from!=spl->last && spline->from!=spl->first )
	    SplinePointFree(spline->from);
	SplineFree(spline);
	spline = next;
    }
}

static SplineSet *SplinePointListTruncateAtY(SplineSet *spl,real y) {
    SplineSet *prev=NULL, *ss=spl, *nprev, *snext, *ns;
    Spline *spline, *next;

    for ( ; spl!=NULL; spl = spl->next ) {
	for ( spline=spl->first->next ; spline!=NULL; spline = next ) {
	    next = spline->to->next;
	    SplineSplitAtY(spline,y);
	    if ( next==NULL || next->from == spl->last )
	break;
	}
    }

    prev = NULL;
    y += 1/128.0;		/* Small fudge factor for rounding errors */
    for ( spl=ss; spl!=NULL; spl = snext ) {
	snext = spl->next;
	nprev = spl;
	for ( spline=spl->first->next ; spline!=NULL ; spline = next ) {
	    next = spline->to->next;
	    if ( spline->from->me.y<=y && spline->from->nextcp.y<=y &&
		    spline->to->me.y<=y && spline->to->prevcp.y<=y ) {
		if ( spline->to==spl->first )
	break;
		else
	continue;
	    }
	    /* Remove this spline */
	    while ( next!=NULL && next->from!=spl->first &&
		    (next->from->me.y>y || next->from->nextcp.y>y ||
		     next->to->me.y>y || next->to->prevcp.y>y) )
		next = next->to->next;
	    if ( next==NULL || next->from==spl->first ) {
		/* The area to be removed continues to the end of splineset */
		if ( spline==spl->first->next ) {
		    /* Remove entire splineset */
		    if ( prev==NULL )
			ss = snext;
		    else
			prev->next = snext;
		    SplinePointListFree(spl);
		    nprev = prev;
	break;
		}
		spl->last = spline->from;
		spline->from->next = NULL;
		spl->first->prev = NULL;
		_SplinesRemoveBetween(spline,next,spl);
	break;
	    } else {
		if ( spline==spl->first->next ) {
		    /* Remove everything before next */
		    next->from->prev = NULL;
		    spl->first->next = NULL;
		    spl->first = next->from;
		} else if ( spl->first==spl->last ) {
		    /* rotate splineset so break is at start and end. */
		    spl->last = spline->from;
		    spl->first = next->from;
		    next->from->prev = NULL;
		    spline->from->next = NULL;
		} else {
		    /* Split into two splinesets and remove all between */
		    ns = chunkalloc(sizeof(SplineSet));
		    ns->first = next->from;
		    ns->last = spl->last;
		    spl->last = spline->from;
		    spline->from->next = NULL;
		    spl->first->prev = NULL;
		    next->from->prev = NULL;
		    ns->last->next = NULL;
		    ns->next = spl->next;
		    spl->next = ns->next;
		    nprev = ns;
		}
		_SplinesRemoveBetween(spline,next,spl);
		spl = nprev;
	    }
	}
	prev = nprev;
    }
return( ss );
}

static SplineSet *SplinePointListMerge(SplineSet *old,SplineSet *new) {
    /* Merge the new splineset into the old looking for any endpoints */
    /*  common to both, and if any are found, merging them */
    SplineSet *test1, *next;
    SplineSet *oldold = old;

    while ( new!=NULL ) {
	next = new->next;
	if ( new->first!=new->last ) {
	    for ( test1=oldold; test1!=NULL; test1=test1->next ) {
		if ( test1->first!=test1->last &&
			((test1->first->me.x==new->first->me.x && test1->first->me.y==new->first->me.y) ||
			 (test1->last->me.x==new->first->me.x && test1->last->me.y==new->first->me.y) ||
			 (test1->first->me.x==new->last->me.x && test1->first->me.y==new->last->me.y) ||
			 (test1->last->me.x==new->last->me.x && test1->last->me.y==new->last->me.y)) )
		    break;
	    }
	    if ( test1!=NULL ) {
		if ((test1->first->me.x==new->first->me.x && test1->first->me.y==new->first->me.y) ||
			(test1->last->me.x==new->last->me.x && test1->last->me.y==new->last->me.y))
		    SplineSetReverse(new);
		if ( test1->last->me.x==new->first->me.x && test1->last->me.y==new->first->me.y ) {
		    test1->last->nextcp = new->first->nextcp;
		    test1->last->nonextcp = new->first->nonextcp;
		    test1->last->nextcpdef = new->first->nextcpdef;
		    test1->last->next = new->first->next;
		    new->first->next->from = test1->last;
		    test1->last = new->last;
		    SplinePointFree(new->first);
		    new->first = new->last = NULL;
		    SplinePointListFree(new);
		    if ( test1->last->me.x == test1->first->me.x &&
			    test1->last->me.y == test1->first->me.y ) {
			test1->first->prevcp = test1->last->prevcp;
			test1->first->noprevcp = test1->last->noprevcp;
			test1->first->prevcpdef = test1->last->prevcpdef;
			test1->last->prev->to = test1->first;
			SplinePointFree(test1->last);
			test1->last = test1->first;
		    }
		} else {
		    test1->first->prevcp = new->last->prevcp;
		    test1->first->noprevcp = new->last->noprevcp;
		    test1->first->prevcpdef = new->last->prevcpdef;
		    test1->first->prev = new->last->prev;
		    new->last->prev->to = test1->first;
		    test1->first = new->first;
		    SplinePointFree(new->last);
		    new->first = new->last = NULL;
		    SplinePointListFree(new);
		}
		new = next;
    continue;
	    }
	}
	new->next = old;
	old = new;
	new = next;
    }
return( old );
}

static void TileLine(TD *td) {
    int tilecnt=1, i;
    double scale=1, y;
    real trans[6];
    SplineSet *new;

    switch ( td->tilescale ) {
      case ts_tile:
	tilecnt = ceil( td->plength/td->bb.maxy );
      break;
      case ts_scale:
	scale = td->plength/td->bb.maxy ;
      break;
      case ts_tilescale:
	scale = td->plength/td->bb.maxy ;
	tilecnt = floor( scale );
	if ( tilecnt==0 )
	    tilecnt = 1;
	else if ( scale-tilecnt>.707 )
	    ++tilecnt;
	scale = td->plength/(tilecnt*td->bb.maxy);
      break;
    }

    y = 0;
    trans[0] = 1; trans[3] = scale;		/* Only scale y */
    trans[1] = trans[2] = trans[4] = trans[5] = 0;
    for ( i=0; i<tilecnt; ++i ) {
	new = SplinePointListCopy(td->basetile);
	trans[5] = y;
	new = SplinePointListTransform(new,trans,true);
	if ( i==tilecnt-1 && scale==1 )
	    new = SplinePointListTruncateAtY(new,td->plength);
	td->tileset = SplinePointListMerge(td->tileset,new);
	y += td->bb.maxy*scale;
    }
    if ( td->pcnt>1 ) {
	/* If there are fewer tiles than there are spline elements, then we */
	/*  may not be able to do a good job approximating (suppose the path */
	/*  draws a circle, but there is just one tile, a straight line. */
	/*  without some extra points in the middle of that line there is no */
	/*  way to make a circle). So here we add some extra breaks */
	/* Actually, it's worse than that. If the transition isn't smooth */
	/*  then we'll always want those extra points... */
	TDAddPoints(td);
    }
}

static void AdjustPoint(TD *td,Spline *spline,double t,TPoint *to) {
    double x, y;
    double pos;
    int low;
    double dx, dy, c, s;
    int i;

    to->t = t;

    x = ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t + spline->splines[0].d;
    y = ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t + spline->splines[1].d;

    for ( i=td->pcnt-2; i>=0; --i )
	if ( RealNearish(y,td->joins[i].sofar) )
    break;
    if ( i>=0 ) {
	double x1,y1, x2, y2, dx1, dx2, dy1, dy2;
	x1 = td->joins[i].dx + td->joins[i].c1*x;
	y1 = td->joins[i].dy + td->joins[i].s1*x;
	dx1 = -td->joins[i].s1;
	dy1 = td->joins[i].c1;

	x2 = td->joins[i].dx + td->joins[i].c2*x;
	y2 = td->joins[i].dy + td->joins[i].s2*x;
	dx2 = -td->joins[i].s2;
	dy2 = td->joins[i].c2;
	/* there are two lines at a join and I need to find the intersection */
	if ( dy2>-.00001 && dy2<.00001 ) {
	    to->y = y2;
	    if ( dy1>-.00001 && dy1<.00001 )	/* essentially parallel */
		to->x = x2;
	    else
		to->x = x1 + dx1*(y2-y1)/dy1;
	} else {
	    double s=(dy1*dx2/dy2-dx1);
	    if ( s>-.00001 && s<.00001 ) {	/* essentially parallel */
		to->x = x1; to->y = y1;
	    } else {
		double t1 = (x1-x2- dx2/dy2*(y1-y2))/s;
		to->x = x1 + dx1*t1;
		to->y = y1 + dy1*t1;
	    }
	}
    } else {
	pos = y/td->plength;
	if ( pos<0 ) pos=0;		/* should not happen */
	if ( pos>1 ) pos = 1;

	pos *= td->nsamples;
	low = floor(pos);
	pos -= low;

	if ( pos==0 || low==td->nsamples ) {
	    dx = td->samples[low].dx;
	    dy = td->samples[low].dy;
	    c = td->samples[low].c;
	    s = td->samples[low].s;
	} else {
	    dx = (td->samples[low].dx*(1-pos) + td->samples[low+1].dx*pos);
	    dy = (td->samples[low].dy*(1-pos) + td->samples[low+1].dy*pos);
	    c = (td->samples[low].c*(1-pos) + td->samples[low+1].c*pos);
	    s = (td->samples[low].s*(1-pos) + td->samples[low+1].s*pos);
	}

	to->x = dx + c*x;
	to->y = dy + s*x;
    }
}

static SplinePoint *TDMakePoint(TD *td,Spline *old,real t) {
    TPoint tp;
    SplinePoint *new;

    AdjustPoint(td,old,t,&tp);
    new = chunkalloc(sizeof(SplinePoint));
    new->me.x = tp.x; new->me.y = tp.y;
    new->nextcp = new->me;
    new->prevcp = new->me;
    new->nonextcp = new->noprevcp = true;
    new->nextcpdef = new->prevcpdef = false;
return( new );
}

static Spline *AdjustSpline(TD *td,Spline *old,SplinePoint *newfrom,SplinePoint *newto,
	int order2) {
    TPoint tps[15];
    int i;
    double t;

    if ( newfrom==NULL )
	newfrom = TDMakePoint(td,old,0);
    if ( newto==NULL )
	newto = TDMakePoint(td,old,1);
    for ( i=1, t=1/16.0; i<16; ++i, t+= 1/16.0 )
	AdjustPoint(td,old,t,&tps[i-1]);
return( ApproximateSplineFromPoints(newfrom,newto,tps,15, order2) );
}

static void AdjustSplineSet(TD *td,int order2) {
    SplineSet *spl, *last=NULL, *new;
    Spline *spline, *s;
    SplinePoint *lastsp, *nextsp, *sp;

    if ( td->result!=NULL )
	for ( last=td->result ; last->next!=NULL; last = last->next );

    for ( spl=td->tileset; spl!=NULL; spl=spl->next ) {
	new = chunkalloc(sizeof(SplineSet));
	if ( last==NULL )
	    td->result = new;
	else
	    last->next = new;
	last = new;
	new->first = lastsp = TDMakePoint(td,spl->first->next,0);
	nextsp = NULL;
	for ( spline=spl->first->next; spline!=NULL; spline=spline->to->next ) {
	    if ( spline->to==spl->first )
		nextsp = new->first;
	    s = AdjustSpline(td,spline,lastsp,nextsp,order2);
	    lastsp = s->to;
	    if ( nextsp!=NULL )
	break;
	}
	if ( lastsp!=new->first &&
		RealNearish(lastsp->me.x,new->first->me.x) &&
		RealNearish(lastsp->me.y,new->first->me.y) ) {
	    new->first->prev = lastsp->prev;
	    new->first->prevcp = lastsp->prevcp;
	    new->first->noprevcp = lastsp->noprevcp;
	    new->first->prevcpdef = lastsp->prevcpdef;
	    lastsp->prev->to = new->first;
	    new->last = new->first;
	    SplinePointFree(lastsp);
	} else
	    new->last = lastsp;

	for ( sp = new->first; sp!=NULL; ) {
	    SplinePointCatagorize(sp);
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==new->first )
	break;
	}
    }
}

static void TileSplineSets(TD *td,SplineSet **head,int order2) {
    SplineSet *prev=NULL, *spl, *next;

    for ( spl = *head; spl!=NULL; spl = next ) {
	next = spl->next;
	if ( td->doallpaths || PointListIsSelected(spl)) {
	    if ( prev==NULL )
		*head = next;
	    else
		prev->next = next;
	    td->path = spl;
	    if ( TDMakeSamples(td)) {
		TileLine(td);
		AdjustSplineSet(td,order2);
		free( td->samples );
		free( td->joins );
		SplinePointListsFree(td->tileset);
	    }
	    SplinePointListFree(td->path);
	    td->path = td->tileset = NULL;
	} else
	    prev = spl;
    }
    if ( *head==NULL )
	*head = td->result;
    else {
	for ( spl= *head; spl->next!=NULL; spl = spl->next );
	spl->next = td->result;
    }
}

static void TileIt(SplineSet **head,SplineSet *tile,
	enum tilepos tilepos, enum tilescale tilescale,
	int doall,int order2) {
    TD td;
    real trans[6];

    memset(&td,0,sizeof(td));
    td.tilepos = tilepos;
    td.tilescale = tilescale;
    td.doallpaths = doall;

    td.basetile = tile;
    SplineSetFindBounds(tile,&td.bb);
    trans[0] = trans[3] = 1;
    trans[1] = trans[2] = 0;
    trans[5] = td.bb.miny;
    trans[4] = -td.bb.minx;
    if ( tilepos==tp_center )
	trans[4] -= (td.bb.maxx-td.bb.minx)/2;
    else if ( tilepos==tp_left )
	trans[4] = -td.bb.maxx;
    if ( trans[4]!=0 || trans[5]!=0 )
	SplinePointListTransform(tile,trans,true);
    SplineSetFindBounds(tile,&td.bb);

    TileSplineSets(&td,head,order2);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static enum tilepos tilepos=tp_center;
static enum tilescale tilescale=ts_tilescale;

#define CID_Center	1001
#define CID_Left	1002
#define CID_Right	1003
#define	CID_Tile	1011
#define CID_TileScale	1012
#define CID_Scale	1013

struct tiledlg {
    int done;
    int cancelled;
};

static int TD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct tiledlg *d = GDrawGetUserData(gw);
	d->done = true;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Center)) )
	    tilepos = tp_center;
	else if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Left)) )
	    tilepos = tp_left;
	else
	    tilepos = tp_right;
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Tile)) )
	    tilescale = ts_tile;
	else if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_TileScale)) )
	    tilescale = ts_tilescale;
	else
	    tilescale = ts_scale;
    }
return( true );
}

static int TD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct tiledlg *d = GDrawGetUserData(gw);
	d->done = d->cancelled = true;
    }
return( true );
}

static int td_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct tiledlg *d = GDrawGetUserData(gw);
	d->done = d->cancelled = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("tilepath.html");
return( true );
	}
return( false );
    }
return( true );
}

static int TileAsk(void) {
    struct tiledlg d;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10];
    GTextInfo label[10];

    memset(&d,0,sizeof(d));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Tile Path...");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(220));
    pos.height = GDrawPointsToPixels(NULL,92);
    gw = GDrawCreateTopWindow(NULL,&pos,td_e_h,&d,&wattrs);

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    gcd[0].gd.pos.x = 6; gcd[0].gd.pos.y = 6;
    gcd[0].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[0].gd.mnemonic = 'L';
    label[0].text = (unichar_t *) _("_Left");
    label[0].text_is_1byte = true;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.popup_msg = (unichar_t *) _("The tile (in the clipboard) should be placed to the left of the path\nas the path is traced from its start point to its end");
    gcd[0].gd.cid = CID_Left;
    gcd[0].creator = GRadioCreate;

    gcd[1].gd.pos.x = 60; gcd[1].gd.pos.y = gcd[0].gd.pos.y;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[1].gd.mnemonic = 'C';
    label[1].text = (unichar_t *) _("C_enter");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.popup_msg = (unichar_t *) _("The tile (in the clipboard) should be centered on the path");
    gcd[1].gd.cid = CID_Center;
    gcd[1].creator = GRadioCreate;

    gcd[2].gd.pos.x = 140; gcd[2].gd.pos.y = gcd[1].gd.pos.y;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[2].gd.mnemonic = 'R';
    label[2].text = (unichar_t *) _("_Right");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.popup_msg = (unichar_t *) _("The tile (in the clipboard) should be placed to the right of the path\nas the path is traced from its start point to its end");
    gcd[2].gd.cid = CID_Right;
    gcd[2].creator = GRadioCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = GDrawPointsToPixels(NULL,gcd[2].gd.pos.y+20);
    gcd[3].gd.pos.width = pos.width-10;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels ;
    gcd[3].creator = GLineCreate;

    gcd[4].gd.pos.x = gcd[0].gd.pos.x; gcd[4].gd.pos.y = gcd[2].gd.pos.y+24;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[4].gd.mnemonic = 'T';
    label[4].text = (unichar_t *) _("_Tile");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.popup_msg = (unichar_t *) _("Multiple copies of the selection should be tiled onto the path");
    gcd[4].gd.cid = CID_Tile;
    gcd[4].creator = GRadioCreate;

    gcd[5].gd.pos.x = gcd[1].gd.pos.x; gcd[5].gd.pos.y = gcd[4].gd.pos.y;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[5].gd.mnemonic = 'a';
    label[5].text = (unichar_t *) _("Sc_ale & Tile");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.popup_msg = (unichar_t *) _("An integral number of the selection will be used to cover the path.\nIf the path length is not evenly divisible by the selection's\nheight, then the selection should be scaled slightly.");
    gcd[5].gd.cid = CID_TileScale;
    gcd[5].creator = GRadioCreate;

    gcd[6].gd.pos.x = gcd[2].gd.pos.x; gcd[6].gd.pos.y = gcd[5].gd.pos.y;
    gcd[6].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[6].gd.mnemonic = 'S';
    label[6].text = (unichar_t *) _("_Scale");
    label[6].text_is_1byte = true;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.popup_msg = (unichar_t *) _("The selection should be scaled so that it will cover the path's length");
    gcd[6].gd.cid = CID_Scale;
    gcd[6].creator = GRadioCreate;

    gcd[7].gd.pos.x = 20-3; gcd[7].gd.pos.y = gcd[6].gd.pos.y+26;
    gcd[7].gd.pos.width = -1; gcd[7].gd.pos.height = 0;
    gcd[7].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[7].text = (unichar_t *) _("_OK");
    label[7].text_is_1byte = true;
    label[7].text_in_resource = true;
    gcd[7].gd.mnemonic = 'O';
    gcd[7].gd.label = &label[7];
    gcd[7].gd.handle_controlevent = TD_OK;
    gcd[7].creator = GButtonCreate;

    gcd[8].gd.pos.x = -20; gcd[8].gd.pos.y = gcd[7].gd.pos.y+3;
    gcd[8].gd.pos.width = -1; gcd[8].gd.pos.height = 0;
    gcd[8].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[8].text = (unichar_t *) _("_Cancel");
    label[8].text_is_1byte = true;
    label[8].text_in_resource = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.mnemonic = 'C';
    gcd[8].gd.handle_controlevent = TD_Cancel;
    gcd[8].creator = GButtonCreate;

    gcd[0+tilepos].gd.flags |= gg_cb_on;
    gcd[4+tilescale].gd.flags |= gg_cb_on;

    GGadgetsCreate(gw,gcd);
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return( !d.cancelled );
}

void CVTile(CharView *cv) {
    SplineSet *tile = ClipBoardToSplineSet();
    int anypoints, anyrefs, anyimages, anyattach;

    if ( tile==NULL )
return;

    CVAnySel(cv,&anypoints,&anyrefs,&anyimages,&anyattach);
    if ( anyrefs || anyimages || anyattach )
return;

    if ( !TileAsk())
return;

    tile = SplinePointListCopy(tile);
    CVPreserveState(cv);
    TileIt(&cv->layerheads[cv->drawmode]->splines,tile, tilepos,tilescale, !anypoints,cv->sc->parent->order2);
    CVCharChangedUpdate(cv);
    SplinePointListsFree(tile);
    cv->lastselpt = NULL;
}

void SCTile(SplineChar *sc) {
    SplineSet *tile = ClipBoardToSplineSet();

    if ( tile==NULL )
return;

    if ( sc->layers[ly_fore].splines==NULL )
return;

    if ( !TileAsk())
return;

    tile = SplinePointListCopy(tile);
    SCPreserveState(sc,false);
    TileIt(&sc->layers[ly_fore].splines,tile, tilepos,tilescale, true, sc->parent->order2);
    SCCharChangedUpdate(sc);
    SplinePointListsFree(tile);
}

void FVTile(FontView *fv) {
    SplineSet *tile = ClipBoardToSplineSet();
    SplineChar *sc;
    int i, gid;

    if ( tile==NULL )
return;

    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		(sc=fv->sf->glyphs[gid])!=NULL && sc->layers[ly_fore].splines!=NULL )
    break;
    if ( i==fv->map->enccount )
return;

    if ( !TileAsk())
return;

    tile = SplinePointListCopy(tile);
    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i )
	if ( fv->selected[i] && (gid=fv->map->map[i])!=-1 &&
		(sc=fv->sf->glyphs[gid])!=NULL && !sc->ticked &&
		sc->layers[ly_fore].splines!=NULL ) {
	    sc->ticked = true;
	    SCPreserveState(sc,false);
	    TileIt(&sc->layers[ly_fore].splines,tile, tilepos,tilescale, true, fv->sf->order2);
	    SCCharChangedUpdate(sc);
	}
    SplinePointListsFree(tile);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
#endif 		/* FONTFORGE_CONFIG_TILEPATH */
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
