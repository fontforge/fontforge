/* Copyright (C) 2002-2012 by George Williams */
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
#include "fontforgeui.h"
#include "splineutil.h"
#include "splineutil2.h"
#include <math.h>
#include <gkeysym.h>

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
    SplineSet *firsttile;
    SplineSet *lasttile;
    SplineSet *isolatedtile;
    SplineSet *tileset;		/* As many copies of the basetile as we are */
				/*  going to need. Each successive one bb.maxy */
			        /*  higher than the last */
    SplineSet *result;		/* Final result after transformation */
    DBounds bb, fbb, lbb, ibb;	/* Of the basetile, first & last tiles */
    uint8 include_white, finclude_white, linclude_white, iinclude_white;
    bigreal xscale[3];

    SplineSet *path;
    bigreal plength;		/* Length of path */
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

    BasePoint patternSize;		/* Last 3 fields used only for patterns */
    IPoint repeatCnt;			/*  And not for tilepath */
    Layer *pattern;
} TD;
enum whitespace_type { ws_include=0x1, ws_but_not_first=0x2 };

static int TDMakeSamples(TD *td) {
    Spline *spline, *first;
    bigreal len, slen, sofar, t, toff, dt_per_sample;
    int i,end, base, pcnt;
    bigreal sx, sy, angle;

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
    td->samples = malloc((td->nsamples+1)*sizeof(struct tdsample));
    td->joins = malloc(td->pcnt*sizeof(struct jsample));

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
		spline->to->next!=NULL &&
		!((spline->to->pointtype==pt_curve && !spline->to->nonextcp && !spline->to->noprevcp) ||
		  (spline->to->pointtype==pt_hvcurve && !spline->to->nonextcp && !spline->to->noprevcp) ||
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
    bigreal len;
    bigreal ts[3];

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
			  CubicSolve(&tsp->splines[1],len,ts) ) {
		    SplinePoint *mid = SplineBisect(tsp,ts[0]);
		    tsp = mid->next;
		}
		if ( tsp->to == spl->first )
	    break;
	    }
	}
    }
}

static void SplineSplitAtY(Spline *spline,real y) {
    bigreal ts[3];
    SplinePoint *last;

    if ( spline->from->me.y<=y && spline->from->nextcp.y<=y &&
	    spline->to->me.y<=y && spline->to->prevcp.y<=y )
return;
    if ( spline->from->me.y>=y && spline->from->nextcp.y>=y &&
	    spline->to->me.y>=y && spline->to->prevcp.y>=y )
return;


    if ( !CubicSolve(&spline->splines[1],y,ts) )
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

#define Round_Up_At	.5
static void TileLine(TD *td) {
    int tilecnt=1, i;
    bigreal scale=1, y;
    real trans[6];
    SplineSet *new;
    int use_first=false, use_last=false, use_isolated=false;

    switch ( td->tilescale ) {
      case ts_tile:
	if ( td->path->first->prev!=NULL )	/* Closed contours have no ends => all tiles intermediate */
	    tilecnt = ceil( td->plength/td->bb.maxy );
	else if ( td->plength<=td->ibb.maxy ) {
	    tilecnt = 1;
	    use_isolated = true;
	} else if ( td->plength<=td->fbb.maxy ) {
	    tilecnt = 1;
	    use_first = true;
	} else if ( td->plength<=td->fbb.maxy+td->lbb.maxy && td->firsttile!=NULL && td->lasttile!=NULL ) {
	    tilecnt = 2;
	    use_first = use_last = true;
	} else {
	    use_first = (td->firsttile!=NULL);
	    use_last = (td->lasttile!=NULL);
	    tilecnt = use_first+use_last+ceil( (td->plength-td->fbb.maxy-td->lbb.maxy)/td->bb.maxy );
	}
      break;
      case ts_scale:
	if ( td->isolatedtile!=NULL ) {
	    use_isolated = true;
	    scale = td->plength/td->ibb.maxy;
	} else
	    scale = td->plength/td->bb.maxy;
      break;
      case ts_tilescale:
	tilecnt = -1;
	if ( td->path->first->prev!=NULL )	/* Closed contours have no ends => all tiles intermediate */
	    scale = td->plength/td->bb.maxy;
	else if ( td->isolatedtile!=NULL &&
		(( td->firsttile!=NULL && td->lasttile!=NULL && td->plength<td->fbb.maxy+Round_Up_At*td->lbb.maxy) ||
		 ( td->firsttile!=NULL && td->lasttile==NULL && td->plength<td->fbb.maxy+Round_Up_At*td->bb.maxy) ||
		 ( td->firsttile==NULL && td->lasttile==NULL && td->plength<(1+Round_Up_At)*td->bb.maxy)) ) {
	    use_isolated = true;
	    scale = td->plength/td->ibb.maxy;
	    tilecnt = 1;
	} else if ( td->firsttile!=NULL && td->lasttile!=NULL ) {
	    if ( td->plength<td->fbb.maxy+Round_Up_At*td->lbb.maxy ) {
		use_first = true;
		tilecnt = 1;
		scale = td->plength/td->fbb.maxy;
	    } else if ( td->plength<td->fbb.maxy+td->lbb.maxy+Round_Up_At*td->bb.maxy ) {
		use_first = use_last = true;
		tilecnt = 2;
		scale = 2*td->plength/(td->fbb.maxy+td->lbb.maxy);
	    } else {
		use_first = use_last = true;
		scale = 2 + (td->plength-td->fbb.maxy-td->lbb.maxy)/td->bb.maxy;
	    }
	} else if ( td->firsttile!=NULL ) {
	    if ( td->plength<td->fbb.maxy+Round_Up_At*td->bb.maxy ) {
		use_first = true;
		tilecnt = 1;
		scale = td->plength/td->fbb.maxy;
	    } else {
		use_first = true;
		scale = 1 + (td->plength-td->fbb.maxy)/td->bb.maxy;
	    }
	} else if ( td->lasttile!=NULL ) {
	    if ( td->plength<td->lbb.maxy+Round_Up_At*td->bb.maxy ) {
		use_last = true;
		tilecnt = 1;
		scale = td->plength/td->lbb.maxy;
	    } else {
		use_last = true;
		scale = 1 + (td->plength-td->lbb.maxy)/td->bb.maxy;
	    }
	} else
	    scale = td->plength/td->bb.maxy;
	if ( tilecnt == -1 ) {
	    tilecnt = floor( scale );
	    if ( tilecnt==0 )
		tilecnt = 1;
	    else if ( scale-tilecnt>Round_Up_At )
		++tilecnt;
	    scale = td->plength/(use_first*td->fbb.maxy + use_last*td->lbb.maxy +
			(tilecnt-use_first-use_last)*td->bb.maxy);
	}
      break;
    }

    trans[0] = 1; trans[3] = scale;		/* Only scale y */
    trans[1] = trans[2] = trans[4] = trans[5] = 0;
    y = 0;
    for ( i=0; i<tilecnt; ++i ) {
	int which = (i==0 && use_first) ? 1 :
		    (i==0 && use_isolated ) ? 3 :
		    (i==tilecnt-1 && use_last ) ? 2 :
			    0;
	new = SplinePointListCopy((&td->basetile)[which]);
	trans[5] = y;
	new = SplinePointListTransform(new,trans,tpt_AllPoints);
	if ( i==tilecnt-1 && td->tilescale==ts_tile )
	    new = SplinePointListTruncateAtY(new,td->plength);
	td->tileset = SplinePointListMerge(td->tileset,new);
	y += (&td->bb)[which].maxy*scale;
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

static void AdjustPoint(TD *td,Spline *spline,bigreal t,TPoint *to) {
    bigreal x, y;
    bigreal pos;
    int low;
    bigreal dx, dy, c, s;
    int i;

    to->t = t;

    x = ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t + spline->splines[0].d;
    y = ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t + spline->splines[1].d;

    for ( i=td->pcnt-2; i>=0; --i )
	if ( RealNearish(y,td->joins[i].sofar) )
    break;
    if ( i>=0 ) {
	bigreal x1,y1, x2, y2, dx1, dx2, dy1, dy2;
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
	    bigreal s=(dy1*dx2/dy2-dx1);
	    if ( s>-.00001 && s<.00001 ) {	/* essentially parallel */
		to->x = x1; to->y = y1;
	    } else {
		bigreal t1 = (x1-x2- dx2/dy2*(y1-y2))/s;
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
    bigreal t;

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
	    SplinePointCategorize(sp);
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
    SPLCategorizePoints(td->result);
    if ( *head==NULL )
	*head = td->result;
    else {
	for ( spl= *head; spl->next!=NULL; spl = spl->next );
	spl->next = td->result;
    }
}

static void TileIt(SplineSet **head,struct tiledata *td,
	int doall,int order2) {
    real trans[6];
    int i;
    SplineSet *thistile;

    td->doallpaths = doall;

    trans[0] = trans[3] = 1;
    trans[1] = trans[2] = 0;
    for ( i=0; i<4; ++i ) if ( (thistile = (&td->basetile)[i])!=NULL ) {
	DBounds *bb = &(&td->bb)[i];
	SplineSetFindBounds(thistile,bb);
	trans[5] = (&td->include_white)[i]&ws_include ? 0 : -bb->miny;
	trans[4] = -bb->minx;
	if ( td->tilepos==tp_center )
	    trans[4] -= (bb->maxx-bb->minx)/2;
	else if ( td->tilepos==tp_left )
	    trans[4] = -bb->maxx;
	if ( trans[4]!=0 || trans[5]!=0 )
	    SplinePointListTransform(thistile,trans,tpt_AllPoints);
	SplineSetFindBounds(thistile,bb);
    }
    td->tileset = td->result = NULL;

    TileSplineSets(td,head,order2);
}

static enum tilepos tilepos=tp_center;
static enum tilescale tilescale=ts_tilescale;
static int include_whitespace[4] = {0,0,0,0};
static SplineSet *last_tiles[4];

#define CID_Center	1001
#define CID_Left	1002
#define CID_Right	1003
#define	CID_Tile	1011
#define CID_TileScale	1012
#define CID_Scale	1013
#define CID_IncludeWhiteSpaceBelowTile	1021	/* +[0...3] */
#define CID_FirstTile	1025			/* +[0...3] for the other tiles */

static void TPDSubResize(TilePathDlg *tpd, GEvent *event) {
    int width, height;
    int i;

    if ( !event->u.resize.sized )
return;

    width = event->u.resize.size.width;
    height = event->u.resize.size.height;
    if ( width!=tpd->cv_width || height!=tpd->cv_height ) {
	tpd->cv_width = width; tpd->cv_height = height;
	for ( i=0; i<4; ++i ) {
	    CharView *cv = (&tpd->cv_first)+i;
	    GDrawResize(cv->gw,width,height);
	}
    }

    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
}

static char *tilenames[] = { N_("First"), N_("Medial"), N_("Final"), N_("Isolated") };
static void TPDDraw(TilePathDlg *tpd, GWindow pixmap, GEvent *event) {
    GRect r,pos;
    int i;

    GDrawSetLineWidth(pixmap,0);
    for ( i=0; i<4; ++i ) {
	CharView *cv = (&tpd->cv_first)+i;

	GGadgetGetSize(GWidgetGetControl(tpd->gw,CID_FirstTile+i),&pos);
	r.x = pos.x; r.y = pos.y-1;
	r.width = pos.width+1; r.height = pos.height+1;
	GDrawDrawRect(pixmap,&r,0);

	GDrawSetFont(pixmap,cv->inactive ? tpd->plain : tpd->bold);
	GDrawDrawText8(pixmap,r.x,pos.y-2-tpd->fh+tpd->as,_(tilenames[i]),-1,0);
    }
}

static void TPDMakeActive(TilePathDlg *tpd,CharView *cv) {
    int i;

    if ( tpd==NULL )
return;
    for ( i=0; i<4; ++i )
	(&tpd->cv_first)[i].inactive = true;
    cv->inactive = false;
    GDrawSetUserData(tpd->gw,cv);
    for ( i=0; i<4; ++i )
	GDrawRequestExpose((&tpd->cv_first)[i].v,NULL,false);
    GDrawRequestExpose(tpd->gw,NULL,false);
}

static void TPDChar(TilePathDlg *tpd, GEvent *event) {
    int i;
    for ( i=0; i<4; ++i )
	if ( !(&tpd->cv_first)[i].inactive )
    break;

    if ( event->u.chr.keysym==GK_Tab || event->u.chr.keysym==GK_BackTab ) {
	if ( event->u.chr.keysym==GK_Tab ) ++i; else --i;
	if ( i<0 ) i=3; else if ( i>3 ) i = 0;
	TPDMakeActive(tpd,(&tpd->cv_first)+i);
    } else
	CVChar((&tpd->cv_first)+i,event);
}

static void TPD_DoClose(struct cvcontainer *cvc) {
    TilePathDlg *tpd = (TilePathDlg *) cvc;
    int i;

    for ( i=0; i<4; ++i ) {
	SplineChar *msc = &(&tpd->sc_first)[i];
	SplinePointListsFree(msc->layers[0].splines);
	SplinePointListsFree(msc->layers[1].splines);
	free( msc->layers );
    }

    tpd->done = true;
}

static int tpd_sub_e_h(GWindow gw, GEvent *event) {
    TilePathDlg *tpd;

    if ( event->type==et_destroy )
return( true );

    tpd = (TilePathDlg *) ((CharViewBase *) GDrawGetUserData(gw))->container;

    switch ( event->type ) {
      case et_resize:
	if ( event->u.resize.sized )
	    TPDSubResize(tpd,event);
      break;
      case et_char:
	TPDChar(tpd,event);
      break;
    }
return( true );
}

static int tpd_e_h(GWindow gw, GEvent *event) {
    TilePathDlg *tpd;
    int i;

    if ( event->type == et_destroy )
return( true );

    tpd = (TilePathDlg *) ((CharViewBase *) GDrawGetUserData(gw))->container;
    switch ( event->type ) {
      case et_expose:
	TPDDraw(tpd, gw, event);
      break;
      case et_char:
	TPDChar(tpd,event);
      break;
      case et_close:
	TPD_DoClose((struct cvcontainer *) tpd);
      break;
      case et_create:
      break;
      case et_map:
	for ( i=0; i<4; ++i ) {
	    CharView *cv = (&tpd->cv_first)+i;
	    if ( !cv->inactive ) {
		if ( event->u.map.is_visible )
		    CVPaletteActivate(cv);
		else
		    CVPalettesHideIfMine(cv);
	break;
	    }
	}
	/* tpd->isvisible = event->u.map.is_visible; */
      break;
    }
return( true );
}

static int TilePathD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	TilePathDlg *tpd = (TilePathDlg *) (((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
	TPD_DoClose(&tpd->base);
    }
return( true );
}

static int TPD_Useless(SplineSet *ss) {
    DBounds bb;

    if ( ss==NULL )
return( true );
    SplineSetFindBounds(ss,&bb);
return( bb.maxy==bb.miny );
}

static int TilePathD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	TilePathDlg *tpd = (TilePathDlg *) (((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
	struct tiledata *td = tpd->td;

	if ( GGadgetIsChecked(GWidgetGetControl(tpd->gw,CID_Center)) )
	    td->tilepos = tp_center;
	else if ( GGadgetIsChecked(GWidgetGetControl(tpd->gw,CID_Left)) )
	    td->tilepos = tp_left;
	else
	    td->tilepos = tp_right;
	if ( GGadgetIsChecked(GWidgetGetControl(tpd->gw,CID_Tile)) )
	    td->tilescale = ts_tile;
	else if ( GGadgetIsChecked(GWidgetGetControl(tpd->gw,CID_TileScale)) )
	    td->tilescale = ts_tilescale;
	else
	    td->tilescale = ts_scale;
	if ( TPD_Useless(tpd->sc_medial.layers[ly_fore].splines) &&
		(td->tilescale!=ts_scale ||
		 TPD_Useless(tpd->sc_isolated.layers[ly_fore].splines)) ) {
	    if ( td->tilescale == ts_scale )
		ff_post_error(_("Bad Tile"),_("You must specify an isolated (or medial) tile"));
	    else
		ff_post_error(_("Bad Tile"),_("You must specify a medial tile"));
return( true );
	}

	tilepos = td->tilepos;
	tilescale = td->tilescale;

	td->firsttile = tpd->sc_first.layers[ly_fore].splines;
	    tpd->sc_first.layers[ly_fore].splines = NULL;
	include_whitespace[0] = td->finclude_white = GGadgetIsChecked(GWidgetGetControl(tpd->gw,CID_IncludeWhiteSpaceBelowTile+0))?ws_include:0;
	td->basetile = tpd->sc_medial.layers[ly_fore].splines;
	    tpd->sc_medial.layers[ly_fore].splines = NULL;
	include_whitespace[1] = td->include_white = GGadgetIsChecked(GWidgetGetControl(tpd->gw,CID_IncludeWhiteSpaceBelowTile+1))?ws_include:0;
	td->lasttile = tpd->sc_final.layers[ly_fore].splines;
	    tpd->sc_final.layers[ly_fore].splines = NULL;
	include_whitespace[2] = td->linclude_white = GGadgetIsChecked(GWidgetGetControl(tpd->gw,CID_IncludeWhiteSpaceBelowTile+2))?ws_include:0;
	td->isolatedtile = tpd->sc_isolated.layers[ly_fore].splines;
	    tpd->sc_isolated.layers[ly_fore].splines = NULL;
	include_whitespace[3] = td->iinclude_white = GGadgetIsChecked(GWidgetGetControl(tpd->gw,CID_IncludeWhiteSpaceBelowTile+3))?ws_include:0;

	TPD_DoClose(&tpd->base);
	tpd->oked = true;
    }
return( true );
}

static int TPD_Can_Navigate(struct cvcontainer *cvc, enum nav_type type) {
return( false );
}

static int TPD_Can_Open(struct cvcontainer *cvc) {
return( false );
}

static SplineFont *SF_Of_TPD(struct cvcontainer *foo) {
return( NULL );
}

struct cvcontainer_funcs tilepath_funcs = {
    cvc_tilepath,
    (void (*) (struct cvcontainer *cvc,CharViewBase *cv)) TPDMakeActive,
    (void (*) (struct cvcontainer *cvc,void *)) TPDChar,
    TPD_Can_Navigate,
    NULL,
    TPD_Can_Open,
    TPD_DoClose,
    SF_Of_TPD
};


static void TPDInit(TilePathDlg *tpd,SplineFont *sf) {
    int i;

    memset(tpd,0,sizeof(*tpd));
    tpd->base.funcs = &tilepath_funcs;
    tpd->base_sf = sf;

    for ( i=0; i<4; ++i ) {
	SplineChar *msc = &(&tpd->sc_first)[i];
	CharView *mcv = &(&tpd->cv_first)[i];
	msc->orig_pos = i;
	msc->unicodeenc = -1;
	msc->name = i==0 ? "First" :
		    i==1 ? "Medial"  :
		    i==2 ? "Last":
			    "Isolated";
	msc->parent = &tpd->dummy_sf;
	msc->layer_cnt = 2;
	msc->layers = calloc(2,sizeof(Layer));
	LayerDefault(&msc->layers[0]);
	LayerDefault(&msc->layers[1]);
	tpd->chars[i] = msc;

	mcv->b.sc = msc;
	mcv->b.layerheads[dm_fore] = &msc->layers[ly_fore];
	mcv->b.layerheads[dm_back] = &msc->layers[ly_back];
	mcv->b.layerheads[dm_grid] = &tpd->dummy_sf.grid;
	msc->layers[ly_fore].splines = SplinePointListCopy(last_tiles[i]);
	mcv->b.drawmode = dm_fore;
	mcv->b.container = (struct cvcontainer *) tpd;
	mcv->inactive = i!=0;
    }
    tpd->dummy_sf.glyphs = tpd->chars;
    tpd->dummy_sf.glyphcnt = tpd->dummy_sf.glyphmax = 4;
    tpd->dummy_sf.pfminfo.fstype = -1;
    tpd->dummy_sf.pfminfo.stylemap = -1;
    tpd->dummy_sf.fontname = tpd->dummy_sf.fullname = tpd->dummy_sf.familyname = "dummy";
    tpd->dummy_sf.weight = "Medium";
    tpd->dummy_sf.origname = "dummy";
    tpd->dummy_sf.ascent = sf->ascent;
    tpd->dummy_sf.descent = sf->descent;
    tpd->dummy_sf.layers = tpd->layerinfo;
    tpd->dummy_sf.layer_cnt = 2;
    tpd->layerinfo[ly_back].order2 = sf->layers[ly_back].order2;
    tpd->layerinfo[ly_back].name = _("Back");
    tpd->layerinfo[ly_fore].order2 = sf->layers[ly_fore].order2;
    tpd->layerinfo[ly_fore].name = _("Fore");
    tpd->dummy_sf.grid.order2 = sf->grid.order2;
    tpd->dummy_sf.anchor = NULL;

    tpd->dummy_sf.fv = (FontViewBase *) &tpd->dummy_fv;
    tpd->dummy_fv.b.active_layer = ly_fore;
    tpd->dummy_fv.b.sf = &tpd->dummy_sf;
    tpd->dummy_fv.b.selected = tpd->sel;
    tpd->dummy_fv.cbw = tpd->dummy_fv.cbh = default_fv_font_size+1;
    tpd->dummy_fv.magnify = 1;

    tpd->dummy_fv.b.map = &tpd->dummy_map;
    tpd->dummy_map.map = tpd->map;
    tpd->dummy_map.backmap = tpd->backmap;
    tpd->dummy_map.enccount = tpd->dummy_map.encmax = tpd->dummy_map.backmax = 4;
    tpd->dummy_map.enc = &custom;
}

static int TileAsk(struct tiledata *td,SplineFont *sf) {
    TilePathDlg tpd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[24], boxes[5], *harray[8], *varray[5],
	*rhvarray[4][5], *chvarray[4][5];
    GTextInfo label[24];
    FontRequest rq;
    int as, ds, ld;
    int i,k;
    static GFont *font = NULL, *bold=NULL;

    TPDInit( &tpd,sf );
    tpd.td = td;
    memset(td,0,sizeof(*td));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_isdlg|wam_restrict|wam_undercursor|wam_utf8_wtitle;
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Tile Path");
    pos.width = 600;
    pos.height = 300;
    tpd.gw = gw = GDrawCreateTopWindow(NULL,&pos,tpd_e_h,&tpd.cv_first,&wattrs);

    if ( font==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = 12;
	rq.weight = 400;
	font = GDrawInstanciateFont(NULL,&rq);
	font = GResourceFindFont("TilePath.Font",font);

	GDrawDecomposeFont(font, &rq);
	rq.weight = 700;
	bold = GDrawInstanciateFont(NULL,&rq);
	bold = GResourceFindFont("TilePath.BoldFont",bold);
    }
    tpd.plain = font;
    tpd.bold = bold;
    GDrawWindowFontMetrics(tpd.gw,tpd.plain,&as,&ds,&ld);
    tpd.fh = as+ds; tpd.as = as;

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k = 0;
    gcd[k].gd.flags = gg_visible|gg_enabled ;		/* This space is for the menubar */
    gcd[k].gd.pos.height = 18; gcd[k].gd.pos.width = 20;
    gcd[k++].creator = GSpacerCreate;

    for ( i=0; i<4; ++i ) {
	gcd[k].gd.pos.height = tpd.fh;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k++].creator = GSpacerCreate;
	chvarray[0][i] = &gcd[k-1];

	gcd[k].gd.pos.width = gcd[k].gd.pos.height = 200;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k].gd.cid = CID_FirstTile+i;
	gcd[k].gd.u.drawable_e_h = tpd_sub_e_h;
	gcd[k++].creator = GDrawableCreate;
	chvarray[1][i] = &gcd[k-1];

	gcd[k].gd.pos.x = gcd[0].gd.pos.x; gcd[k].gd.pos.y = gcd[6].gd.pos.y+24;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	if ( include_whitespace[i] ) gcd[k].gd.flags |= gg_cb_on;
	label[k].text = (unichar_t *) _("Include Whitespace below Tile");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.popup_msg = (unichar_t *) _("Normally the Tile will consist of everything\nwithin the minimum bounding box of the tile --\nso adjacent tiles will abut directly on one\nanother. If you wish whitespace between tiles\nset this flag");
	gcd[k].gd.cid = CID_IncludeWhiteSpaceBelowTile+i;
	gcd[k++].creator = GCheckBoxCreate;
	chvarray[2][i] = &gcd[k-1];
    }
    chvarray[0][4] = chvarray[1][4] = chvarray[2][4] = chvarray[3][0] = NULL;

    gcd[k].gd.pos.x = 6; gcd[k].gd.pos.y = 6;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.mnemonic = 'L';
    label[k].text = (unichar_t *) _("_Left");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = (unichar_t *) _("The tiles should be placed to the left of the path\nas the path is traced from its start point to its end");
    gcd[k].gd.cid = CID_Left;
    gcd[k++].creator = GRadioCreate;
    rhvarray[0][0] = &gcd[k-1];

    gcd[k].gd.pos.x = 60; gcd[k].gd.pos.y = gcd[0].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.mnemonic = 'C';
    label[k].text = (unichar_t *) _("C_enter");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = (unichar_t *) _("The tiles should be centered on the path");
    gcd[k].gd.cid = CID_Center;
    gcd[k++].creator = GRadioCreate;
    rhvarray[0][1] = &gcd[k-1];

    gcd[k].gd.pos.x = 140; gcd[k].gd.pos.y = gcd[1].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.mnemonic = 'R';
    label[k].text = (unichar_t *) _("_Right");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = (unichar_t *) _("The tiles should be placed to the right of the path\nas the path is traced from its start point to its end");
    gcd[k].gd.cid = CID_Right;
    gcd[k++].creator = GRadioCreate;
    rhvarray[0][2] = &gcd[k-1];
    rhvarray[0][3] = GCD_Glue; rhvarray[0][4] = NULL;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = GDrawPointsToPixels(NULL,gcd[2].gd.pos.y+20);
    gcd[k].gd.pos.width = pos.width-10;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels ;
    gcd[k++].creator = GLineCreate;
    rhvarray[1][0] = &gcd[k-1];
    rhvarray[1][1] = rhvarray[1][2] = GCD_ColSpan;
    rhvarray[1][3] = GCD_Glue; rhvarray[1][4] = NULL;

    gcd[k].gd.pos.x = gcd[0].gd.pos.x; gcd[k].gd.pos.y = gcd[2].gd.pos.y+24;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.mnemonic = 'T';
    label[k].text = (unichar_t *) _("_Tile");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = (unichar_t *) _("Multiple copies of the selection should be tiled onto the path");
    gcd[k].gd.cid = CID_Tile;
    gcd[k++].creator = GRadioCreate;
    rhvarray[2][0] = &gcd[k-1];

    gcd[k].gd.pos.x = gcd[1].gd.pos.x; gcd[k].gd.pos.y = gcd[4].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.mnemonic = 'a';
    label[k].text = (unichar_t *) _("Sc_ale & Tile");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = (unichar_t *) _("An integral number of the selection will be used to cover the path.\nIf the path length is not evenly divisible by the selection's\nheight, then the selection should be scaled slightly.");
    gcd[k].gd.cid = CID_TileScale;
    gcd[k++].creator = GRadioCreate;
    rhvarray[2][1] = &gcd[k-1];

    gcd[k].gd.pos.x = gcd[2].gd.pos.x; gcd[k].gd.pos.y = gcd[5].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.mnemonic = 'S';
    label[k].text = (unichar_t *) _("_Scale");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.popup_msg = (unichar_t *) _("The selection should be scaled so that it will cover the path's length");
    gcd[k].gd.cid = CID_Scale;
    gcd[k++].creator = GRadioCreate;
    rhvarray[2][2] = &gcd[k-1];
    rhvarray[2][3] = GCD_Glue; rhvarray[2][4] = NULL;
    rhvarray[3][0] = NULL;

    gcd[k-7+tilepos].gd.flags |= gg_cb_on;
    gcd[k-3+tilescale].gd.flags |= gg_cb_on;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_default;
    gcd[k].gd.handle_controlevent = TilePathD_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_cancel;
    gcd[k].gd.handle_controlevent = TilePathD_Cancel;
    gcd[k++].creator = GButtonCreate;

    harray[0] = GCD_Glue; harray[1] = &gcd[k-2]; harray[2] = GCD_Glue;
    harray[3] = GCD_Glue; harray[4] = &gcd[k-1]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = chvarray[0];
    boxes[3].creator = GHVBoxCreate;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = rhvarray[0];
    boxes[4].creator = GHVBoxCreate;

    varray[0] = &gcd[0];
    varray[1] = &boxes[3];
    varray[2] = &boxes[4];
    varray[3] = &boxes[2];
    varray[4] = NULL;

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw,boxes);

    TPDCharViewInits(&tpd,CID_FirstTile);

    GHVBoxSetExpandableRow(boxes[0].ret,1);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxSetExpandableRow(boxes[3].ret,1);
    GHVBoxSetPadding(boxes[3].ret, 2, 2);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandglue);
    GGadgetResize(boxes[0].ret,pos.width,pos.height);

    TPDMakeActive(&tpd,&tpd.cv_medial);

    GDrawResize(gw,1000,400);		/* Force a resize event */

    GDrawSetVisible(tpd.gw,true);

    while ( !tpd.done )
	GDrawProcessOneEvent(NULL);

    for ( i=0; i<4; ++i ) {
	CharView *cv = &tpd.cv_first + i;
	if ( cv->backimgs!=NULL ) {
	    GDrawDestroyWindow(cv->backimgs);
	    cv->backimgs = NULL;
	}
	CVPalettesHideIfMine(cv);
    }
    GDrawDestroyWindow(tpd.gw);
return( tpd.oked );
}

static void TDFree(struct tiledata *td) {
    int i;

    for ( i=0 ; i<4; ++i )
	SplinePointListFree( last_tiles[i]);

    last_tiles[0] = td->firsttile;
    last_tiles[1] = td->basetile;
    last_tiles[2] = td->lasttile;
    last_tiles[3] = td->isolatedtile;
}

void CVTile(CharView *cv) {
    struct tiledata td;
    int anypoints, anyrefs, anyimages, anyattach;

    CVAnySel(cv,&anypoints,&anyrefs,&anyimages,&anyattach);
    if ( anyrefs || anyimages || anyattach )
return;

    if ( !TileAsk(&td,cv->b.sc->parent))
return;

    CVPreserveState((CharViewBase *) cv);
    TileIt(&cv->b.layerheads[cv->b.drawmode]->splines,&td, !anypoints,cv->b.layerheads[cv->b.drawmode]->order2);
    CVCharChangedUpdate(&cv->b);
    TDFree(&td);
    cv->lastselpt = NULL;
}

void SCTile(SplineChar *sc,int layer) {
    struct tiledata td;

    if ( sc->layers[layer].splines==NULL )
return;

    if ( !TileAsk(&td,sc->parent))
return;

    SCPreserveLayer(sc,layer,false);
    TileIt(&sc->layers[layer].splines,&td, true, sc->layers[layer].order2);
    SCCharChangedUpdate(sc,layer);
    TDFree(&td);
}

void FVTile(FontView *fv) {
    struct tiledata td;
    SplineChar *sc;
    int i, gid;
    int layer = fv->b.active_layer;

    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] && (gid=fv->b.map->map[i])!=-1 &&
		(sc=fv->b.sf->glyphs[gid])!=NULL && sc->layers[ly_fore].splines!=NULL )
    break;
    if ( i==fv->b.map->enccount )
return;

    if ( !TileAsk(&td,fv->b.sf))
return;

    SFUntickAll(fv->b.sf);
    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] && (gid=fv->b.map->map[i])!=-1 &&
		(sc=fv->b.sf->glyphs[gid])!=NULL && !sc->ticked &&
		sc->layers[layer].splines!=NULL ) {
	    sc->ticked = true;
	    SCPreserveLayer(sc,layer,false);
	    TileIt(&sc->layers[layer].splines,&td, true, fv->b.sf->layers[layer].order2);
	    SCCharChangedUpdate(sc,layer);
	}
    TDFree(&td);
}

/* ************************************************************************** */
/* ****************************** Tile Pattern ****************************** */
/* ************************************************************************** */

static void SCTilePattern(SplineChar *sc, int layer, Layer *pattern,
	BasePoint *size, IPoint *cnt) {
    Layer *ly;
    SplineSet *ss, *lastss;
    RefChar *ref, *lastref, *r;
    real transform[6];
    int i,j;

    if ( pattern==NULL || cnt->x<=0 || cnt->y<=0 || size->x<=0 || size->y<=0 ||
	    (pattern->splines==NULL && (pattern->refs==NULL || layer==ly_grid )))
return;

    SCPreserveLayer(sc,layer,false);

    if ( layer==ly_grid )
	ly = &sc->parent->grid;
    else if ( layer<0 )
	ly = &sc->layers[ly_fore];
    else
	ly = &sc->layers[layer];

    memset(transform,0,sizeof(transform));
    transform[0] = transform[3] = 1.0;

    if ( ly->splines==NULL )
	lastss = NULL;
    else
	for ( lastss=ly->splines; lastss->next!=NULL; lastss=lastss->next );
    if ( ly->refs==NULL )
	lastref = NULL;
    else
	for ( lastref=ly->refs; lastref->next!=NULL; lastref=lastref->next );

    for ( i=0; i<cnt->x; ++i ) for ( j=0; j<cnt->y; ++j ) {
	transform[4] = i*size->x; transform[5] = j*size->y;
	if ( pattern->splines!=NULL ) {
	    ss = SplinePointListTransform(SplinePointListCopy(pattern->splines),transform,tpt_AllPoints);
	    if ( lastss!=NULL )
		lastss->next = ss;
	    else
		ly->splines = ss;
	    for ( ; ss->next!=NULL; ss=ss->next );
	    lastss = ss;
	}
	if ( pattern->refs!=NULL ) {
	    ref = RefCharsCopy(pattern->refs);
	    ref->transform[4] += transform[4];
	    ref->transform[5] += transform[5];
	    if ( lastref!=NULL )
		lastref->next = ref;
	    else
		ly->refs = ref;
	    for ( r=ref; r!=NULL; r=r->next ) {
		SCMakeDependent(sc,r->sc);
		SCReinstanciateRefChar(sc,r,layer);
		lastref = r;
	    }
	}
    }
    SCCharChangedUpdate(sc,layer);
}

static SplineSet *last_pattern=NULL;
static BasePoint patternSize;
static IPoint patternRepeat = { 10, 10 };

#define	CID_Tile	1011		/* Same as above */
#define CID_PatternWidth	1012
#define CID_PatternHeight	1013
#define CID_XRepeat	1014
#define CID_YRepeat	1015

static void PTDSubResize(TilePathDlg *tpd, GEvent *event) {
    int width, height;

    if ( !event->u.resize.sized )
return;

    width = event->u.resize.size.width;
    height = event->u.resize.size.height;
    if ( width!=tpd->cv_width || height!=tpd->cv_height ) {
	tpd->cv_width = width; tpd->cv_height = height;
	 {
	    CharView *cv = &tpd->cv_first;
	    GDrawResize(cv->gw,width,height);
	}
    }

    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
}

static void PTDDraw(TilePathDlg *tpd, GWindow pixmap, GEvent *event) {
    GRect r,pos;

    GDrawSetLineWidth(pixmap,0);

    GGadgetGetSize(GWidgetGetControl(tpd->gw,CID_Tile),&pos);
    r.x = pos.x; r.y = pos.y-1;
    r.width = pos.width+1; r.height = pos.height+1;
    GDrawDrawRect(pixmap,&r,0);
}

static void PTDMakeActive(TilePathDlg *ptd,CharView *cv) {

    if ( ptd==NULL )
return;
    cv->inactive = false;
    GDrawSetUserData(ptd->gw,cv);
}

static void PTDChar(TilePathDlg *ptd, GEvent *event) {

    CVChar(&ptd->cv_first,event);
}

static void PTD_DoClose(struct cvcontainer *cvc) {
    TilePathDlg *ptd = (TilePathDlg *) cvc;
    SplineChar *msc = &ptd->sc_first;

    SplinePointListsFree(msc->layers[0].splines);
    RefCharsFree(msc->layers[0].refs);
    SplinePointListsFree(msc->layers[1].splines);
    RefCharsFree(msc->layers[1].refs);
    free( msc->layers );

    ptd->done = true;
}

static int ptd_sub_e_h(GWindow gw, GEvent *event) {
    TilePathDlg *ptd;

    if ( event->type==et_destroy )
return( true );

    ptd = (TilePathDlg *) ((CharViewBase *) GDrawGetUserData(gw))->container;

    switch ( event->type ) {
      case et_resize:
	if ( event->u.resize.sized )
	    PTDSubResize(ptd,event);
      break;
      case et_char:
	PTDChar(ptd,event);
      break;
    }
return( true );
}

static int ptd_e_h(GWindow gw, GEvent *event) {
    TilePathDlg *ptd;

    if ( event->type == et_destroy )
return( true );

    ptd = (TilePathDlg *) ((CharViewBase *) GDrawGetUserData(gw))->container;
    switch ( event->type ) {
      case et_expose:
	PTDDraw(ptd, gw, event);
      break;
      case et_char:
	PTDChar(ptd,event);
      break;
      case et_close:
	PTD_DoClose((struct cvcontainer *) ptd);
      break;
      case et_timer: {
	SplineSet *ss;
	char buffer[32];
	int err;

	for ( ss = ptd->sc_first.layers[ly_back].splines; ss!=NULL; ss=ss->next ) {
	    if ( ss->first->prev!=NULL || ss->first->next==NULL || ss->first->next->to!=ss->last )
	continue;
	    err = 0;
	    if ( ss->first->me.x==ss->last->me.x ) {
		/* Don't make the change if they are the same (or there's an */
		/*  error) as the user might be busy editing and that would */
		/*  lose his/her work */
		if ( !RealNear(GetCalmReal8(ptd->gw,CID_PatternWidth,_("Width"),&err),
			ss->first->me.x) && !err ) {
		    sprintf( buffer,"%g",ss->first->me.x);
		    GGadgetSetTitle8(GWidgetGetControl(ptd->gw,CID_PatternWidth),buffer);
		}
	    } else if ( ss->first->me.y==ss->last->me.y ) {
		if ( !RealNear(GetCalmReal8(ptd->gw,CID_PatternHeight,_("Height"),&err),
			ss->first->me.y) && !err ) {
		    sprintf( buffer,"%g",ss->first->me.y);
		    GGadgetSetTitle8(GWidgetGetControl(ptd->gw,CID_PatternHeight),buffer);
		}
	    }
	}
      } break;
      case et_map:
	{
	    CharView *cv = &ptd->cv_first;
	    if ( !cv->inactive ) {
		if ( event->u.map.is_visible )
		    CVPaletteActivate(cv);
		else
		    CVPalettesHideIfMine(cv);
	break;
	    }
	}
	/* ptd->isvisible = event->u.map.is_visible; */
      break;
    }
return( true );
}

static int PTD_RefigureBackground(GGadget *g, GEvent *e) {
    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_textchanged )) {
	TilePathDlg *ptd = (TilePathDlg *) (((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
	int hsize, vsize;
	int err = 0;
	SplinePoint *sp1, *sp2;
	SplineSet *ss;

	hsize = GetCalmReal8(ptd->gw,CID_PatternWidth,_("Width"),&err);
	vsize = GetCalmReal8(ptd->gw,CID_PatternHeight,_("Height"),&err);
	if ( err )
return( true );
	ptd->sc_first.width = hsize;
	SplinePointListFree(ptd->sc_first.layers[ly_back].splines);
	sp1 = SplinePointCreate(-1000,vsize);
	sp2 = SplinePointCreate(2000,vsize);
	SplineMake(sp1,sp2,ptd->sc_first.layers[ly_back].order2);
	ss = chunkalloc(sizeof(SplineSet));
	ss->first = sp1; ss->last = sp2;
	ptd->sc_first.layers[ly_back].splines = ss;
	sp1 = SplinePointCreate(hsize,-1000);
	sp2 = SplinePointCreate(hsize,2000);
	SplineMake(sp1,sp2,ptd->sc_first.layers[ly_back].order2);
	ss = chunkalloc(sizeof(SplineSet));
	ss->first = sp1; ss->last = sp2;
	ptd->sc_first.layers[ly_back].splines->next = ss;
	GDrawRequestExpose(ptd->cv_first.v,NULL,false);
    }
return( true );
}

static int PTD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	TilePathDlg *ptd = (TilePathDlg *) (((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
	PTD_DoClose(&ptd->base);
    }
return( true );
}

static int PTD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	TilePathDlg *ptd = (TilePathDlg *) (((CharViewBase *) GDrawGetUserData(GGadgetGetWindow(g)))->container);
	struct tiledata *td = ptd->td;
	int err = 0;

	td->patternSize.x = GetReal8(ptd->gw,CID_PatternWidth,_("Width"),&err);
	td->patternSize.y = GetReal8(ptd->gw,CID_PatternHeight,_("Height"),&err);

	td->repeatCnt.x = GetInt8(ptd->gw,CID_XRepeat,_("X Repeat Count"),&err);
	td->repeatCnt.y = GetInt8(ptd->gw,CID_YRepeat,_("Y Repeat Count"),&err);
	if ( err )
return( true );
	if ( td->patternSize.x<=0 || td->patternSize.y<=0 ) {
	    ff_post_error(_("Bad Pattern Size"),_("The pattern size (width & height) must be a positive number"));
return( true );
	}
	if ( td->repeatCnt.x<=0 || td->repeatCnt.y<=0 || td->repeatCnt.x>2000 || td->repeatCnt.y>2000 ) {
	    ff_post_error(_("Bad Pattern Size"),_("The repeat counts must be positive numbers"));
return( true );
	}

	td->pattern = calloc(1,sizeof(Layer));
	td->pattern->splines = SplinePointListCopy(ptd->sc_first.layers[ly_fore].splines);
	td->pattern->refs = RefCharsCopy(ptd->sc_first.layers[ly_fore].refs);
	
	if ( td->pattern->splines==NULL && td->pattern->refs==NULL ) {
	    ff_post_error(_("Bad Pattern"),_("You must specify a pattern"));
return( true );
	}

	PTD_DoClose(&ptd->base);
	ptd->oked = true;
    }
return( true );
}

static int PTD_Can_Navigate(struct cvcontainer *cvc, enum nav_type type) {
return( false );
}

static int PTD_Can_Open(struct cvcontainer *cvc) {
return( false );
}

static SplineFont *SF_Of_PTD(struct cvcontainer *cvc) {
    TilePathDlg *ptd = (TilePathDlg *) cvc;
return( ptd->base_sf );
}

struct cvcontainer_funcs tilepattern_funcs = {
    cvc_multiplepattern,
    (void (*) (struct cvcontainer *cvc,CharViewBase *cv)) PTDMakeActive,
    (void (*) (struct cvcontainer *cvc,void *)) PTDChar,
    PTD_Can_Navigate,
    NULL,
    PTD_Can_Open,
    PTD_DoClose,
    SF_Of_PTD
};


static void PTDInit(TilePathDlg *ptd,SplineFont *sf) {

    memset(ptd,0,sizeof(*ptd));
    ptd->base.funcs = &tilepattern_funcs;
    ptd->base_sf = sf;

    {
	SplineChar *msc = &ptd->sc_first;
	CharView *mcv = &ptd->cv_first;
	msc->orig_pos = 0;
	msc->unicodeenc = -1;
	msc->name = "Pattern";
	msc->parent = &ptd->dummy_sf;
	msc->layer_cnt = 2;
	msc->layers = calloc(2,sizeof(Layer));
	LayerDefault(&msc->layers[0]);
	LayerDefault(&msc->layers[1]);
	ptd->chars[0] = msc;

	mcv->b.sc = msc;
	mcv->b.layerheads[dm_fore] = &msc->layers[ly_fore];
	mcv->b.layerheads[dm_back] = &msc->layers[ly_back];
	mcv->b.layerheads[dm_grid] = &ptd->dummy_sf.grid;
	msc->layers[ly_fore].splines = SplinePointListCopy(last_pattern);
	mcv->b.drawmode = dm_fore;
	mcv->b.container = (struct cvcontainer *) ptd;
	mcv->inactive = false;
    }
    ptd->dummy_sf.glyphs = ptd->chars;
    ptd->dummy_sf.glyphcnt = ptd->dummy_sf.glyphmax = 1;
    ptd->dummy_sf.pfminfo.fstype = -1;
    ptd->dummy_sf.pfminfo.stylemap = -1;
    ptd->dummy_sf.fontname = ptd->dummy_sf.fullname = ptd->dummy_sf.familyname = "dummy";
    ptd->dummy_sf.weight = "Medium";
    ptd->dummy_sf.origname = "dummy";
    ptd->dummy_sf.ascent = sf->ascent;
    ptd->dummy_sf.descent = sf->descent;
    ptd->dummy_sf.layers = ptd->layerinfo;
    ptd->dummy_sf.layer_cnt = 2;
    ptd->layerinfo[ly_back].order2 = sf->layers[ly_back].order2;
    ptd->layerinfo[ly_back].name = _("Size");
    ptd->layerinfo[ly_fore].order2 = sf->layers[ly_fore].order2;
    ptd->layerinfo[ly_fore].name = _("Pattern");
    ptd->dummy_sf.grid.order2 = sf->grid.order2;
    ptd->dummy_sf.anchor = NULL;

    ptd->dummy_sf.fv = (FontViewBase *) &ptd->dummy_fv;
    ptd->dummy_fv.b.active_layer = ly_fore;
    ptd->dummy_fv.b.sf = &ptd->dummy_sf;
    ptd->dummy_fv.b.selected = ptd->sel;
    ptd->dummy_fv.cbw = ptd->dummy_fv.cbh = default_fv_font_size+1;
    ptd->dummy_fv.magnify = 1;

    ptd->dummy_fv.b.map = &ptd->dummy_map;
    ptd->dummy_map.map = ptd->map;
    ptd->dummy_map.backmap = ptd->backmap;
    ptd->dummy_map.enccount = ptd->dummy_map.encmax = ptd->dummy_map.backmax = 1;
    ptd->dummy_map.enc = &custom;
}

static int TilePatternAsk(struct tiledata *td,SplineFont *sf) {
    TilePathDlg ptd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[24], boxes[5], *harray1[8], *harray2[8], *varray[10],
	*harray[8];
    GTextInfo label[24];
    int k;
    char width[30], height[30], xr[30], yr[30];

    PTDInit( &ptd,sf );
    ptd.td = td;
    memset(td,0,sizeof(*td));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_isdlg|wam_restrict|wam_undercursor|wam_utf8_wtitle;
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Tile Pattern");
    pos.width = 600;
    pos.height = 300;
    ptd.gw = gw = GDrawCreateTopWindow(NULL,&pos,ptd_e_h,&ptd.cv_first,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k = 0;
    gcd[k].gd.flags = gg_visible|gg_enabled ;		/* This space is for the menubar */
    gcd[k].gd.pos.height = 18; gcd[k].gd.pos.width = 20;
    gcd[k++].creator = GSpacerCreate;
    varray[0] = &gcd[k-1];

    gcd[k].gd.pos.width = gcd[k].gd.pos.height = 200;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Tile;
    gcd[k].gd.u.drawable_e_h = ptd_sub_e_h;
    gcd[k++].creator = GDrawableCreate;
    varray[1] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible | gg_enabled;
    label[k].text = (unichar_t *) _("Pattern Size:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GLabelCreate;
    varray[2] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible | gg_enabled;
    label[k].text = (unichar_t *) _("Width:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GLabelCreate;
    harray1[0] = &gcd[k-1];

    if ( last_pattern==NULL )
	sprintf( width, "%d", sf->ascent+sf->descent );
    else
	sprintf( width, "%g", patternSize.x );
    gcd[k].gd.flags = gg_visible | gg_enabled;
    label[k].text = (unichar_t *) width;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PatternWidth;
    gcd[k].gd.handle_controlevent = PTD_RefigureBackground;
    gcd[k++].creator = GTextFieldCreate;
    harray1[1] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible | gg_enabled;
    label[k].text = (unichar_t *) _("Height:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GLabelCreate;
    harray1[2] = &gcd[k-1];

    if ( last_pattern==NULL )
	sprintf( height, "%d", sf->ascent+sf->descent );
    else
	sprintf( height, "%g", patternSize.y );
    gcd[k].gd.flags = gg_visible | gg_enabled;
    label[k].text = (unichar_t *) height;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_PatternHeight;
    gcd[k].gd.handle_controlevent = PTD_RefigureBackground;
    gcd[k++].creator = GTextFieldCreate;
    harray1[3] = &gcd[k-1]; harray1[4] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray1;
    boxes[2].creator = GHBoxCreate;
    varray[3] = &boxes[2];

    gcd[k].gd.flags = gg_visible | gg_enabled;
    label[k].text = (unichar_t *) _("Repeat Counts:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GLabelCreate;
    varray[4] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible | gg_enabled;
    label[k].text = (unichar_t *) _("X:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GLabelCreate;
    harray2[0] = &gcd[k-1];

    sprintf( xr, "%d", patternRepeat.x );
    gcd[k].gd.flags = gg_visible | gg_enabled;
    label[k].text = (unichar_t *) xr;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_XRepeat;
    gcd[k++].creator = GTextFieldCreate;
    harray2[1] = &gcd[k-1];

    gcd[k].gd.flags = gg_visible | gg_enabled;
    label[k].text = (unichar_t *) _("Y:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GLabelCreate;
    harray2[2] = &gcd[k-1];

    sprintf( yr, "%d", patternRepeat.y );
    gcd[k].gd.flags = gg_visible | gg_enabled;
    label[k].text = (unichar_t *) yr;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_YRepeat;
    gcd[k++].creator = GTextFieldCreate;
    harray2[3] = &gcd[k-1]; harray2[4] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray2;
    boxes[3].creator = GHBoxCreate;
    varray[5] = &boxes[3];


    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_default;
    gcd[k].gd.handle_controlevent = PTD_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled|gg_visible|gg_but_cancel;
    gcd[k].gd.handle_controlevent = PTD_Cancel;
    gcd[k++].creator = GButtonCreate;

    harray[0] = GCD_Glue; harray[1] = &gcd[k-2]; harray[2] = GCD_Glue;
    harray[3] = GCD_Glue; harray[4] = &gcd[k-1]; harray[5] = GCD_Glue;
    harray[6] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = harray;
    boxes[4].creator = GHBoxCreate;
    varray[6] = &boxes[4]; varray[7] = NULL;

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw,boxes);

    PTDCharViewInits(&ptd,CID_Tile);

    GHVBoxSetExpandableRow(boxes[0].ret,1);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
    GGadgetResize(boxes[0].ret,pos.width,pos.height);

    PTD_RefigureBackground(GWidgetGetControl(ptd.gw,CID_PatternWidth),NULL);

    PTDMakeActive(&ptd,&ptd.cv_first);

    GDrawResize(gw,400,400);		/* Force a resize event */

    GDrawSetVisible(ptd.gw,true);
    GDrawRequestTimer(ptd.gw,1000,1000,NULL);

    while ( !ptd.done )
	GDrawProcessOneEvent(NULL);

    {
	CharView *cv = &ptd.cv_first;
	if ( cv->backimgs!=NULL ) {
	    GDrawDestroyWindow(cv->backimgs);
	    cv->backimgs = NULL;
	}
	CVPalettesHideIfMine(cv);
    }
    GDrawDestroyWindow(ptd.gw);
return( ptd.oked );
}

static void TPFree(struct tiledata *td) {

    SplinePointListFree(last_pattern);
    last_pattern = td->pattern->splines;
    RefCharsFree(td->pattern->refs);
    free(td->pattern);
    patternSize = td->patternSize;
    patternRepeat = td->repeatCnt;
}

void CVPatternTile(CharView *cv) {
    struct tiledata td;

    if ( !TilePatternAsk(&td,cv->b.sc->parent))
return;

    SCTilePattern(cv->b.sc,CVLayer((CharViewBase *) cv),td.pattern, &td.patternSize, &td.repeatCnt);
    TPFree(&td);
    cv->lastselpt = NULL;
}

void FVPatternTile(FontView *fv) {
    struct tiledata td;
    SplineChar *sc;
    int i, gid;
    int layer = fv->b.active_layer;

    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] && (gid=fv->b.map->map[i])!=-1 &&
		(sc=fv->b.sf->glyphs[gid])!=NULL )
    break;
    if ( i==fv->b.map->enccount )
return;

    if ( !TilePatternAsk(&td,fv->b.sf))
return;

    SFUntickAll(fv->b.sf);
    for ( i=0; i<fv->b.map->enccount; ++i )
	if ( fv->b.selected[i] && (gid=fv->b.map->map[i])!=-1 &&
		(sc=fv->b.sf->glyphs[gid])!=NULL && !sc->ticked  ) {
	    sc->ticked = true;
	    SCTilePattern(sc,layer,td.pattern, &td.patternSize, &td.repeatCnt);
	}
    TPFree(&td);
}
#endif 		/* FONTFORGE_CONFIG_TILEPATH */
