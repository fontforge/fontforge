/* Copyright (C) 2000,2001 by George Williams */
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
#include <math.h>
#include <unistd.h>
#include <utype.h>
#include <time.h>
#include <ustring.h>
#include <locale.h>
#include <chardata.h>

#include "psfont.h"		/* for struct fddata */

/* This file produces a ttf file given a splinefont. The most interesting thing*/
/*  it does is to figure out a quadratic approximation to the cubic splines */
/*  that postscript uses. We do this by looking at each spline and running */
/*  from the end toward the beginning, checking approximately every emunit */
/*  There is only one quadratic spline possible for any given interval of the */
/*  cubic. The start and end points are the interval end points (obviously) */
/*  the control point is where the two slopes (at start and end) intersect. */
/* If this spline is a close approximation to the cubic spline (doesn't */
/*  deviate from it by more than an emunit or so), then we use this interval */
/*  as one of our quadratic splines. */
/* It may turn out that the "quadratic" spline above is actually linear. Well */
/*  that's ok. It may also turn out that we can't find a good approximation. */
/*  If that's true then just insert a linear segment for an emunit stretch. */
/*  (actually this failure mode may not be possible), but I'm not sure */
/* Then we play the same trick for the rest of the cubic spline (if any) */

/* Does the quadratic spline in ttf approximate the cubic spline in ps */
/*  within one pixel between tmin and tmax (on ps. presumably ttf between 0&1 */
/* dim is the dimension in which there is the greatest change */
static int comparespline(Spline *ps, Spline *ttf, real tmin, real tmax) {
    int dim=0, other;
    real dx, dy, ddim, dt, t;
    real d, o;
    real ttf_t, sq, val;
    DBounds bb;

    /* Are all points on ttf near points on ps? */
    /* This doesn't answer that question, but rules out gross errors */
    bb.minx = bb.maxx = ps->from->me.x; bb.miny = bb.maxy = ps->from->me.y;
    if ( ps->from->nextcp.x>bb.maxx ) bb.maxx = ps->from->nextcp.x;
    else bb.minx = ps->from->nextcp.x;
    if ( ps->from->nextcp.y>bb.maxy ) bb.maxy = ps->from->nextcp.y;
    else bb.miny = ps->from->nextcp.y;
    if ( ps->to->prevcp.x>bb.maxx ) bb.maxx = ps->to->prevcp.x;
    else if ( ps->to->prevcp.x<bb.minx ) bb.minx = ps->to->prevcp.x;
    if ( ps->to->prevcp.y>bb.maxy ) bb.maxy = ps->to->prevcp.y;
    else if ( ps->to->prevcp.y<bb.miny ) bb.miny = ps->to->prevcp.y;
    if ( ps->to->me.x>bb.maxx ) bb.maxx = ps->to->me.x;
    else if ( ps->to->me.x<bb.minx ) bb.minx = ps->to->me.x;
    if ( ps->to->me.y>bb.maxy ) bb.maxy = ps->to->me.y;
    else if ( ps->to->me.y<bb.miny ) bb.miny = ps->to->me.y;
    for ( t=.1; t<1; t+= .1 ) {
	d = (ttf->splines[0].b*t+ttf->splines[0].c)*t+ttf->splines[0].d;
	o = (ttf->splines[1].b*t+ttf->splines[1].c)*t+ttf->splines[1].d;
	if ( d<bb.minx || d>bb.maxx || o<bb.miny || o>bb.maxy )
return( false );
    }

    /* Are all points on ps near points on ttf? */
    dx = ((ps->splines[0].a*tmax+ps->splines[0].b)*tmax+ps->splines[0].c)*tmax -
	 ((ps->splines[0].a*tmin+ps->splines[0].b)*tmin+ps->splines[0].c)*tmin ;
    dy = ((ps->splines[1].a*tmax+ps->splines[1].b)*tmax+ps->splines[1].c)*tmax -
	 ((ps->splines[1].a*tmin+ps->splines[1].b)*tmin+ps->splines[1].c)*tmin ;
    if ( dx<0 ) dx = -dx;
    if ( dy<0 ) dy = -dy;
    if ( dx>dy ) {
	dim = 0;
	ddim = dx;
    } else {
	dim = 1;
	ddim = dy;
    }
    other = !dim;

    t = tmin;
    dt = (tmax-tmin)/ddim;
    for ( t=tmin; t<=tmax; t+= dt ) {
	if ( t>tmax-dt/8. ) t = tmax;		/* Avoid rounding errors */
	d = ((ps->splines[dim].a*t+ps->splines[dim].b)*t+ps->splines[dim].c)*t+ps->splines[dim].d;
	o = ((ps->splines[other].a*t+ps->splines[other].b)*t+ps->splines[other].c)*t+ps->splines[other].d;
	if ( ttf->splines[dim].b == 0 ) {
	    ttf_t = (d-ttf->splines[dim].d)/ttf->splines[dim].c;
	} else {
	    sq = ttf->splines[dim].c*ttf->splines[dim].c -
		    4*ttf->splines[dim].b*(ttf->splines[dim].d-d);
	    if ( sq<0 )
return( false );
	    sq = sqrt(sq);
	    ttf_t = (-ttf->splines[dim].c-sq)/(2*ttf->splines[dim].b);
	    if ( ttf_t>=0 && ttf_t<=1.0 ) {
		val = (ttf->splines[other].b*ttf_t+ttf->splines[other].c)*ttf_t+
			    ttf->splines[other].d;
		if ( val>o-1 && val<o+1 )
    continue;
	    }
	    ttf_t = (-ttf->splines[dim].c+sq)/(2*ttf->splines[dim].b);
	}
	if ( ttf_t>=0 && ttf_t<=1.0 ) {
	    val = (ttf->splines[other].b*ttf_t+ttf->splines[other].c)*ttf_t+
			ttf->splines[other].d;
	    if ( val>o-1 && val<o+1 )
    continue;
	}
return( false );
    }

return( true );
}

static SplinePoint *MakeQuadSpline(SplinePoint *start,Spline *ttf,real x, real y) {
    Spline *new = chunkalloc(sizeof(Spline));
    SplinePoint *end = chunkalloc(sizeof(SplinePoint));

    end->me.x = end->nextcp.x = x;
    end->me.y = end->nextcp.y = y;
    end->nonextcp = true;

    *new = *ttf;
    new->from = start;		start->next = new;
    new->to = end;		end->prev = new;
    if ( new->splines[0].b==0 && new->splines[1].b==0 ) {
	end->noprevcp = true;
	end->prevcp.x = x; end->prevcp.y = y;
    } else {
	start->nextcp.x = ttf->splines[0].d+ttf->splines[0].c/3;
	end->prevcp.x = start->nextcp.x+(ttf->splines[0].b+ttf->splines[0].c)/3;
	start->nextcp.y = ttf->splines[1].d+ttf->splines[1].c/3;
	end->prevcp.y = start->nextcp.y+(ttf->splines[1].b+ttf->splines[1].c)/3;
	start->nonextcp = false;
    }
return( end );
}

static SplinePoint *LinearSpline(Spline *ps,SplinePoint *start, real tmax) {
    real x,y;
    Spline *new = chunkalloc(sizeof(Spline));
    SplinePoint *end = chunkalloc(sizeof(SplinePoint));

    x = ((ps->splines[0].a*tmax+ps->splines[0].b)*tmax+ps->splines[0].c)*tmax+ps->splines[0].d;
    y = ((ps->splines[1].a*tmax+ps->splines[1].b)*tmax+ps->splines[1].c)*tmax+ps->splines[1].d;
    end->me.x = end->nextcp.x = end->prevcp.x = x;
    end->me.y = end->nextcp.y = end->prevcp.y = y;
    end->nonextcp = end->noprevcp = start->nonextcp = true;
    new->from = start;		start->next = new;
    new->to = end;		end->prev = new;
    new->splines[0].d = start->me.x;
    new->splines[0].c = (x-start->me.x);
    new->splines[1].d = start->me.y;
    new->splines[1].c = (y-start->me.y);
return( end );
}

static SplinePoint *LinearTest(Spline *ps,real tmin,real tmax,
	real xmax,real ymax, SplinePoint *start) {
    Spline ttf;
    int dim=0, other;
    real dx, dy, ddim, dt, t;
    real d, o, val;
    real ttf_t;

    memset(&ttf,'\0',sizeof(ttf));
    ttf.splines[0].d = start->me.x;
    ttf.splines[0].c = (xmax-start->me.x);
    ttf.splines[1].d = start->me.y;
    ttf.splines[1].c = (ymax-start->me.y);

    if ( ( dx = xmax-start->me.x)<0 ) dx = -dx;
    if ( ( dy = ymax-start->me.y)<0 ) dy = -dy;
    if ( dx>dy ) {
	dim = 0;
	ddim = dx;
    } else {
	dim = 1;
	ddim = dy;
    }
    other = !dim;

    dt = (tmax-tmin)/ddim;
    for ( t=tmin; t<=tmax; t+= dt ) {
	d = ((ps->splines[dim].a*t+ps->splines[dim].b)*t+ps->splines[dim].c)*t+ps->splines[dim].d;
	o = ((ps->splines[other].a*t+ps->splines[other].b)*t+ps->splines[other].c)*t+ps->splines[other].d;
	ttf_t = (d-ttf.splines[dim].d)/ttf.splines[dim].c;
	val = ttf.splines[other].c*ttf_t+ ttf.splines[other].d;
	if ( val<o-1 || val>o+1 )
return( NULL );
    }
return( MakeQuadSpline(start,&ttf,xmax,ymax));
}

static SplinePoint *ttfapprox(Spline *ps,real tmin, real tmax, SplinePoint *start) {
    int dim=0, other;
    real dx, dy, ddim, dt, t;
    real x,y, xmin, ymin;
    real dxdtmin, dydtmin, dxdt, dydt;
    SplinePoint *sp;
    real cx, cy;
    Spline ttf;

    if ( RealNear(ps->splines[0].a,0) && RealNear(ps->splines[1].a,0) ) {
	/* Already Quadratic, just need to find the control point */
	/* Or linear, in which case we don't need to do much of anything */
	Spline *spline;
	sp = chunkalloc(sizeof(SplinePoint));
	sp->me.x = ps->to->me.x; sp->me.y = ps->to->me.y;
	sp->nonextcp = true;
	spline = chunkalloc(sizeof(Spline));
	spline->from = start;
	spline->to = sp;
	spline->splines[0] = ps->splines[0]; spline->splines[1] = ps->splines[1];
	start->next = sp->prev = spline;
	if ( ps->knownlinear ) {
	    spline->islinear = spline->knownlinear = true;
	    start->nonextcp = sp->noprevcp = true;
	    start->nextcp = start->me;
	    sp->prevcp = sp->me;
	} else {
	    start->nonextcp = sp->noprevcp = false;
	    start->nextcp.x = sp->prevcp.x = (ps->splines[0].c+2*ps->splines[0].d)/2;
	    start->nextcp.y = sp->prevcp.y = (ps->splines[1].c+2*ps->splines[1].d)/2;
	}
return( sp );
    }

    memset(&ttf,'\0',sizeof(ttf));
  tail_recursion:

    xmin = start->me.x;
    ymin = start->me.y;
    dxdtmin = (3*ps->splines[0].a*tmin+2*ps->splines[0].b)*tmin + ps->splines[0].c;
    dydtmin = (3*ps->splines[1].a*tmin+2*ps->splines[1].b)*tmin + ps->splines[1].c;

    dx = ((ps->splines[0].a*tmax+ps->splines[0].b)*tmax+ps->splines[0].c)*tmax -
	 ((ps->splines[0].a*tmin+ps->splines[0].b)*tmin+ps->splines[0].c)*tmin ;
    dy = ((ps->splines[1].a*tmax+ps->splines[1].b)*tmax+ps->splines[1].c)*tmax -
	 ((ps->splines[1].a*tmin+ps->splines[1].b)*tmin+ps->splines[1].c)*tmin ;
    if ( dx<0 ) dx = -dx;
    if ( dy<0 ) dy = -dy;
    if ( dx>dy ) {
	dim = 0;
	ddim = dx;
    } else {
	dim = 1;
	ddim = dy;
    }
    other = !dim;

    if ( ddim<2 )
return( LinearSpline(ps,start,tmax));

    dt = (tmax-tmin)/ddim;
    for ( t=tmax; t>tmin+dt/128; t-= dt ) {		/* dt/128 is a hack to avoid rounding errors */
	x = ((ps->splines[0].a*t+ps->splines[0].b)*t+ps->splines[0].c)*t+ps->splines[0].d;
	y = ((ps->splines[1].a*t+ps->splines[1].b)*t+ps->splines[1].c)*t+ps->splines[1].d;
	ttf.splines[0].d = xmin;
	ttf.splines[0].c = x-xmin;
	ttf.splines[0].b = 0;
	ttf.splines[1].d = ymin;
	ttf.splines[1].c = y-ymin;
	ttf.splines[1].b = 0;
	if ( comparespline(ps,&ttf,tmin,t) ) {
	    sp = LinearSpline(ps,start,t);
	    if ( t==tmax )
return( sp );
	    tmin = t;
	    start = sp;
  goto tail_recursion;
	}
	dxdt = (3*ps->splines[0].a*t+2*ps->splines[0].b)*t + ps->splines[0].c;
	dydt = (3*ps->splines[1].a*t+2*ps->splines[1].b)*t + ps->splines[1].c;
	/* if the slopes are parallel at the ends there can be no bezier quadratic */
	/*  (control point is where the splines intersect. But if they are */
	/*  parallel and colinear then there is a line between 'em */
	if ( ( dxdtmin==0 && dxdt==0 ) || (dydtmin==0 && dydt==0) ||
		( dxdt!=0 && dxdtmin!=0 &&
		    RealNearish(dydt/dxdt,dydtmin/dxdtmin)) ) {
	    if (( dxdt==0 && x==xmin ) || (dydt==0 && y==ymin) ||
		    (dxdt!=0 && x!=xmin && RealNearish(dydt/dxdt,(y-ymin)/(x-xmin))) ) {
		if ( (sp = LinearTest(ps,tmin,t,x,y,start))!=NULL ) {
		    if ( t==tmax )
return( sp );
		    tmin = t;
		    start = sp;
  goto tail_recursion;
	      }
	  }
    continue;
	}
	if ( dxdt==0 )
	    cx=x;
	else if ( dxdtmin==0 )
	    cx=xmin;
	else
	    cx = -(ymin-(dydtmin/dxdtmin)*xmin-y+(dydt/dxdt)*x)/(dydtmin/dxdtmin-dydt/dxdt);
	cy = (dydt/dxdt)*(cx-x)+y;
	/* Make the quadratic spline from (xmin,ymin) through (cx,cy) to (x,y)*/
	ttf.splines[0].d = xmin;
	ttf.splines[0].c = 2*(cx-xmin);
	ttf.splines[0].b = xmin+x-2*cx;
	ttf.splines[1].d = ymin;
	ttf.splines[1].c = 2*(cy-ymin);
	ttf.splines[1].b = ymin+y-2*cy;
	if ( comparespline(ps,&ttf,tmin,t) ) {
	    sp = MakeQuadSpline(start,&ttf,x,y);
/* Without these two lines we would turn out the cps for cubic splines */
/*  (what postscript uses). With the lines we get the cp for the quadratic */
/* So with them we've got the wrong cps for cubic splines and can't manipulate*/
/*  em properly within the editor */
#if 1
	    start->nextcp.x = sp->prevcp.x = cx;
	    start->nextcp.y = sp->prevcp.y = cy;
#endif
/* End of quadratic code */
	    if ( t==tmax )
return( sp );
	    tmin = t;
	    start = sp;
  goto tail_recursion;
	}
    }
    tmin += dt;
    start = LinearSpline(ps,start,tmin);
  goto tail_recursion;
}
    
static SplineSet *SSttfApprox(SplineSet *ss) {
    SplineSet *ret = chunkalloc(sizeof(SplineSet));
    Spline *spline, *first;

    ret->first = chunkalloc(sizeof(SplinePoint));
    *ret->first = *ss->first;
    ret->last = ret->first;

    first = NULL;
    for ( spline=ss->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	ret->last = ttfapprox(spline,0,1,ret->last);
	if ( first==NULL ) first = spline;
    }
    if ( ss->first==ss->last ) {
	if ( ret->last!=ret->first ) {
	    ret->first->prevcp = ret->last->prevcp;
	    ret->first->noprevcp = ret->last->noprevcp;
	    ret->last->prev->to = ret->first;
	    SplinePointFree(ret->last);
	    ret->last = ret->first;
	}
    }
return( ret );
}

#if 0
static SplineSet *SplineSetsTTFApprox(SplineSet *ss) {
    SplineSet *head=NULL, *last, *cur;

    while ( ss!=NULL ) {
	cur = SSttfApprox(ss);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	ss = ss->next;
    }
return( head );
}
#endif

/* ************************************************************************** */

/* Required tables:
	cmap		encoding
	head		header data
	hhea		more header data
	hmtx		horizontal metrics (widths, lsidebearing)
	maxp		various maxima in the font
	name		various names associated with the font
	post		postscript names and other stuff
	OS/2		bleah.
Required for TrueType
	loca		pointers to the glyphs
	glyf		character shapes
Required for OpenType (Postscript)
	CFF 		A complete postscript CFF font here with all its internal tables
additional tables
	kern		(if data are present)
	cvt		for hinting
*/

struct tabdir {
    int32 version;	/* 0x00010000 */
    uint16 numtab;
    uint16 searchRange;	/* (Max power of 2 <= numtab) *16 */
    uint16 entrySel;	/* Log2(Max power of 2 <= numtab ) */
    uint16 rangeShift;	/* numtab*16 - searchRange */
    struct taboff {
	uint32 tag;	/* Table name */
	uint32 checksum;/* for table */
	uint32 offset;	/* to start of table in file */
	uint32 length;
    } tabs[11];		/* room for all the above tables */
};

struct glyphhead {
    int16 numContours;
    int16 xmin;
    int16 ymin;
    int16 xmax;
    int16 ymax;
};

struct head {
    int32 version;	/* 0x00010000 */
    int32 revision;	/* 0 */
    uint32 checksumAdj;	/* set to 0, sum entire font, store 0xb1b0afba-sum */
    uint32 magicNum;	/* 0x5f0f3cf5 */
    uint16 flags;	/* 1 */
    uint16 emunits;	/* sf->ascent+sf->descent */
    int32 createtime[2];/* number of seconds since 1904 */
    int32 modtime[2];
    int16 xmin;		/* min for entire font */
    int16 ymin;
    int16 xmax;
    int16 ymax;
    uint16 macstyle;	/* 1=>Bold, 2=>Italic */
    uint16 lowestreadable;	/* size in pixels. Say about 10? */
    int16 dirhint;	/* 0=>mixed directional characters, */
    int16 locais32;	/* is the location table 32bits or 16, 0=>16, 1=>32 */
    int16 glyphformat;	/* 0 */
    uint16 mbz;		/* padding */
};

struct hhead {
    int32 version;	/* 0x00010000 */
    int16 ascender;	/* sf->ascender */
    int16 descender;	/* -sf->descender */
    int16 linegap;	/* 0 */
    int16 maxwidth;	/* of all characters */
    int16 minlsb;	/* How is this different from xmin above? */
    int16 minrsb;
    int16 maxextent;	/* How is this different from xmax above? */
    int16 caretSlopeRise;/* Uh... let's say 1? */
    int16 caretSlopeRun;/* Uh... let's say 0 */
    int16 mbz[5];
    int16 metricformat;	/* 0 */
    uint16 numMetrics;	/* just set to glyph count */
};

struct hmtx {
    uint16 width;	/* NOTE: TTF only allows positive widths!!! */
    int16 lsb;
};

struct kern {
    uint16 version;	/* 0 */
    uint16 ntab;	/* 1, number of subtables */
    /* first (and only) subtable */
    uint16 stversion;	/* 0 */
    uint16 length;	/* length of subtable beginning at &stversion */
    uint16 coverage;	/* 1, (set of flags&format) */
    uint16 nPairs;	/* number of kern pairs */
    uint16 searchRange;	/* (Max power of 2 <= nPairs) *6 */
    uint16 entrySel;	/* Log2(Max power of 2 <= nPairs ) */
    uint16 rangeShift;	/* numtab*6 - searchRange */
    struct kp {
	uint16 left;	/* left glyph num */
	uint16 right;	/* right glyph num */
	/* table is ordered by these two above treated as uint32 */
	int16 offset;	/* kern amount */
    } *kerns;		/* Array should be nPairs big */
};

struct maxp {
    int32 version;	/* 0x00010000 */
    uint16 numGlyphs;
    uint16 maxPoints;	/* max number of points in a simple glyph */
    uint16 maxContours;	/* max number of paths in a simple glyph */
    uint16 maxCompositPts;
    uint16 maxCompositCtrs;
    uint16 maxZones;	/* 1 */
    uint16 maxTwilightPts;	/* 0 */
    uint16 maxStorage;	/* 0 */
    uint16 maxFDEFs;	/* 0 */
    uint16 maxIDEFs;	/* 0 */
    uint16 maxStack;	/* 0 */
    uint16 maxglyphInstr;/* 0 */
    uint16 maxnumcomponents;	/* Maximum number of refs in any composit */
    uint16 maxcomponentdepth;	/* 0 (if no composits), 1 else */
    uint16 mbz;		/* pad out to a 4byte boundary */
};

struct nametab {
    uint16 format;	/* 0 */
    uint16 numrec;	/* 1 */
    uint16 startOfStrings;	/* offset from start of table to start of strings */
    struct namerec {
	uint16 platform;	/* 3 => MS */
	uint16 specific;	/* 1 */
	uint16 language;	/* 0x0409 */
	uint16 nameid;		/* 0=>copyright, 1=>family, 2=>weight, 4=>fullname */
				/*  5=>version, 6=>postscript name */
	uint16 strlen;
	uint16 stroff;
    } nr[6];
};

struct os2 {
    uint16 version;	/* 1 */
    int16 avgCharWid;	/* average width of a-z and space, if no lower case, then average all chars */
    uint16 weightClass;	/* 100=>thin, 200=>extra-light, 300=>light, 400=>normal, */
			/* 500=>Medium, 600=>semi-bold, 700=>bold, 800=>extra-bold, */
			/* 900=>black */
    uint16 widthClass;	/* 75=>condensed, 100, 125=>expanded */
    int16 fstype;	/* 0x0008 => allow embedded editing */
    int16 ysubXSize;	/* emsize/5 */
    int16 ysubYSize;	/* emsize/5 */
    int16 ysubXOff;	/* 0 */
    int16 ysubYOff;	/* emsize/5 */
    int16 ysupXSize;	/* emsize/5 */
    int16 ysupYSize;	/* emsize/5 */
    int16 ysupXOff;	/* 0 */
    int16 ysupYOff;	/* emsize/5 */
    int16 yStrikeoutSize;	/* 102/2048 *emsize */
    int16 yStrikeoutPos;	/* 530/2048 *emsize */
    int16 sFamilyClass;	/* ??? 0 */
	/* high order byte is the "class", low order byte the sub class */
	/* class = 0 => no classification */
	/* class = 1 => old style serifs */
	/*	subclass 0, no class; 1 ibm rounded; 2 garalde; 3 venetian; 4 mod venitian; 5 dutch modern; 6 dutch trad; 7 contemporary; 8 caligraphic; 15 misc */
	/* class = 2 => transitional serifs */
	/*	subclass 0, no class; 1 drect line; 2 script; 15 misc */
	/* class = 3 => modern serifs */
	/*	subclass: 1, italian; 2, script */
	/* class = 4 => clarendon serifs */
	/*	subclass: 1, clarendon; 2, modern; 3 trad; 4 newspaper; 5 stub; 6 monotone; 7 typewriter */
	/* class = 5 => slab serifs */
	/*	subclass: 1, monotone; 2, humanist; 3 geometric; 4 swiss; 5 typewriter */
	/* class = 7 => freeform serifs */
	/*	subclass: 1, modern */
	/* class = 8 => sans serif */
	/*	subclass: 1, ibm neogrotesque; 2 humanist; 3 low-x rounded; 4 high-x rounded; 5 neo-grotesque; 6 mod neo-grot; 9 typewriter; 10 matrix */
	/* class = 9 => ornamentals */
	/*	subclass: 1, engraver; 2 black letter; 3 decorative; 4 3D */
	/* class = 10 => scripts */
	/*	subclass: 1, uncial; 2 brush joined; 3 formal joined; 4 monotone joined; 5 calligraphic; 6 brush unjoined; 7 formal unjoined; 8 monotone unjoined */
	/* class = 12 => symbolic */
	/*	subclass: 3 mixed serif; 6 old style serif; 7 neo-grotesque sans; */
    char panose[10];	/* can be set to zero */
    uint32 unicoderange[4];	
	/* 1<<0=>ascii, 1<<1 => latin1, 2=>100-17f, 3=>180-24f, 4=>250-2af */
	/* 5=> 2b0-2ff, 6=>300-36f, ... */
    char achVendID[4];	/* can be zero */
    uint16 fsSel;	/* 1=> italic, 32=>bold, 64 => regular */
    uint16 firstcharindex; /* minimum unicode encoding */
    uint16 lastcharindex;  /* maximum unicode encoding */
    uint16 ascender;	/* font ascender height (not ascent) */
    uint16 descender;	/* font descender height */
    uint16 linegap;	/* 0 */
    uint16 winascent;	/* ymax */
    uint16 windescent;	/* ymin */
    uint32 ulCodePage[2];
	/* 1<<0 => latin1, 1<<1=>latin2, cyrillic, greek, turkish, hebrew, arabic */
	/* 1<<30 => mac, 1<<31 => symbol */
    /* OTF stuff (version 2 of OS/2) */
    short xHeight;
    short capHeight;
    short defChar;
    short breakChar;
    short maxContext;
};

struct post {
    int32 formattype;		/* 0x00020000 */
    int32 italicAngle;		/* in fixed format */
    int16 upos;
    int16 uwidth;
    uint32 isfixed;
    uint32 minmem42;
    uint32 maxmem42;
    uint32 minmem1;
    uint32 maxmem1;
    uint16 numglyphs;
    uint16 glyphnameindex[1];
};

const char *ttfstandardnames[258] = {
".notdef",
".null",
"nonmarkingreturn",
"space",
"exclam",
"quotedbl",
"numbersign",
"dollar",
"percent",
"ampersand",
"quotesingle",
"parenleft",
"parenright",
"asterisk",
"plus",
"comma",
"hyphen",
"period",
"slash",
"zero",
"one",
"two",
"three",
"four",
"five",
"six",
"seven",
"eight",
"nine",
"colon",
"semicolon",
"less",
"equal",
"greater",
"question",
"at",
"A",
"B",
"C",
"D",
"E",
"F",
"G",
"H",
"I",
"J",
"K",
"L",
"M",
"N",
"O",
"P",
"Q",
"R",
"S",
"T",
"U",
"V",
"W",
"X",
"Y",
"Z",
"bracketleft",
"backslash",
"bracketright",
"asciicircum",
"underscore",
"grave",
"a",
"b",
"c",
"d",
"e",
"f",
"g",
"h",
"i",
"j",
"k",
"l",
"m",
"n",
"o",
"p",
"q",
"r",
"s",
"t",
"u",
"v",
"w",
"x",
"y",
"z",
"braceleft",
"bar",
"braceright",
"asciitilde",
"Adieresis",
"Aring",
"Ccedilla",
"Eacute",
"Ntilde",
"Odieresis",
"Udieresis",
"aacute",
"agrave",
"acircumflex",
"adieresis",
"atilde",
"aring",
"ccedilla",
"eacute",
"egrave",
"ecircumflex",
"edieresis",
"iacute",
"igrave",
"icircumflex",
"idieresis",
"ntilde",
"oacute",
"ograve",
"ocircumflex",
"odieresis",
"otilde",
"uacute",
"ugrave",
"ucircumflex",
"udieresis",
"dagger",
"degree",
"cent",
"sterling",
"section",
"bullet",
"paragraph",
"germandbls",
"registered",
"copyright",
"trademark",
"acute",
"dieresis",
"notequal",
"AE",
"Oslash",
"infinity",
"plusminus",
"lessequal",
"greaterequal",
"yen",
"mu",
"partialdiff",
"summation",
"product",
"pi",
"integral",
"ordfeminine",
"ordmasculine",
"Omega",
"ae",
"oslash",
"questiondown",
"exclamdown",
"logicalnot",
"radical",
"florin",
"approxequal",
"Delta",
"guillemotleft",
"guillemotright",
"ellipsis",
"nonbreakingspace",
"Agrave",
"Atilde",
"Otilde",
"OE",
"oe",
"endash",
"emdash",
"quotedblleft",
"quotedblright",
"quoteleft",
"quoteright",
"divide",
"lozenge",
"ydieresis",
"Ydieresis",
"fraction",
"currency",
"guilsinglleft",
"guilsinglright",
"fi",
"fl",
"daggerdbl",
"periodcentered",
"quotesinglbase",
"quotedblbase",
"perthousand",
"Acircumflex",
"Ecircumflex",
"Aacute",
"Edieresis",
"Egrave",
"Iacute",
"Icircumflex",
"Idieresis",
"Igrave",
"Oacute",
"Ocircumflex",
"apple",
"Ograve",
"Uacute",
"Ucircumflex",
"Ugrave",
"dotlessi",
"circumflex",
"tilde",
"macron",
"breve",
"dotaccent",
"ring",
"cedilla",
"hungarumlaut",
"ogonek",
"caron",
"Lslash",
"lslash",
"Scaron",
"scaron",
"Zcaron",
"zcaron",
"brokenbar",
"Eth",
"eth",
"Yacute",
"yacute",
"Thorn",
"thorn",
"minus",
"multiply",
"onesuperior",
"twosuperior",
"threesuperior",
"onehalf",
"onequarter",
"threequarters",
"franc",
"Gbreve",
"gbreve",
"Idotaccent",
"Scedilla",
"scedilla",
"Cacute",
"cacute",
"Ccaron",
"ccaron",
"dcroat"
};

static unichar_t uniranges[][2] = {
    { 0x20, 0x7e },
    { 0xa0, 0xff },
    { 0x100, 0x17f },
    { 0x180, 0x24f },
    { 0x250, 0x2af },
    { 0x2b0, 0x2ff },
    { 0x300, 0x36f },
    { 0x370, 0x3cf },
    { 0x3d0, 0x3ff },
    { 0x400, 0x4ff },
    { 0x530, 0x58f },
    { 0x590, 0x5ff },
    { 0,0 },		/* Supposed to be hebrew extended, but I can't find that */
    { 0x600, 0x6ff },
    { 0,0 },
    { 0x900, 0x97f },
    { 0x980, 0x9ff },
    { 0xa00, 0xa7f },
    { 0xa80, 0xaff },
    { 0xb00, 0xb7f },
    { 0xb80, 0xbff },		/* bit 20, tamil */
    { 0xc00, 0xc7f },
    { 0xc80, 0xcff },
    { 0xd00, 0xd7f },
    { 0xe00, 0xe7f },
    { 0xe80, 0xeff },		/* bit 25, lao */
    { 0x10c0, 0x10ff },
    { 0x10a0, 0x10bf },
    { 0x1100, 0x11ff },
    { 0x1e00, 0x1eff },
    { 0x1f00, 0x1fff },		/* bit 30, greek extended */
    { 0x2000, 0x206f },

    { 0x2070, 0x209f },		/* bit 32, subscripts, super scripts */
    { 0x20a0, 0x20cf },
    { 0x20d0, 0x20ff },
    { 0x2100, 0x214f },
    { 0x2150, 0x218f },
    { 0x2190, 0x21ff },
    { 0x2200, 0x22ff },
    { 0x2300, 0x237f },
    { 0x2400, 0x243f },		/* bit 40 control pictures */
    { 0x2440, 0x245f },
    { 0x2460, 0x24ff },
    { 0x2500, 0x257f },
    { 0x2580, 0x259f },
    { 0x25a0, 0x25ff },		/* bit 45 geometric shapes */
    { 0x2600, 0x267f },
    { 0x2700, 0x27bf },		/* bit 47 zapf dingbats */
    { 0x3000, 0x303f },
    { 0x3040, 0x309f },
    { 0x30a0, 0x30ff },		/* bit 50 katakana */
    { 0x3100, 0x312f },
    { 0x3130, 0x318f },
    { 0x3190, 0x31ff },
    { 0x3200, 0x32ff },
    { 0x3300, 0x33ff },		/* bit 55 CJK compatability */
    { 0xac00, 0xd7af },		/* bit 56 Hangul */
    { 0, 0 },			/* subranges */
    { 0, 0 },			/* subranges */
    { 0x4e00, 0x9fff },		/* bit 59, CJK */
    { 0, 0 },			/* bit 60, private use */
    { 0xf900, 0xfaff },
    { 0xfb00, 0xfb4f },
    { 0xfb50, 0xfdff },

    { 0xfe20, 0xfe2f },		/* bit 64 combining half marks */
    { 0xfe30, 0xfe4f },
    { 0xfe50, 0xfe6f },
    { 0xfe70, 0xfeef },
    { 0xff00, 0xffef },
    { 0xfff0, 0xffff },
    { 0xffff, 0xffff}
};

struct glyphinfo {
    struct maxp *maxp;		/* this one is given to dumpglyphs, rest blank */
    uint32 *loca;
    FILE *glyphs;
    FILE *hmtx;
    int hmtxlen;
    int next_glyph;
    int glyph_len;
    short *cvt;
    int cvtmax;
    int cvtcur;
    int xmin, ymin, xmax, ymax;
    real blues[14];
    int bcnt;
};

struct alltabs {
    struct tabdir tabdir;
    struct head head;
    struct hhead hhead;
    struct maxp maxp;
    struct os2 os2;
    FILE *loca;
    int localen;
    FILE *name;
    int namelen;
    FILE *post;
    int postlen;
    FILE *kern;
    int kernlen;
    FILE *cmap;
    int cmaplen;
    FILE *headf;
    int headlen;
    FILE *hheadf;
    int hheadlen;
    FILE *maxpf;
    int maxplen;
    FILE *os2f;
    int os2len;
    FILE *cvtf;
    int cvtlen;
    FILE *cfff;
    int cfflen;
    FILE *sidf;
    FILE *sidh;
    FILE *charset;
    FILE *encoding;
    FILE *private;
    FILE *charstrings;
    FILE *fdselect;
    FILE *fdarray;
    int defwid, nomwid;
    int sidcnt;
    int lenpos;
    int privatelen;
    unsigned int sidlongoffset: 1;
    unsigned int cfflongoffset: 1;
    struct glyphinfo gi;
    int isfixed;
    struct fd2data *fds;
};

static int32 getuint32(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    int ch3 = getc(ttf);
    int ch4 = getc(ttf);
    if ( ch4==EOF )
return( EOF );
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

static int32 getushort(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    if ( ch2==EOF )
return( EOF );
return( (ch1<<8)|ch2 );
}

static void dumpshort(FILE *file,int sval) {
    putc((sval>>8)&0xff,file);
    putc(sval&0xff,file);
}

static void dumplong(FILE *file,int val) {
    putc((val>>24)&0xff,file);
    putc((val>>16)&0xff,file);
    putc((val>>8)&0xff,file);
    putc(val&0xff,file);
}
#define dumpabsoffset	dumplong

static void dumpoffset(FILE *file,int offsize,int val) {
    if ( offsize==1 )
	putc(val,file);
    else if ( offsize==2 )
	dumpshort(file,val);
    else if ( offsize==3 ) {
	putc((val>>16)&0xff,file);
	putc((val>>8)&0xff,file);
	putc(val&0xff,file);
    } else
	dumplong(file,val);
}

static void dump2d14(FILE *file,real dval) {
    int val;
    int mant;

    val = floor(dval);
    mant = floor(16384.*(dval-val));
    val = (val<<14) | mant;
    dumpshort(file,val);
}

static void dumpfixed(FILE *file,real dval) {
    int val;
    int mant;

    val = floor(dval);
    mant = floor(65536.*(dval-val));
    val = (val<<16) | mant;
    dumplong(file,val);
}

static void copyfile(FILE *ttf, FILE *other, int pos) {
    int ch;

    if ( pos!=ftell(ttf))
	GDrawIError("File Offset wrong for ttf table, %d expected %d", ftell(ttf), pos );
    rewind(other);
    while (( ch = getc(other))!=EOF )
	putc(ch,ttf);
    fclose(other);
}

static int getcvtval(struct glyphinfo *gi,int val) {
    int i;

    /* by default sign is unimportant in the cvt */
    if ( val<0 ) val = -val;

    if ( gi->cvtmax==0 ) {
	gi->cvtmax = 100;
	gi->cvt = galloc(100*sizeof(short));
    }
    for ( i=0; i<gi->cvtcur; ++i )
	if ( val>=gi->cvt[i]-1 && val<=gi->cvt[i]+1 )
return( i );
    if ( i>=gi->cvtmax ) {
	gi->cvtmax += 100;
	gi->cvt = grealloc(gi->cvt,gi->cvtmax*sizeof(short));
    }
    gi->cvt[i] = val;
    ++gi->cvtcur;
return( i );
}

static void dumpghstruct(struct glyphinfo *gi,struct glyphhead *gh) {

    dumpshort(gi->glyphs,gh->numContours);
    dumpshort(gi->glyphs,gh->xmin);
    dumpshort(gi->glyphs,gh->ymin);
    dumpshort(gi->glyphs,gh->xmax);
    dumpshort(gi->glyphs,gh->ymax);
    if ( gh->xmin<gi->xmin ) gi->xmin = gh->xmin;
    if ( gh->ymin<gi->ymin ) gi->ymin = gh->ymin;
    if ( gh->xmax>gi->xmax ) gi->xmax = gh->xmax;
    if ( gh->ymax>gi->ymax ) gi->ymax = gh->ymax;
}

static int SSPointCnt(SplineSet *ss) {
    SplinePoint *sp, *first=NULL;
    int cnt;

    ss = SSttfApprox(ss);
    for ( sp=ss->first, cnt=0; sp!=first ; ) {
	if ( sp==ss->first || sp->nonextcp || sp->noprevcp ||
		(sp->prevcp.x+sp->nextcp.x)/2!=sp->me.x ||
		(sp->prevcp.y+sp->nextcp.y)/2!=sp->me.y )
	    ++cnt;
	if ( !sp->nonextcp ) ++cnt;
	if ( sp->next==NULL )
    break;
	if ( first==NULL ) first = sp;
	sp = sp->next->to;
    }
    SplinePointListFree(ss);
    /* this may produce a slight overcount if we find some points we can */
    /*  omit (an on curve point that is midway between two off curve points */
    /*  then we can omit it) */
return( cnt );
}

#define _On_Curve	1
#define _X_Short	2
#define _Y_Short	4
#define _Repeat		8
#define _X_Same		0x10
#define _Y_Same		0x20

static int SSAddPoints(SplineSet *ss,int ptcnt,BasePoint *bp, char *flags) {
    SplinePoint *sp, *first;

    ss = SSttfApprox(ss);
    first = NULL;
    for ( sp=ss->first; sp!=first ; ) {
	if ( sp==ss->first || sp->nonextcp || sp->noprevcp ||
		(sp->prevcp.x+sp->nextcp.x)/2!=sp->me.x ||
		(sp->prevcp.y+sp->nextcp.y)/2!=sp->me.y ) {
	    /* If an on curve point is midway between two off curve points*/
	    /*  it may be omitted and will be interpolated on read in */
	    flags[ptcnt] = _On_Curve;
	    bp[ptcnt].x = rint(sp->me.x);
	    bp[ptcnt++].y = rint(sp->me.y);
	}
	if ( !sp->nonextcp ) {
	    flags[ptcnt] = 0;
	    bp[ptcnt].x = rint(sp->nextcp.x);
	    bp[ptcnt++].y = rint(sp->nextcp.y);
	}
	if ( sp->next==NULL )
    break;
	if ( first==NULL ) first = sp;
	sp = sp->next->to;
    }
    SplinePointListFree(ss);
return( ptcnt );
}

static void dumppointarrays(struct glyphinfo *gi,BasePoint *bp, char *fs, int pc) {
    BasePoint last;
    int i,flags;
    int lastflag, flagcnt;

    if ( gi->maxp->maxPoints<pc )
	gi->maxp->maxPoints = pc;

	/* flags */
    last.x = last.y = 0;
    lastflag = -1; flagcnt = 0;
    for ( i=0; i<pc; ++i ) {
	flags = 0;
	if ( fs==NULL || fs[i] )
	    flags = _On_Curve;		/* points are on curve */
	if ( last.x==bp[i].x )
	    flags |= _X_Same;
	else if ( bp[i].x-last.x>-256 && bp[i].x-last.x<255 ) {
	    flags |= _X_Short;
	    if ( bp[i].x>=last.x )
		flags |= _X_Same;		/* In this context it means positive */
	}
	if ( last.y==bp[i].y )
	    flags |= _Y_Same;
	else if ( bp[i].y-last.y>-256 && bp[i].y-last.y<255 ) {
	    flags |= _Y_Short;
	    if ( bp[i].y>=last.y )
		flags |= _Y_Same;		/* In this context it means positive */	
	}
	last = bp[i];
	if ( lastflag==-1 ) {
	    lastflag = flags;
	    flagcnt = 0;
	} else if ( flags!=lastflag ) {
	    if ( flagcnt!=0 )
		lastflag |= _Repeat;
	    putc(lastflag,gi->glyphs);
	    if ( flagcnt!=0 )
		putc(flagcnt,gi->glyphs);
	    lastflag = flags;
	    flagcnt = 0;
	} else {
	    if ( ++flagcnt == 255 ) {
		putc(lastflag|_Repeat,gi->glyphs);
		putc(255,gi->glyphs);
		lastflag = -1;
		flagcnt = 0;
	    }
	}
    }
    if ( lastflag!=-1 ) {
	if ( flagcnt!=0 )
	    lastflag |= _Repeat;
	putc(lastflag,gi->glyphs);
	if ( flagcnt!=0 )
	    putc(flagcnt,gi->glyphs);
    }

	/* xcoords */
    last.x = 0;
    for ( i=0; i<pc; ++i ) {
	if ( last.x==bp[i].x )
	    /* Do Nothing */;
	else if ( bp[i].x-last.x>-256 && bp[i].x-last.x<255 ) {
	    if ( bp[i].x>=last.x )
		putc(bp[i].x-last.x,gi->glyphs);
	    else
		putc(last.x-bp[i].x,gi->glyphs);
	} else
	    dumpshort(gi->glyphs,bp[i].x-last.x);
	last.x = bp[i].x;
    }
	/* ycoords */
    last.y = 0;
    for ( i=0; i<pc; ++i ) {
	if ( last.y==bp[i].y )
	    /* Do Nothing */;
	else if ( bp[i].y-last.y>-256 && bp[i].y-last.y<255 ) {
	    if ( bp[i].y>=last.y )
		putc(bp[i].y-last.y,gi->glyphs);
	    else
		putc(last.y-bp[i].y,gi->glyphs);
	} else
	    dumpshort(gi->glyphs,bp[i].y-last.y);
	last.y = bp[i].y;
    }
    if ( ftell(gi->glyphs)&1 )		/* Pad the file so that the next glyph */
	putc('\0',gi->glyphs);		/* on a word boundary */
}

static void dumpinstrs(struct glyphinfo *gi,uint8 *instrs,int cnt) {
    int i;

 /*cnt = 0;		/* Debug */
    if ( gi->maxp->maxglyphInstr<cnt ) gi->maxp->maxglyphInstr=cnt;
    dumpshort(gi->glyphs,cnt);
    for ( i=0; i<cnt; ++i )
	putc( instrs[i],gi->glyphs );
}

static void dumpmissingglyph(SplineFont *sf,struct glyphinfo *gi) {
    struct glyphhead gh;
    BasePoint bp[10];
    uint8 instrs[50];
    int stemcvt, stem;

    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);
    gi->maxp->maxContours = 2;

    gh.numContours = 2;
    gh.ymin = 0;
    gh.ymax = 2*(sf->ascent+sf->descent)/3;
    gh.xmax = (sf->ascent+sf->descent)/3;
    gh.xmin = stem = gh.xmax/10;
    gh.xmax += stem;
    if ( gh.ymax>sf->ascent ) gh.ymax = sf->ascent;
    dumpghstruct(gi,&gh);

    stemcvt = getcvtval(gi,stem);

    bp[0].x = stem;		bp[0].y = 0;
    bp[1].x = stem;		bp[1].y = gh.ymax;
    bp[2].x = gh.xmax;		bp[2].y = gh.ymax;
    bp[3].x = gh.xmax;		bp[3].y = 0;

    bp[4].x = 2*stem;		bp[4].y = stem;
    bp[5].x = gh.xmax-stem;	bp[5].y = stem;
    bp[6].x = gh.xmax-stem;	bp[6].y = gh.ymax-stem;
    bp[7].x = 2*stem;		bp[7].y = gh.ymax-stem;

    instrs[0] = 0xb1;			/* Pushb, 2byte */
    instrs[1] = 1;			/* Point 1 */
    instrs[2] = 0;			/* Point 0 */
    instrs[3] = 0x2f;			/* MDAP, rounded (pt0) */
    instrs[4] = 0x3c;			/* ALIGNRP, (pt1 same pos as pt0)*/
    instrs[5] = 0xb2;			/* Pushb, 3byte */
    instrs[6] = 7;			/* Point 7 */
    instrs[7] = 4;			/* Point 4 */
    instrs[8] = stemcvt;		/* CVT entry for our stem width */
    instrs[9] = 0xe0+0x0d;		/* MIRP, don't set rp0, minimum, rounded, black */
    instrs[10] = 0x32;			/* SHP[rp2] (pt7 same pos as pt4) */
    instrs[11] = 0xb1;			/* Pushb, 2byte */
    instrs[12] = 6;			/* Point 6 */
    instrs[13] = 5;			/* Point 5 */
    instrs[14] = 0xc0+0x1c;		/* MDRP, set rp0, minimum, rounded, grey */
    instrs[15] = 0x3c;			/* ALIGNRP, (pt6 same pos as pt5)*/
    instrs[16] = 0xb2;			/* Pushb, 3byte */
    instrs[17] = 3;			/* Point 3 */
    instrs[18] = 2;			/* Point 2 */
    instrs[19] = stemcvt;		/* CVT entry for our stem width */
    instrs[20] = 0xe0+0x0d;		/* MIRP, dont set rp0, minimum, rounded, black */
    instrs[21] = 0x32;			/* SHP[rp2] (pt3 same pos as pt2) */

    instrs[22] = 0x00;			/* SVTCA, y axis */

    instrs[23] = 0xb1;			/* Pushb, 2byte */
    instrs[24] = 3;			/* Point 3 */
    instrs[25] = 0;			/* Point 0 */
    instrs[26] = 0x2f;			/* MDAP, rounded */
    instrs[27] = 0x3c;			/* ALIGNRP, (pt3 same height as pt0)*/
    instrs[28] = 0xb2;			/* Pushb, 3byte */
    instrs[29] = 5;			/* Point 5 */
    instrs[30] = 4;			/* Point 4 */
    instrs[31] = stemcvt;		/* CVT entry for our stem width */
    instrs[32] = 0xe0+0x0d;		/* MIRP, don't set rp0, minimum, rounded, black */
    instrs[33] = 0x32;			/* SHP[rp2] (pt5 same height as pt4) */
    instrs[34] = 0xb2;			/* Pushb, 3byte */
    instrs[35] = 7;			/* Point 7 */
    instrs[36] = 6;			/* Point 6 */
    instrs[37] = getcvtval(gi,bp[6].y);	/* CVT entry for top height */
    instrs[38] = 0xe0+0x1c;		/* MIRP, set rp0, minimum, rounded, grey */
    instrs[39] = 0x3c;			/* ALIGNRP (pt7 same height as pt6) */
    instrs[40] = 0xb2;			/* Pushb, 3byte */
    instrs[41] = 1;			/* Point 1 */
    instrs[42] = 2;			/* Point 2 */
    instrs[43] = stemcvt;		/* CVT entry for our stem width */
    instrs[44] = 0xe0+0x0d;		/* MIRP, dont set rp0, minimum, rounded, black */
    instrs[45] = 0x32;			/* SHP[rp2] (pt1 same height as pt2) */

	/* We've touched all points in all dimensions */
	/* Don't need any IUP */

	/* end contours array */
    dumpshort(gi->glyphs,4-1);
    dumpshort(gi->glyphs,8-1);
	/* instruction length&instructions */
    dumpinstrs(gi,instrs,46);

    dumppointarrays(gi,bp,NULL,8);

    dumpshort(gi->hmtx,gh.xmax + 2*stem);
    dumpshort(gi->hmtx,stem);
}

static void dumpblankglyph(struct glyphinfo *gi,SplineFont *sf) {
    /* These don't get a glyph header, because there are no contours */
    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);
    dumpshort(gi->hmtx,gi->next_glyph==2?0:(sf->ascent+sf->descent)/3);
    dumpshort(gi->hmtx,0);
}

static void dumpspace(SplineChar *sc, struct glyphinfo *gi) {
    /* These don't get a glyph header, because there are no contours */
    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);
    dumpshort(gi->hmtx,sc->width);
    dumpshort(gi->hmtx,0);
}

static void dumpcomposit(SplineChar *sc, RefChar *refs, struct glyphinfo *gi) {
    struct glyphhead gh;
    DBounds bb;
    int i, ptcnt, ctcnt, contourtemp=0, pttemp=0, flags;
    SplineSet *ss;
    RefChar *ref;

    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);

    SplineCharFindBounds(sc,&bb);
    gh.numContours = -1;
    gh.xmin = floor(bb.minx); gh.ymin = floor(bb.miny);
    gh.xmax = ceil(bb.maxx); gh.ymax = ceil(bb.maxy);
    dumpghstruct(gi,&gh);

    i=ptcnt=ctcnt=0;
    for ( ref=refs; ref!=NULL; ref=ref->next, ++i ) {
	flags = (1<<1)|(1<<2);		/* Args are always values for me */
					/* Always round args to grid */
	if ( ref->next!=NULL )
	    flags |= (1<<5);		/* More components */
	if ( ref->transform[1]!=0 || ref->transform[2]!=0 )
	    flags |= (1<<7);		/* Need a full matrix */
	else if ( ref->transform[0]!=ref->transform[3] )
	    flags |= (1<<6);		/* different xy scales */
	else if ( ref->transform[0]!=1. )
	    flags |= (1<<3);		/* xy scale is same */
	if ( ref->transform[4]<-128 || ref->transform[4]>127 ||
		ref->transform[5]<-128 || ref->transform[5]>127 )
	    flags |= (1<<0);
	dumpshort(gi->glyphs,flags);
	dumpshort(gi->glyphs,ref->sc->ttf_glyph);
	if ( flags&(1<<0) ) {
	    dumpshort(gi->glyphs,(short)ref->transform[4]);
	    dumpshort(gi->glyphs,(short)ref->transform[5]);
	} else {
	    putc((char) (ref->transform[4]),gi->glyphs);
	    putc((char) (ref->transform[5]),gi->glyphs);
	}
	if ( flags&(1<<7) ) {
	    dump2d14(gi->glyphs,ref->transform[0]);
	    dump2d14(gi->glyphs,ref->transform[1]);
	    dump2d14(gi->glyphs,ref->transform[2]);
	    dump2d14(gi->glyphs,ref->transform[3]);
	} else if ( flags&(1<<6) ) {
	    dump2d14(gi->glyphs,ref->transform[0]);
	    dump2d14(gi->glyphs,ref->transform[3]);
	} else if ( flags&(1<<3) ) {
	    dump2d14(gi->glyphs,ref->transform[0]);
	}
	contourtemp = pttemp = 0;
	for ( ss=ref->splines; ss!=NULL ; ss=ss->next ) {
	    ++contourtemp;
	    pttemp += SSPointCnt(ss);
	}
    }
    if ( gi->maxp->maxnumcomponents<i ) gi->maxp->maxnumcomponents = i;
    gi->maxp->maxcomponentdepth = 1;
    if ( gi->maxp->maxCompositPts<pttemp ) gi->maxp->maxCompositPts=pttemp;
    if ( gi->maxp->maxCompositCtrs<contourtemp ) gi->maxp->maxCompositCtrs=contourtemp;

    dumpshort(gi->hmtx,sc->width);
    dumpshort(gi->hmtx,gh.xmin);
    if ( ftell(gi->glyphs)&1 )		/* Pad the file so that the next glyph */
	putc('\0',gi->glyphs);		/* on a word boundary, I think this can't happen here */
    RefCharsFree(refs);
}

static uint8 *pushpoint(uint8 *instrs,int pt) {
    if ( pt<256 ) {
	*instrs++ = 0xb0;
	*instrs++ = pt;
    } else {
	*instrs++ = 0xb8;
	*instrs++ = pt>>8;
	*instrs++ = pt&0xff;
    }
return( instrs );
}

static uint8 *pushpointstem(uint8 *instrs,int pt, int stem) {
    if ( pt<256 && stem<256 ) {
	*instrs++ = 0xb1;
	*instrs++ = pt;
	*instrs++ = stem;
    } else {
	*instrs++ = 0xb9;
	*instrs++ = pt>>8;
	*instrs++ = pt&0xff;
	*instrs++ = stem>>8;
	*instrs++ = stem&0xff;
    }
return( instrs );
}

/* Look for simple serifs (a vertical stem to the left or right of a hinted */
/*  stem). Serifs will not be hinted because they overlap the main stem. */
/*  To be a serif it must be a vertical stem that doesn't overlap (in y) the */
/*  hinted stem. It must be to the left of the left edge, or to the right of */
/*  the right edge of the hint. */
/* This is a pretty dumb detector, but it gets simple cases */
static uint8 *serifcheck(struct glyphinfo *gi, uint8 *instrs,StemInfo *hint,
	int *contourends, BasePoint *bp, int ptcnt, int pt, char *touched) {
    int up, right, prev;
    int i;
    int cs, ce;

    for ( i=0; contourends[i]!=0; ++i )
	if ( pt<=contourends[i] )
    break;
    cs = i==0?0:contourends[i-1]+1;
    ce = contourends[i];

    if ( pt>cs && bp[pt-1].x==bp[pt].x ) {
	prev = true;
	up = bp[pt-1].y<bp[pt].y;
    } else if ( pt==cs && bp[ce].x==bp[pt].x ) {
	prev = true;
	up = bp[ce].y<bp[pt].y;
    } else if ( pt<ce && bp[pt+1].x==bp[pt].x ) {
	prev = false;
	up = bp[pt+1].y<bp[pt].y;
    } else if ( pt==ce && bp[0].x==bp[pt].x ) {
	prev = false;
	up = bp[cs].y<bp[pt].y;
    } else		/* no vertical stem here, unlikely to be serifs */
return( instrs );

    right = (( hint->start==bp[pt].x && hint->width<0 ) ||
	    (hint->start+hint->width==bp[pt].x && hint->width>0 ));
    if ( prev ) {	/* the vstem comes before the point, serif must come after */
	for ( i=pt+1; i<=ce; ++i ) {
	    if ( (right && bp[i].x<=bp[i-1].x) || (!right && bp[i].x>=bp[i-1].x) ||
		    (up && bp[i].y<bp[i-1].y) || (!up && bp[i].y>bp[i-1].y))
return( instrs );
	    else if ( i<ce && bp[i].x==bp[i+1].x && 
		    ((up && bp[i].y<bp[i+1].y) || (!up && bp[i].y>bp[i+1].y))) {
		instrs = pushpointstem(instrs,i,getcvtval(gi,bp[i].x-bp[pt].x));
		*instrs++ = 0xe0+0x0d;	/* MIRP, minimum, rounded, black */
		touched[i] |= 1;
		instrs = pushpoint(instrs,i+1);
		*instrs++ = 0x32;	/* SHP, rp2 */
		touched[i+1] |= 1;
return( instrs );
	    }
	}
    } else {
	for ( i=pt-1; i>=cs; --i ) {
	    if ( (right && bp[i].x<=bp[i+1].x) || (!right && bp[i].x>=bp[i+1].x) ||
		    (up && bp[i].y<bp[i+1].y) || (!up && bp[i].y>bp[i+1].y))
return( instrs );
	    else if ( i>cs && bp[i].x==bp[i-1].x && 
		    ((up && bp[i].y<bp[i-1].y) || (!up && bp[i].y>bp[i-1].y))) {
		instrs = pushpointstem(instrs,i,getcvtval(gi,bp[i].x-bp[pt].x));
		*instrs++ = 0xe0+0x0d;	/* MIRP, minimum, rounded, black */
		touched[i] |= 1;
		instrs = pushpoint(instrs,i-1);
		*instrs++ = 0x32;	/* SHP, rp2 */
		touched[i+1] |= 1;
return( instrs );
	    }
	}
    }
return( instrs );
}

/* Run through all points. For any on this hint's start, position them */
/*  If the first point of this hint falls in a blue zone, do a cvt based */
/*	positioning, else
/*  The first point on the first hint is positioned to where it is (dull) */
/*  The first point of this hint is positioned offset from the last hint */
/*  Any other points are moved by the same amount as the last move */
/* Run through the points again, for any pts on this hint's end: */
/*  Position them offset from the hint's start */
/* Complications: */
/*  If the hint start isn't in a blue zone, but the hint end is, then */
/*	treat the end as the start with a negative width */
/*  If we have overlapping hints then either the points on the start or end */
/*	may already have been positioned. We won't be called if both have been*/
/*	positioned. If the end points have been positioned then reverse the */
/*	hint again. If either edge has been done, then all we need to do is */
/*	establish a reference point on that edge, don't need to position all */
/*	the points again. */
static uint8 *geninstrs(struct glyphinfo *gi, uint8 *instrs,StemInfo *hint,
	int *contourends, BasePoint *bp, int ptcnt, int first, int xdir,
	char *touched) {
    int i;
    int last= -1;
    int stem, basecvt=-1;
    real hbase, base, width;
    StemInfo *h;

    hbase = base = hint->start; width = hint->width;
    if ( !xdir ) {
	/* check the "bluevalues" for things like cap height and xheight */
	for ( i=0; i<gi->bcnt; i+=2 ) {
	    if ( base>=gi->blues[i] && base<=gi->blues[i+1] ) {
		base = (gi->blues[i]+gi->blues[i+1])/2;
		basecvt = getcvtval(gi,(int)base);
	break;
	    }
	}
	if ( basecvt == -1 && !hint->startdone ) {
	    hbase = (base += width);
	    for ( i=0; i<gi->bcnt; i+=2 ) {
		if ( base>=gi->blues[i] && base<=gi->blues[i+1] ) {
		    base = (gi->blues[i]+gi->blues[i+1])/2;
		    basecvt = getcvtval(gi,(int)base);
	    break;
		}
	    }
	    if ( basecvt!=-1 )
		width = -width;
	    else
		hbase = (base -= width);
	}
    }
    if ( width>0 && hint->enddone ) {
	hbase = (base += width);
	width = -width;
	basecvt = -1;
    }

    /* Position all points on this hint's base */
    if (( width>0 && !hint->startdone) || (width<0 && !hint->enddone)) {
	for ( i=0; i<ptcnt; ++i ) {
	    if ( (xdir && bp[i].x==hbase) || (!xdir && bp[i].y==hbase) ) {
		if ( 0 && last!=-1 && i==last+1 )
		    /* Don't need to touch this guy */;
		    /* Er, maybe, but if I do that my hinting doesn't work */
		    /* I don't understand how IUP works I guess */
		else if ( basecvt!=-1 && last==-1 ) {
		    instrs = pushpointstem(instrs,i,basecvt);
		    *instrs++ = 0x3f;		/* MIAP, rounded */
		} else {
		    instrs = pushpoint(instrs,i);
		    if ( first ) {
			/* set rp0 */
			*instrs++ = 0x2f;		/* MDAP, rounded */
			first = false;
		    } else if ( last==-1 ) {
			/* set rp0 relative to last hint */
			*instrs++ = 0xc0+0x1c;	/* MDRP, set rp0, minimum, rounded, grey */
		    } else {
			*instrs++ = 0x33;		/* SHP, rp1 */
		    }
		}
		touched[i] |= (xdir?1:2);
		if ( xdir )
		    instrs = serifcheck(gi,instrs,hint,contourends,bp,ptcnt,i,touched);
		last = i;
	    }
	}
	if ( last==-1 )		/* I'm confused. But if the hint doesn't start*/
return(instrs);			/*  anywhere, can't give it a width */
				/* Some hints aren't associated with points */
    } else {
	/* Need to find something to be a reference point, doesn't matter */
	/*  what. Note that it should already have been positioned */
	for ( i=0; i<ptcnt; ++i ) {
	    if ( (xdir && bp[i].x==hbase) || (!xdir && bp[i].y==hbase) ) {
		instrs = pushpoint(instrs,i);
		*instrs++ = 0x10;		/* Set RP0, SRP0 */
	break;
	    }
	}
    }

    /* Position all points on this hint's base+width */
    stem = getcvtval(gi,width);
    last = -1;
    for ( i=0; i<ptcnt; ++i ) {
	if ( (xdir && bp[i].x==hbase+width) || (!xdir && bp[i].y==hbase+width) ) {
	    if ( 0 && last!=-1 && i==last+1 )
		/* Don't need to touch this guy */;
	    else {
		instrs = pushpointstem(instrs,i,stem);
		*instrs++ = 0xe0+0x0d;	/* MIRP, minimum, rounded, black */
	    }
	    touched[i] |= (xdir?1:2);
	    if ( xdir )
		instrs = serifcheck(gi,instrs,hint,contourends,bp,ptcnt,i,touched);
	    last = i;
	}
    }

    for ( h=hint->next; h!=NULL && h->start<=hint->start+hint->width; h=h->next ) {
	if ( h->start==hint->start || h->start==hint->start+hint->width )
	    h->startdone = true;
	if ( h->start+h->width == hint->start+hint->width )
	    h->enddone = true;
    }

return( instrs );
}

/* diagonal stem hints */
static uint8 *gendinstrs(struct glyphinfo *gi,uint8 *pt,DStemInfo *dstem,
	BasePoint *bp,int ptcnt,char *touched) {
    int i, stemindex;
    int corners[4];
    real stemwidth, tempx, tempy, len;

    /* first get the indexes of the points that make up the diagonal */
    for ( i=0; i<4; ++i ) corners[i] =-1;
    for ( i=0; i<ptcnt; ++i ) {
	if ( bp[i].x==dstem->leftedgetop.x && bp[i].y==dstem->leftedgetop.y )
	    corners[0] = i;
	else if ( bp[i].x==dstem->rightedgetop.x && bp[i].y==dstem->rightedgetop.y )
	    corners[1] = i;
	else if ( bp[i].x==dstem->leftedgebottom.x && bp[i].y==dstem->leftedgebottom.y )
	    corners[2] = i;
	else if ( bp[i].x==dstem->rightedgebottom.x && bp[i].y==dstem->rightedgebottom.y )
	    corners[3] = i;
    }
    /* Must have found all four points for it to be worth doing anything */
    if ( corners[0]==-1 || corners[1]==-1 || corners[2]==-1 || corners[3]==-1 )
return( pt );

    /* set the projection vector perpendicular to the edge, freedom vector to x */
    pt = pushpointstem(pt,corners[0],corners[2]);	/* bad func name, we're pushing two points, but the effect is correct */
    *pt++ = 7;			/* SPVTL[orthog] */
    *pt++ = 5;			/* SFVTCA[x-axis] */

    /* find the orthogonal distance from the left stem to the right. Make it positive */
    /*  (just a dot product with the unit vector orthog to the left edge) */
    tempx = dstem->leftedgetop.x-dstem->leftedgebottom.x;
    tempy = dstem->leftedgetop.y-dstem->leftedgebottom.y;
    len = sqrt(tempx*tempx+tempy*tempy);
    stemwidth = ((dstem->rightedgetop.x-dstem->leftedgetop.x)*tempy -
	    (dstem->rightedgetop.y-dstem->leftedgetop.y)*tempx)/len;
    if ( stemwidth<0 ) stemwidth = -stemwidth;

    /* which of the two top points is most heavily positioned? */
    if ( (touched[corners[0]]&1) + (touched[corners[2]]&1) >=
	    (touched[corners[1]]&1) + (touched[corners[3]]&1) ) {
	/* Holding the left edge fixed, position the right */
	stemindex = getcvtval(gi,stemwidth);
	pt = pushpoint(pt,corners[0]);
	if ( touched[corners[0]]&1 )
	    *pt++ = 16;		/* SRP0 */
	else
	    *pt++ = 0x2f;	/* MDAP[1round] */
	pt = pushpointstem(pt,corners[1],stemindex);
	*pt++ = 0xed;		/* MIRP[01101] */
	pt = pushpoint(pt,corners[2]);
	if ( touched[corners[2]]&1 )
	    *pt++ = 16;		/* SRP0 */
	else
	    *pt++ = 0x2f;	/* MDAP[1round] */
	pt = pushpointstem(pt,corners[3],stemwidth);
	*pt++ = 0xed;		/* MIRP[01101] */
	touched[corners[1]] |= 1;
	touched[corners[3]] |= 1;
    } else {
	/* Holding the right edge fixed, position the left */
	stemindex = getcvtval(gi,-stemwidth);
	pt = pushpoint(pt,corners[1]);
	if ( touched[corners[1]]&1 )
	    *pt++ = 16;		/* SRP0 */
	else
	    *pt++ = 0x2f;	/* MDAP[1round] */
	pt = pushpointstem(pt,corners[0],stemindex);
	*pt++ = 0xed;		/* MIRP[01101] */
	pt = pushpoint(pt,corners[3]);
	if ( touched[corners[3]]&1 )
	    *pt++ = 16;		/* SRP0 */
	else
	    *pt++ = 0x2f;	/* MDAP[1round] */
	pt = pushpointstem(pt,corners[2],stemwidth);
	*pt++ = 0xed;		/* MIRP[01101] */
	touched[corners[0]] |= 1;
	touched[corners[2]] |= 1;
    }
    /* Diagonal (italic) serifs? */
return( pt );
}

static void dumpgeninstructions(SplineChar *sc, struct glyphinfo *gi,
	int *contourends, BasePoint *bp, int ptcnt) {
    StemInfo *hint;
    uint8 *instrs, *pt, *ystart;
    int max;
    char *touched;
    DStemInfo *dstem;

    if ( sc->vstem==NULL && sc->hstem==NULL && sc->dstem==NULL ) {
	dumpshort(gi->glyphs,0);		/* no instructions */
return;
    }
    /* Maximum instruction length is 6 bytes for each point in each dimension */
    /*  2 extra bytes to finish up. And one byte to switch from x to y axis */
    /* Diagonal take more space because we need to set the orientation on */
    /*  each stem
    /*  That should be an over-estimate */
    max=2;
    if ( sc->vstem!=NULL ) max += 6*ptcnt;
    if ( sc->hstem!=NULL ) max += 6*ptcnt+1;
    for ( dstem=sc->dstem; dstem!=NULL; max+=7+4*6, dstem=dstem->next );
    max += 6*ptcnt;			/* paranoia */
    instrs = pt = galloc(max);
    touched = gcalloc(1,ptcnt);

    for ( hint=sc->vstem; hint!=NULL; hint=hint->next )
	hint->enddone = hint->startdone = false;
    for ( hint=sc->vstem; hint!=NULL; hint=hint->next ) {
	if ( !hint->startdone || !hint->enddone )
	    pt = geninstrs(gi,pt,hint,contourends,bp,ptcnt,pt==instrs,true,touched);
    }
	
    if ( sc->hstem!=NULL ) {
	*pt++ = 0x00;	/* Set Vectors to y */
	ystart = pt;
	for ( hint=sc->hstem; hint!=NULL; hint=hint->next )
	    hint->enddone = hint->startdone = false;
	for ( hint=sc->hstem; hint!=NULL; hint=hint->next ) {
	    if ( !hint->startdone || !hint->enddone )
		pt = geninstrs(gi,pt,hint,contourends,bp,ptcnt,pt==ystart,false,touched);
	}
    }

    if ( sc->dstem!=NULL ) {
	for ( dstem = sc->dstem; dstem!=NULL; dstem=dstem->next )
	    pt = gendinstrs(gi,pt,dstem,bp,ptcnt,touched);
    }

    /* there seems some discention as to which of these does x and which does */
    /*  y. So rather than try and be clever, let's always do both */
    *pt++ = 0x30;	/* Interpolate Untouched Points y */
    *pt++ = 0x31;	/* Interpolate Untouched Points x */
    if ( pt-instrs > max ) {
	fprintf(stderr,"We're about to crash.\nWe miscalculated the glyph's instruction set length\n" );
	fprintf(stderr,"When processing TTF instructions (hinting) of %s\n", sc->name );
    }
    dumpinstrs(gi,instrs,pt-instrs);
    free(instrs);
}

static void dumpglyph(SplineChar *sc, struct glyphinfo *gi) {
    struct glyphhead gh;
    DBounds bb;
    SplineSet *ss;
    int contourcnt, ptcnt;
    int *contourends;
    BasePoint *bp;
    char *fs;
    RefChar *ref;

    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);

    if ( sc->changedsincelasthinted && !sc->manualhints )
	SplineCharAutoHint(sc,true);

    contourcnt = ptcnt = 0;
    for ( ss=sc->splines; ss!=NULL; ss=ss->next ) {
	++contourcnt;
	ptcnt += SSPointCnt(ss);
    }
    for ( ref=sc->refs; ref!=NULL; ref=ref->next ) {
	for ( ss=ref->splines; ss!=NULL; ss=ss->next ) {
	    ++contourcnt;
	    ptcnt += SSPointCnt(ss);
	}
    }

    SplineCharFindBounds(sc,&bb);
    gh.numContours = contourcnt;
    gh.xmin = floor(bb.minx); gh.ymin = floor(bb.miny);
    gh.xmax = ceil(bb.maxx); gh.ymax = ceil(bb.maxy);
    dumpghstruct(gi,&gh);
    if ( contourcnt>gi->maxp->maxContours ) gi->maxp->maxContours = contourcnt;
    if ( ptcnt>gi->maxp->maxPoints ) gi->maxp->maxPoints = ptcnt;
    contourends = galloc((contourcnt+1)*sizeof(int));

    bp = galloc(ptcnt*sizeof(BasePoint));
    fs = galloc(ptcnt);
    ptcnt = contourcnt = 0;
    for ( ss=sc->splines; ss!=NULL; ss=ss->next ) {
	ptcnt = SSAddPoints(ss,ptcnt,bp,fs);
	dumpshort(gi->glyphs,ptcnt-1);
	contourends[contourcnt++] = ptcnt-1;
    }
    for ( ref=sc->refs; ref!=NULL; ref=ref->next ) {
	for ( ss=ref->splines; ss!=NULL; ss=ss->next ) {
	    ptcnt = SSAddPoints(ss,ptcnt,bp,fs);
	    dumpshort(gi->glyphs,ptcnt-1);
	    contourends[contourcnt++] = ptcnt-1;
	}
    }
    contourends[contourcnt] = 0;
    dumpgeninstructions(sc,gi,contourends,bp,ptcnt);
    dumppointarrays(gi,bp,fs,ptcnt);
    free(bp);
    free(contourends);
    free(fs);

    dumpshort(gi->hmtx,sc->width);
    dumpshort(gi->hmtx,gh.xmin);
}

static void dumpglyphs(SplineFont *sf,struct glyphinfo *gi) {
    int i, cnt;
    RefChar *refs;

    GProgressChangeStages(2);
    FindBlues(sf,gi->blues,NULL);
    GProgressNextStage();

    for ( i=12; i>=0 && (gi->blues[i]!=0 || gi->blues[i+1]!=0) ; i-=2 );
    gi->bcnt = i+2;

    i=0, cnt=0;
    if ( sf->chars[0]!=NULL &&
	    (sf->chars[0]->splines!=NULL || sf->chars[0]->widthset) &&
	    sf->chars[0]->refs==NULL && strcmp(sf->chars[0]->name,".notdef")==0 )
	sf->chars[i++]->ttf_glyph = cnt++;
    for ( cnt=3; i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i]) )
	    sf->chars[i]->ttf_glyph = cnt++;

    gi->maxp->numGlyphs = cnt;
    gi->loca = galloc((gi->maxp->numGlyphs+1)*sizeof(uint32));
    gi->next_glyph = 0;
    gi->glyphs = tmpfile();
    gi->hmtx = tmpfile();

    i = 0;
    if ( sf->chars[0]!=NULL &&
	    (sf->chars[0]->splines!=NULL || sf->chars[0]->widthset) &&
	    sf->chars[0]->refs==NULL && strcmp(sf->chars[0]->name,".notdef")==0 )
	dumpglyph(sf->chars[i++],gi);
    else
	dumpmissingglyph(sf,gi);
    dumpblankglyph(gi,sf);	/* I'm not sure exactly why but there seem */
    dumpblankglyph(gi,sf);	/* to be a couple of blank glyphs at the start*/
    for ( cnt=0; i<sf->charcnt; ++i ) {
	if ( SCWorthOutputting(sf->chars[i]) ) {
	    if ( (refs = SCCanonicalRefs(sf->chars[i],false))!=NULL )
		dumpcomposit(sf->chars[i],refs,gi);
	    else if ( sf->chars[i]->splines==NULL && sf->chars[i]->refs==NULL )
		dumpspace(sf->chars[i],gi);
	    else
		dumpglyph(sf->chars[i],gi);
	}
	GProgressNext();
    }

    /* extra location entry points to end of last glyph */
    gi->loca[gi->next_glyph] = ftell(gi->glyphs);
    gi->glyph_len = ftell(gi->glyphs);
    gi->hmtxlen = ftell(gi->hmtx);
    /* pad out to four bytes */
    if ( gi->loca[gi->next_glyph]&3 ) {
	for ( i=4-(gi->loca[gi->next_glyph]&3); i>0; --i )
	    putc('\0',gi->glyphs);
    }
}

/* Standard names for cff */
extern const char *cffnames[];
extern const int nStdStrings;

static int storesid(struct alltabs *at,char *str) {
    int i;
    FILE *news;
    char *pt;
    long pos;

    if ( str!=NULL ) {			/* NULL is the magic string at end of array */
	for ( i=0; cffnames[i]!=NULL; ++i ) {
	    if ( strcmp(cffnames[i],str)==0 )
return( i );
	}
    }

    pos = ftell(at->sidf)+1;
    if ( pos>=65536 && !at->sidlongoffset ) {
	at->sidlongoffset = true;
	news = tmpfile();
	rewind(at->sidh);
	for ( i=0; i<at->sidcnt; ++i )
	    dumplong(news,getushort(at->sidh));
	fclose(at->sidh);
	at->sidh = news;
    }
    if ( at->sidlongoffset )
	dumplong(at->sidh,pos);
    else
	dumpshort(at->sidh,pos);

    if ( str!=NULL ) {
	for ( pt=str; *pt; ++pt )
	    putc(*pt,at->sidf);
    }
return( at->sidcnt++ + nStdStrings );
}

static void dumpint(FILE *cfff,int num) {

    if ( num>=-107 && num<=107 )
	putc(num+139,cfff);
    else if ( num>=108 && num<=1131 ) {
	num -= 108;
	putc((num>>8)+247,cfff);
	putc(num&0xff,cfff);
    } else if ( num>=-1131 && num<=-108 ) {
	num = -num;
	num -= 108;
	putc((num>>8)+251,cfff);
	putc(num&0xff,cfff);
    } else if ( num>=-32768 && num<32768 ) {
	putc(28,cfff);
	putc(num>>8,cfff);
	putc(num&0xff,cfff);
    } else {		/* In dict data we have 4 byte ints, in type2 strings we don't */
	putc(29,cfff);
	putc((num>>24)&0xff,cfff);
	putc((num>>16)&0xff,cfff);
	putc((num>>8)&0xff,cfff);
	putc(num&0xff,cfff);
    }
}

static void dumpdbl(FILE *cfff,double d) {
    if ( d-rint(d)>-.00001 && d-rint(d)<.00001 )
	dumpint(cfff,(int) d);
    else {
	/* The type2 strings have a fixed format, but the dict data does not */
	char buffer[20], *pt;
	int sofar,n;
	sprintf( buffer, "%g", d);
	sofar = 0;
	putc(30,cfff);		/* Start a double */
	for ( pt=buffer; *pt; ++pt ) {
	    if ( isdigit(*pt) )
		n = *pt-'0';
	    else if ( *pt=='.' )
		n = 0xa;
	    else if ( *pt=='-' )
		n = 0xe;
	    else if (( *pt=='E' || *pt=='e') && pt[1]=='-' ) {
		n = 0xc;
		++pt;
	    } else if ( *pt=='E' || *pt=='e')
		n = 0xb;
	    if ( sofar==0 )
		sofar = n<<4;
	    else {
		putc(sofar|n,cfff);
		sofar=0;
	    }
	}
	if ( sofar==0 )
	    putc(0xff,cfff);
	else
	    putc(sofar|0xf,cfff);
    }
}

static void dumpoper(FILE *cfff,int oper ) {
    if ( oper!=-1 ) {
	if ( oper>=256 )
	    putc(oper>>8,cfff);
	putc(oper&0xff,cfff);
    }
}

static void dumpdbloper(FILE *cfff,double d, int oper ) {
    dumpdbl(cfff,d);
    dumpoper(cfff,oper);
}

static void dumpintoper(FILE *cfff,int v, int oper ) {
    dumpint(cfff,v);
    dumpoper(cfff,oper);
}

static void dumpsizedint(FILE *cfff,int big,int num, int oper ) {
    if ( big ) {
	putc(29,cfff);
	putc((num>>24)&0xff,cfff);
	putc((num>>16)&0xff,cfff);
	putc((num>>8)&0xff,cfff);
	putc(num&0xff,cfff);
    } else {
	putc(28,cfff);
	putc(num>>8,cfff);
	putc(num&0xff,cfff);
    }
    dumpoper(cfff,oper);
}

static void dumpsid(FILE *cfff,struct alltabs *at,char *str,int oper) {
    if ( str==NULL )
return;
    dumpint(cfff,storesid(at,str));
    dumpoper(cfff,oper);
}

static void DumpStrDouble(char *pt,FILE *cfff,int oper) {
    real d = strtod(pt,NULL);
    dumpdbloper(cfff,d,oper);
}

static void DumpDblArray(real *arr,int n,FILE *cfff, int oper) {
    int mi,i;

    for ( mi=n-1; mi>=0 && arr[mi]==0; --mi );
    if ( mi<0 )
return;
    dumpdbl(cfff,arr[0]);
    for ( i=1; i<=mi; ++i )
	dumpdbl(cfff,arr[i]-arr[i-1]);
    dumpoper(cfff,oper);
}

static void DumpStrArray(char *pt,FILE *cfff,int oper) {
    real d, last=0;
    char *end;

    while ( *pt==' ' ) ++pt;
    if ( *pt=='\0' )
return;
    if ( *pt=='[' ) ++pt;
    while ( *pt==' ' ) ++pt;
    while ( *pt!=']' && *pt!='\0' ) {
	d = strtod(pt,&end);
	if ( pt==end )		/* User screwed up. Should be a number */
    break;
	dumpdbl(cfff,d-last);
	last = d;
	pt = end;
	while ( *pt==' ' ) ++pt;
    }
    dumpoper(cfff,oper);
}

static void dumpcffheader(SplineFont *sf,FILE *cfff) {
    putc('\1',cfff);		/* Major version: 1 */
    putc('\0',cfff);		/* Minor version: 0 */
    putc('\4',cfff);		/* Header size in bytes */
    putc('\4',cfff);		/* Absolute Offset size. */
	/* I don't think there are any absolute offsets that aren't encoded */
	/*  in a dict as numbers (ie. inherently variable sized items) */
}

static void dumpcffnames(SplineFont *sf,FILE *cfff) {
    char *pt;

    dumpshort(cfff,1);		/* One font name */
    putc('\1',cfff);		/* Offset size */
    putc('\1',cfff);		/* Offset to first name */
    putc('\1'+strlen(sf->fontname),cfff);
    for ( pt=sf->fontname; *pt; ++pt )
	putc(*pt,cfff);
}

static void dumpcffcharset(SplineFont *sf,struct alltabs *at) {
    int i;

    putc(0,at->charset);
    /* I always use a format 0 charset. ie. an array of SIDs in random order */

    /* First element must be ".notdef" and is omitted */
    /* So if glyph 0 isn't notdef do something special */
    if ( SCWorthOutputting(sf->chars[0]) && strcmp(sf->chars[0]->name,".notdef")!=0 )
	dumpshort(at->charset,storesid(at,sf->chars[0]->name));

    for ( i=1; i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i]) )
	    dumpshort(at->charset,storesid(at,sf->chars[i]->name));
}

static void dumpcffcidset(SplineFont *sf,struct alltabs *at) {
    int cid, k, max=0, start;

    putc(2,at->charset);

    for ( k=0; k<sf->subfontcnt; ++k )
	if ( sf->subfonts[k]->charcnt>max ) max = sf->subfonts[k]->charcnt;

    start = -1;			/* Glyph 0 always maps to CID 0, and is omitted */
    for ( cid = 1; cid<max; ++cid ) {
	for ( k=0; k<sf->subfontcnt; ++k ) {
	    if ( cid<sf->subfonts[k]->charcnt &&
		SCWorthOutputting(sf->subfonts[k]->chars[cid]) )
	break;
	}
	if ( k==sf->subfontcnt ) {
	    if ( start!=-1 ) {
		dumpshort(at->charset,start);
		dumpshort(at->charset,cid-1-start);	/* Count of glyphs in range excluding first */
		start = -1;
	    }
	} else {
	    if ( start==-1 ) start = cid;
	}
    }
    if ( start!=-1 ) {
	dumpshort(at->charset,start);
	dumpshort(at->charset,cid-1-start);	/* Count of glyphs in range excluding first */
    }
}

static void dumpcfffdselect(SplineFont *sf,struct alltabs *at) {
    int cid, k, max=0, lastfd, cnt;
    int gid;

    putc(3,at->fdselect);
    dumpshort(at->fdselect,0);		/* number of ranges, fill in later */

    for ( k=0; k<sf->subfontcnt; ++k )
	if ( sf->subfonts[k]->charcnt>max ) max = sf->subfonts[k]->charcnt;

    for ( k=0; k<sf->subfontcnt; ++k )
	if ( SCWorthOutputting(sf->subfonts[k]->chars[0]))
    break;
    if ( k==sf->subfontcnt ) --k;	/* If CID 0 not defined, put it in last font */
    dumpshort(at->fdselect,0);
    putc(k,at->fdselect);
    lastfd = k;
    cnt = 1;
    for ( cid = gid = 1; cid<max; ++cid ) {
	for ( k=0; k<sf->subfontcnt; ++k ) {
	    if ( cid<sf->subfonts[k]->charcnt &&
		SCWorthOutputting(sf->subfonts[k]->chars[cid]) )
	break;
	}
	if ( k==sf->subfontcnt )
	    /* Doesn't map to a glyph, irrelevant */;
	else {
	    if ( k!=lastfd ) {
		dumpshort(at->fdselect,gid);
		putc(k,at->fdselect);
		lastfd = k;
		++cnt;
	    }
	    ++gid;
	}
    }
    dumpshort(at->fdselect,gid);
    fseek(at->fdselect,1,SEEK_SET);
    dumpshort(at->fdselect,cnt);
    fseek(at->fdselect,0,SEEK_END);
}

static void dumpcffencoding(SplineFont *sf,struct alltabs *at) {
    int i,offset, pos;

    putc(0,at->encoding);
    /* I always use a format 0 encoding. ie. an array of glyph indexes */
    putc(0xff,at->encoding);
    /* And I put in 255 of them (with the encoding for 0 implied, I hope) */

    offset = 0;
    if ( SCWorthOutputting(sf->chars[0]) && strcmp(sf->chars[0]->name,".notdef")!=0 )
	offset = 1;

    for ( i=pos=1; i<sf->charcnt && i<256; ++i )
	if ( SCWorthOutputting(sf->chars[i]) )
	    putc(pos++ +offset,at->encoding);
	else
	    putc(0,at->encoding);
    for ( ; i<256; ++i )
	putc(0,at->encoding);
}

static void _dumpcffstrings(FILE *file, struct pschars *strs) {
    int i, len, offsize;

    /* First figure out the offset size */
    len = 1;
    for ( i=0; i<strs->cnt; ++i )
	len += strs->lens[i];

    /* Then output the index size and offsets */
    dumpshort( file, strs->cnt );
    if ( strs->cnt!=0 ) {
	offsize = len<=255?1:len<=65535?2:len<=0xffffff?3:4;
	putc(offsize,file);
	len = 1;
	for ( i=0; i<strs->cnt; ++i ) {
	    dumpoffset(file,offsize,len);
	    len += strs->lens[i];
	}
	dumpoffset(file,offsize,len);

	/* last of all the strings */
	for ( i=0; i<strs->cnt; ++i ) {
	    uint8 *pt = strs->values[i], *end = pt+strs->lens[i];
	    while ( pt<end )
		putc( *pt++, file );
	}
    }
}

static FILE *dumpcffstrings(struct pschars *strs) {
    FILE *file = tmpfile();
    _dumpcffstrings(file,strs);
    PSCharsFree(strs);
return( file );
}

static void dumpcffprivate(SplineFont *sf,struct alltabs *at,int subfont) {
    char *pt;
    FILE *private = subfont==-1?at->private:at->fds[subfont].private;
    int mi,i,j,cnt, allsame=true, sameval = 0x8000000;
    real bluevalues[14], otherblues[10];
    real snapcnt[12];
    real stemsnaph[12], stemsnapv[12];
    real stdhw[1], stdvw[1];
    int hasblue=0, hash=0, hasv=0, bs;
    int maxw=0;
    uint16 *widths; uint32 *cumwid;
    int nomwid, defwid;

    /* The private dict is not in an index, so no index header. Just the data */

    for ( i=0; i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i]) ) {
	    if ( maxw<sf->chars[i]->width ) maxw = sf->chars[i]->width;
	    if ( sameval == 0x8000000 )
		sameval = sf->chars[i]->width;
	    else if ( sameval!=sf->chars[i]->width )
		allsame = false;
	}
    if ( allsame ) {
	nomwid = defwid = sameval;
    } else {
	++maxw;
	widths = gcalloc(maxw,sizeof(uint16));
	cumwid = gcalloc(maxw,sizeof(uint32));
	defwid = 0; cnt=0;
	for ( i=0; i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]->width>=0)
		if ( ++widths[sf->chars[i]->width] > cnt ) {
		    defwid = sf->chars[i]->width;
		    cnt = widths[defwid];
		}
	widths[defwid] = 0;
	for ( i=0; i<maxw; ++i )
		for ( j=-107; j<=107; ++j )
		    if ( i+j>=0 && i+j<maxw )
			cumwid[i] += widths[i+j];
	cnt = 0; nomwid = 0;
	for ( i=0; i<maxw; ++i )
	    if ( cnt<cumwid[i] ) {
		cnt = cumwid[i];
		nomwid = i;
	    }
	free(widths); free(cumwid);
    }
    dumpintoper(private,defwid,20);		/* Default Width */
    if ( subfont==-1 )
	at->defwid = defwid;
    else
	at->fds[subfont].defwid = defwid;
    dumpintoper(private,nomwid,21);		/* Nominative Width */
    if ( subfont==-1 )
	at->nomwid = nomwid;
    else
	at->fds[subfont].nomwid = nomwid;

    bs = SplineFontIsFlexible(sf);
    hasblue = PSDictHasEntry(sf->private,"BlueValues")!=NULL;
    hash = PSDictHasEntry(sf->private,"StdHW")!=NULL;
    hasv = PSDictHasEntry(sf->private,"StdVW")!=NULL;
    GProgressChangeStages(3+!hasblue);
    GProgressChangeLine1R(_STR_AutoHintingFont);
    SplineFontAutoHint(sf);
    GProgressNextStage();

    if ( !hasblue ) {
	FindBlues(sf,bluevalues,otherblues);
	GProgressNextStage();
    }

    stdhw[0] = stdvw[0] = 0;
    if ( !hash ) {
	FindHStems(sf,stemsnaph,snapcnt);
	mi = -1;
	for ( i=0; stemsnaph[i]!=0 && i<12; ++i )
	    if ( mi==-1 ) mi = i;
	    else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
	if ( mi!=-1 ) stdhw[0] = stemsnaph[mi];
    }

    if ( !hasv ) {
	FindVStems(sf,stemsnapv,snapcnt);
	mi = -1;
	for ( i=0; stemsnapv[i]!=0 && i<12; ++i )
	    if ( mi==-1 ) mi = i;
	    else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
	if ( mi!=-1 ) stdvw[0] = stemsnapv[mi];
    }
    GProgressChangeLine1R(_STR_SavingOpenTypeFont);

    if ( hasblue )
	DumpStrArray(PSDictHasEntry(sf->private,"BlueValues"),private,6);
    else
	DumpDblArray(bluevalues,sizeof(bluevalues)/sizeof(bluevalues[0]),private,6);
    if ( (pt=PSDictHasEntry(sf->private,"OtherBlues"))!=NULL )
	DumpStrArray(pt,private,7);
    else if ( !hasblue )
	DumpDblArray(otherblues,sizeof(otherblues)/sizeof(otherblues[0]),private,7);
    if ( (pt=PSDictHasEntry(sf->private,"FamilyBlues"))!=NULL )
	DumpStrArray(pt,private,8);
    if ( (pt=PSDictHasEntry(sf->private,"FamilyOtherBlues"))!=NULL )
	DumpStrArray(pt,private,9);
    if ( (pt=PSDictHasEntry(sf->private,"BlueScale"))!=NULL )
	DumpStrDouble(pt,private,(12<<8)+9);
    if ( (pt=PSDictHasEntry(sf->private,"BlueShift"))!=NULL )
	DumpStrDouble(pt,private,(12<<8)+10);
    else
	dumpintoper(private,bs,(12<<8)+10);
    if ( (pt=PSDictHasEntry(sf->private,"BlueFuzz"))!=NULL )
	DumpStrDouble(pt,private,(12<<8)+11);
    if ( hash ) {
	DumpStrDouble(PSDictHasEntry(sf->private,"StdHW"),private,10);
	if ( (pt=PSDictHasEntry(sf->private,"StemSnapH"))!=NULL )
	    DumpStrArray(pt,private,(12<<8)|12);
    } else {
	if ( stdhw[0]!=0 )
	    dumpdbloper(private,stdhw[0],10);
	DumpDblArray(stemsnaph,sizeof(stemsnaph)/sizeof(stemsnaph[0]),private,(12<<8)|12);
    }
    if ( hasv ) {
	DumpStrDouble(PSDictHasEntry(sf->private,"StdVW"),private,11);
	if ( (pt=PSDictHasEntry(sf->private,"StemSnapV"))!=NULL )
	    DumpStrArray(pt,private,(12<<8)|13);
    } else {
	if ( stdvw[0]!=0 )
	    dumpdbloper(private,stdvw[0],11);
	DumpDblArray(stemsnapv,sizeof(stemsnapv)/sizeof(stemsnapv[0]),private,(12<<8)|13);
    }
    if ( (pt=PSDictHasEntry(sf->private,"ForceBold"))!=NULL ) {
	dumpintoper(private,*pt=='t'||*pt=='T',(12<<8)|14);
    } else if ( sf->weight!=NULL &&
	    (strstrmatch(sf->weight,"Bold")!=NULL ||
	     strstrmatch(sf->weight,"Black")!=NULL))
	dumpintoper(private,1,(12<<8)|14);
    if ( (pt=PSDictHasEntry(sf->private,"LanguageGroup"))!=NULL )
	DumpStrDouble(pt,private,(12<<8)+17);
    else if ( sf->encoding_name>=em_first2byte && sf->encoding_name<em_unicode )
	dumpintoper(private,1,(12<<8)|17);
    if ( (pt=PSDictHasEntry(sf->private,"ExpansionFactor"))!=NULL )
	DumpStrDouble(pt,private,(12<<8)+18);
    dumpsizedint(private,false,ftell(private)+3+1,19);	/* Subrs */

    if ( subfont==-1 )
	at->privatelen = ftell(private);
    else
	at->fds[subfont].privatelen = ftell(private);
}

/* When we exit this the topdict is not complete, we still need to fill in */
/*  values for charset,encoding,charstrings and private. Then we need to go */
/*  back and fill in the table length (at lenpos) */
static void dumpcfftopdict(SplineFont *sf,struct alltabs *at) {
    char *pt, *end;
    FILE *cfff = at->cfff;
    DBounds b;

    dumpshort(cfff,1);		/* One top dict */
    putc('\2',cfff);		/* Offset size */
    dumpshort(cfff,1);		/* Offset to topdict */
    at->lenpos = ftell(cfff);
    dumpshort(cfff,0);		/* placeholder for final position (final offset in index points beyond last element) */
    dumpsid(cfff,at,sf->version,0);
    dumpsid(cfff,at,sf->copyright,1);
    dumpsid(cfff,at,sf->fullname,2);
    dumpsid(cfff,at,sf->familyname,3);
    dumpsid(cfff,at,sf->weight,4);
    if ( SFOneWidth(sf)) dumpintoper(cfff,1,(12<<8)|1);
    if ( sf->italicangle!=0 ) dumpdbloper(cfff,sf->italicangle,(12<<8)|2);
    if ( sf->upos!=-100 ) dumpdbloper(cfff,sf->upos,(12<<8)|3);
    if ( sf->uwidth!=50 ) dumpdbloper(cfff,sf->uwidth,(12<<8)|4);
    /* We'll never set painttype */
    /* We'll never set CharstringType */
    if ( sf->ascent+sf->descent!=1000 ) {
	dumpdbl(cfff,1.0/(sf->ascent+sf->descent));
	dumpint(cfff,0);
	dumpint(cfff,0);
	dumpdbl(cfff,1.0/(sf->ascent+sf->descent));
	dumpint(cfff,0);
	dumpintoper(cfff,0,(12<<8)|7);
    }
    dumpintoper(cfff, sf->uniqueid?sf->uniqueid:4000000 + (rand()&0x3ffff), 13 );
    SplineFontFindBounds(sf,&b);
    at->gi.xmin = b.minx;
    at->gi.ymin = b.miny;
    at->gi.xmax = b.maxx;
    at->gi.ymax = b.maxy;
    dumpdbl(cfff,floor(b.minx));
    dumpdbl(cfff,floor(b.miny));
    dumpdbl(cfff,ceil(b.maxx));
    dumpdbloper(cfff,ceil(b.maxy),5);
    /* We'll never set StrokeWidth */
    if ( sf->xuid!=NULL ) {
	pt = sf->xuid; if ( *pt=='[' ) ++pt;
	while ( *pt && *pt!=']' ) {
	    dumpint(cfff,strtol(pt,&end,10));
	    for ( pt = end; *pt==' '; ++pt );
	}
	putc(14,cfff);
	SFIncrementXUID(sf);
    }
    /* Offset to charset (oper=15) needed here */
    /* Offset to encoding (oper=16) needed here (not for CID )*/
    /* Offset to charstrings (oper=17) needed here */
    /* Length of, and Offset to private (oper=18) needed here (not for CID )*/
}

static int dumpcffdict(SplineFont *sf,struct alltabs *at) {
    FILE *fdarray = at->fdarray;
    int pstart;
    /* according to the PSRef Man v3, only fontname, fontmatrix and private */
    /*  appear in this dictionary */

    dumpsid(fdarray,at,sf->fontname,(12<<8)|38);
    if ( sf->ascent+sf->descent!=1000 ) {
	dumpdbl(fdarray,1.0/(sf->ascent+sf->descent));
	dumpint(fdarray,0);
	dumpint(fdarray,0);
	dumpdbl(fdarray,1.0/(sf->ascent+sf->descent));
	dumpint(fdarray,0);
	dumpintoper(fdarray,0,(12<<8)|7);
    }
    pstart = ftell(fdarray);
    dumpsizedint(fdarray,false,0,-1);	/* private length */
    dumpsizedint(fdarray,true,0,18);	/* private offset */
return( pstart );
}

static void dumpcffdictindex(SplineFont *sf,struct alltabs *at) {
    int i;
    int pos;

    dumpshort(at->fdarray,sf->subfontcnt);
    putc('\2',at->fdarray);		/* DICTs aren't very big, and there are at most 255 */\
    dumpshort(at->fdarray,1);		/* Offset to first dict */
    for ( i=0; i<sf->subfontcnt; ++i )
	dumpshort(at->fdarray,0);	/* Dump offset placeholders (note there's one extra to mark the end) */
    pos = ftell(at->fdarray)-1;
    for ( i=0; i<sf->subfontcnt; ++i ) {
	at->fds[i].fillindictmark = dumpcffdict(sf->subfonts[i],at);
	at->fds[i].eodictmark = ftell(at->fdarray);
	if ( at->fds[i].eodictmark>65536 )
	    GDrawIError("The DICT INDEX got too big, result won't work");
    }
    fseek(at->fdarray,2*sizeof(short)+sizeof(char),SEEK_SET);
    for ( i=0; i<sf->subfontcnt; ++i )
	dumpshort(at->fdarray,at->fds[i].eodictmark-pos);
    fseek(at->fdarray,0,SEEK_END);
}

static void dumpcffcidtopdict(SplineFont *sf,struct alltabs *at) {
    char *pt, *end;
    FILE *cfff = at->cfff;
    DBounds b;
    int cidcnt=0, k;

    for ( k=0; k<sf->subfontcnt; ++k )
	if ( sf->subfonts[k]->charcnt>cidcnt ) cidcnt = sf->subfonts[k]->charcnt;

    dumpshort(cfff,1);		/* One top dict */
    putc('\2',cfff);		/* Offset size */
    dumpshort(cfff,1);		/* Offset to topdict */
    at->lenpos = ftell(cfff);
    dumpshort(cfff,0);		/* placeholder for final position */
    dumpsid(cfff,at,sf->cidregistry,-1);
    dumpsid(cfff,at,sf->ordering,-1);
    dumpintoper(cfff,sf->supplement,(12<<8)|30);		/* ROS operator must be first */
    dumpdbloper(cfff,sf->cidversion,(12<<8)|31);
    dumpintoper(cfff,cidcnt,(12<<8)|34);
    dumpintoper(cfff, sf->uniqueid?sf->uniqueid:4000000 + (rand()&0x3ffff), (12<<8)|35 );

    dumpsid(cfff,at,sf->copyright,1);
    dumpsid(cfff,at,sf->fullname,2);
    dumpsid(cfff,at,sf->familyname,3);
    dumpsid(cfff,at,sf->weight,4);
    /* FontMatrix  (identity here, real ones in sub fonts)*/
    /* Actually there is no fontmatrix in the adobe cid font I'm looking at */
    /*  which means it should default to [.001...] but it doesn't so the */
    /*  docs aren't completely accurate */
#if 0
    dumpdbl(cfff,1.0);
    dumpint(cfff,0);
    dumpint(cfff,0);
    dumpdbl(cfff,1.0);
    dumpint(cfff,0);
    dumpintoper(cfff,0,(12<<8)|7);
#endif

    CIDFindBounds(sf,&b);
    at->gi.xmin = b.minx;
    at->gi.ymin = b.miny;
    at->gi.xmax = b.maxx;
    at->gi.ymax = b.maxy;
    dumpdbl(cfff,floor(b.minx));
    dumpdbl(cfff,floor(b.miny));
    dumpdbl(cfff,ceil(b.maxx));
    dumpdbloper(cfff,ceil(b.maxy),5);
    /* We'll never set StrokeWidth */
    if ( sf->xuid!=NULL ) {
	pt = sf->xuid; if ( *pt=='[' ) ++pt;
	while ( *pt && *pt!=']' ) {
	    dumpint(cfff,strtol(pt,&end,10));
	    for ( pt = end; *pt==' '; ++pt );
	}
	putc(14,cfff);
	SFIncrementXUID(sf);
    }
    dumpint(cfff,0);			/* Docs say a private dict is required and they don't specifically omit CID top dicts */
    dumpintoper(cfff,0,18);		/* But they do say it can be zero */
    /* Offset to charset (oper=15) needed here */
    /* Offset to charstrings (oper=17) needed here */
    /* Offset to FDArray (oper=12,36) needed here */
    /* Offset to FDSelect (oper=12,37) needed here */
}

static void finishup(SplineFont *sf,struct alltabs *at) {
    int strlen, shlen, glen,enclen,csetlen,cstrlen,prvlen;
    int base, eotop, strhead;

    storesid(at,NULL);		/* end the strings index */
    strlen = ftell(at->sidf) + (shlen = ftell(at->sidh));
    glen = sizeof(short);	/* Single entry: 0, no globals */
    enclen = ftell(at->encoding);
    csetlen = ftell(at->charset);
    cstrlen = ftell(at->charstrings);
    prvlen = ftell(at->private);
    base = ftell(at->cfff);
    if ( base+6*3+strlen+glen+enclen+csetlen+cstrlen+prvlen > 32767 ) {
	at->cfflongoffset = true;
	base += 5*5+4;
    } else
	base += 5*3+4;
    strhead = 2+(at->sidcnt>1);
    base += strhead;

    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen,15);
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen,16);
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen+enclen,17);
    dumpsizedint(at->cfff,at->cfflongoffset,at->privatelen,-1);
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen+enclen+cstrlen,18);
    eotop = base-strhead-at->lenpos-1;
    if ( at->cfflongoffset ) {
	fseek(at->cfff,3,SEEK_SET);
	putc(4,at->cfff);
    }
    fseek(at->cfff,at->lenpos,SEEK_SET);
    dumpshort(at->cfff,eotop);
    fseek(at->cfff,0,SEEK_END);

    /* String Index */
    dumpshort(at->cfff,at->sidcnt-1);
    if ( at->sidcnt!=1 ) {		/* Everybody gets an added NULL */
	putc(at->sidlongoffset?4:2,at->cfff);
	copyfile(at->cfff,at->sidh,base);
	copyfile(at->cfff,at->sidf,base+shlen);
    }

    /* Global Subrs */
    dumpshort(at->cfff,0);

    /* Charset */
    copyfile(at->cfff,at->charset,base+strlen+glen);

    /* Encoding */
    copyfile(at->cfff,at->encoding,base+strlen+glen+csetlen);

    /* Char Strings */
    copyfile(at->cfff,at->charstrings,base+strlen+glen+csetlen+enclen);

    /* Private & Subrs */
    copyfile(at->cfff,at->private,base+strlen+glen+csetlen+enclen+cstrlen);
}

static void finishupcid(SplineFont *sf,struct alltabs *at) {
    int strlen, shlen, glen,csetlen,cstrlen,fdsellen,fdarrlen,prvlen;
    int base, eotop, strhead;
    int i;

    storesid(at,NULL);		/* end the strings index */
    strlen = ftell(at->sidf) + (shlen = ftell(at->sidh));
    glen = sizeof(short);	/* Single entry: 0, no globals */
    /* No encodings */
    csetlen = ftell(at->charset);
    fdsellen = ftell(at->fdselect);
    cstrlen = ftell(at->charstrings);
    fdarrlen = ftell(at->fdarray);
    base = ftell(at->cfff);

    at->cfflongoffset = true;
    base += 5*4+4+2;		/* two of the opers below are two byte opers */
    strhead = 2+(at->sidcnt>1);
    base += strhead;

    prvlen = 0;
    for ( i=0; i<sf->subfontcnt; ++i ) {
	fseek(at->fdarray,at->fds[i].fillindictmark,SEEK_SET);
	dumpsizedint(at->fdarray,false,at->fds[i].privatelen,-1);	/* Private len */
	dumpsizedint(at->fdarray,true,base+strlen+glen+csetlen+fdsellen+cstrlen+fdarrlen+prvlen,18);	/* Private offset */
	prvlen += ftell(at->fds[i].private);	/* private & subrs */
    }

    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen,15);	/* charset */
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen,(12<<8)|37);	/* fdselect */
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen+fdsellen,17);	/* charstrings */
    dumpsizedint(at->cfff,at->cfflongoffset,base+strlen+glen+csetlen+fdsellen+cstrlen,(12<<8)|36);	/* fdarray */
    eotop = base-strhead-at->lenpos-1;
    fseek(at->cfff,at->lenpos,SEEK_SET);
    dumpshort(at->cfff,eotop);
    fseek(at->cfff,0,SEEK_END);

    /* String Index */
    dumpshort(at->cfff,at->sidcnt-1);
    if ( at->sidcnt!=1 ) {		/* Everybody gets an added NULL */
	putc(at->sidlongoffset?4:2,at->cfff);
	copyfile(at->cfff,at->sidh,base);
	copyfile(at->cfff,at->sidf,base+shlen);
    }

    /* Global Subrs */
    dumpshort(at->cfff,0);

    /* Charset */
    copyfile(at->cfff,at->charset,base+strlen+glen);

    /* FDSelect */
    copyfile(at->cfff,at->fdselect,base+strlen+glen+csetlen);

    /* Char Strings */
    copyfile(at->cfff,at->charstrings,base+strlen+glen+csetlen+fdsellen);

    /* FDArray (DICT Index) */
    copyfile(at->cfff,at->fdarray,base+strlen+glen+csetlen+fdsellen+cstrlen);

    /* Private & Subrs */
    prvlen = 0;
    for ( i=0; i<sf->subfontcnt; ++i ) {
	int temp = ftell(at->fds[i].private);
	copyfile(at->cfff,at->fds[i].private,
		base+strlen+glen+csetlen+fdsellen+cstrlen+fdarrlen+prvlen);
	prvlen += temp;
    }

    free(at->fds);
}

static void dumpcffhmtx(struct alltabs *at,SplineFont *sf) {
    DBounds b;
    SplineChar *sc;
    int i,cnt;

    at->gi.hmtx = tmpfile();
    if ( SCWorthOutputting(sf->chars[0]) && strcmp(sf->chars[0]->name,".notdef")==0 ) {
	dumpshort(at->gi.hmtx,sf->chars[0]->width);
	SplineCharFindBounds(sf->chars[0],&b);
	dumpshort(at->gi.hmtx,b.minx);
	i = 1;
    } else {
	i = 0;
	dumpshort(at->gi.hmtx,sf->ascent+sf->descent);
	dumpshort(at->gi.hmtx,0);
    }
    cnt = 1;
    for ( i=1; i<sf->charcnt; ++i ) {
	sc = sf->chars[i];
	if ( SCWorthOutputting(sc)) {
	    dumpshort(at->gi.hmtx,sc->width);
	    SplineCharFindBounds(sc,&b);
	    dumpshort(at->gi.hmtx,b.minx);
	    ++cnt;
	}
    }
    at->gi.hmtxlen = ftell(at->gi.hmtx);
    /* we don't need to align this table, things get written aligned */

    at->gi.maxp->numGlyphs = cnt;
}

static void dumpcffcidhmtx(struct alltabs *at,SplineFont *_sf) {
    DBounds b;
    SplineChar *sc;
    int cid,i,cnt=0,max;
    SplineFont *sf;

    at->gi.hmtx = tmpfile();
    max = 0;
    for ( i=0; i<_sf->subfontcnt; ++i )
	if ( max<_sf->subfonts[i]->charcnt )
	    max = _sf->subfonts[i]->charcnt;
    for ( cid = 0; cid<max; ++cid ) {
	for ( i=0; i<_sf->subfontcnt; ++i ) {
	    sf = _sf->subfonts[i];
	    if ( cid<sf->charcnt && SCWorthOutputting(sf->chars[cid]))
	break;
	}
	if ( i!=_sf->subfontcnt ) {
	    sc = sf->chars[cid];
	    dumpshort(at->gi.hmtx,sc->width);
	    SplineCharFindBounds(sc,&b);
	    dumpshort(at->gi.hmtx,b.minx);
	    ++cnt;
	} else if ( cid==0 && i==sf->subfontcnt ) {
	    /* Use final subfont to contain mythical default if there is no real default */
	    dumpshort(at->gi.hmtx,sf->ascent+sf->descent);
	    dumpshort(at->gi.hmtx,0);
	    ++cnt;
	}
    }
    at->gi.hmtxlen = ftell(at->gi.hmtx);
    /* we don't need to align this table, things get written aligned */

    at->gi.maxp->numGlyphs = cnt;
}

static void dumptype2glyphs(SplineFont *sf,struct alltabs *at) {
    int i;
    struct pschars *subrs;

    at->cfff = tmpfile();
    at->sidf = tmpfile();
    at->sidh = tmpfile();
    at->charset = tmpfile();
    at->encoding = tmpfile();
    at->private = tmpfile();

    dumpcffheader(sf,at->cfff);
    dumpcffnames(sf,at->cfff);
    dumpcffcharset(sf,at);
    dumpcffencoding(sf,at);
    GProgressChangeStages(2);
    dumpcffprivate(sf,at,-1);
    subrs = SplineFont2Subrs2(sf);
    _dumpcffstrings(at->private,subrs);
    GProgressNextStage();
    at->charstrings = dumpcffstrings(SplineFont2Chrs2(sf,at->defwid,at->nomwid,subrs));
    dumpcfftopdict(sf,at);
    finishup(sf,at);

    at->cfflen = ftell(at->cfff);
    if ( at->cfflen&3 ) {
	for ( i=4-(at->cfflen&3); i>0; --i )
	    putc('\0',at->cfff);
    }

    dumpcffhmtx(at,sf);
}

static void dumpcidglyphs(SplineFont *sf,struct alltabs *at) {
    int i;

    at->cfff = tmpfile();
    at->sidf = tmpfile();
    at->sidh = tmpfile();
    at->charset = tmpfile();
    at->fdselect = tmpfile();
    at->fdarray = tmpfile();

    at->fds = gcalloc(sf->subfontcnt,sizeof(struct fd2data));
    for ( i=0; i<sf->subfontcnt; ++i ) {
	at->fds[i].private = tmpfile();
	dumpcffprivate(sf->subfonts[i],at,i);
	at->fds[i].subrs = SplineFont2Subrs2(sf->subfonts[i]);
	_dumpcffstrings(at->fds[i].private,at->fds[i].subrs);
    }

    dumpcffheader(sf,at->cfff);
    dumpcffnames(sf,at->cfff);
    dumpcffcidset(sf,at);
    dumpcfffdselect(sf,at);
    dumpcffdictindex(sf,at);
    at->charstrings = dumpcffstrings(CID2Chrs2(sf,at->fds));
    for ( i=0; i<sf->subfontcnt; ++i )
	PSCharsFree(at->fds[i].subrs);
    dumpcffcidtopdict(sf,at);
    finishupcid(sf,at);

    at->cfflen = ftell(at->cfff);
    if ( at->cfflen&3 ) {
	for ( i=4-(at->cfflen&3); i>0; --i )
	    putc('\0',at->cfff);
    }

    dumpcffcidhmtx(at,sf);
}

static void sethead(struct head *head,SplineFont *_sf) {
    time_t now;
    uint32 now1904[4];
    uint32 year[2];
    int i, lr, rl, j;
    SplineFont *sf = _sf;

    head->version = 0x00010000;
    head->checksumAdj = 0;
    head->magicNum = 0x5f0f3cf5;
    head->flags = 3;		/* baseline at 0, lsbline at 0 */
    head->emunits = sf->ascent+sf->descent;
/* Many of the following style names are truncated because that's what URW */
/*  does. Only 4 characters for each style. Hence "obli" rather than "oblique"*/
    if ( sf->weight!=NULL && (strstrmatch(sf->weight,"bold")!=NULL ||
		strstrmatch(sf->weight,"demi")!=NULL ||
		strstrmatch(sf->weight,"blac")!=NULL ))
	head->macstyle |= 1;
    if ( sf->fontname!=NULL && (strstrmatch(sf->fontname,"ital")!=NULL ||
		strstrmatch(sf->fontname,"obli")!=NULL ))
	head->macstyle |= 2;
    head->lowestreadable = 8;
    head->locais32 = 1;
    lr = rl = 0;
    j = 0;
    do {
	sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[j];
	for ( i=0; i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) ) {
		if ( islefttoright(sf->chars[i]->unicodeenc))
		    lr = 1;
		else if ( isrighttoleft(sf->chars[i]->unicodeenc))
		    rl = 1;
	    }
	++j;
    } while ( j<_sf->subfontcnt );
    sf = _sf;
    /* I assume we've always got some neutrals (spaces, punctuation) */
    if ( lr && rl )
	head->dirhint = 0;
    else if ( rl )
	head->dirhint = -2;
    else
	head->dirhint = 2;

    time(&now);		/* seconds since 1970, need to convert to seconds since 1904 */
    now1904[0] = now1904[1] = now1904[2] = now1904[3] = 0;
    year[0] = 60*60*24*365;
    year[1] = year[0]>>16; year[0] &= 0xffff;
    for ( i=4; i<70; ++i ) {
	now1904[3] += year[0];
	now1904[2] += year[1];
	if ( (i&3)==0 )
	    now1904[3] += 60*60*24;
	now1904[2] += now1904[3]>>16;
	now1904[3] &= 0xffff;
	now1904[1] += now1904[2]>>16;
	now1904[2] &= 0xffff;
    }
    now1904[3] += now&0xffff;
    now1904[2] += now>>16;
    now1904[2] += now1904[3]>>16;
    now1904[3] &= 0xffff;
    now1904[1] += now1904[2]>>16;
    now1904[2] &= 0xffff;
    head->modtime[1] = head->createtime[1] = (now1904[2]<<16)|now1904[3];
    head->modtime[0] = head->createtime[0] = (now1904[0]<<16)|now1904[1];
}

static void sethhead(struct hhead *hhead,struct alltabs *at, SplineFont *_sf) {
    int i, width, rbearing;
    SplineFont *sf;
    DBounds bb;
    int j;

    hhead->version = 0x00010000;
    hhead->ascender = sf->ascent;
    hhead->descender = -sf->descent;
    hhead->linegap = 0;
    width = 0x80000000; rbearing = 0x7fffffff;
    at->isfixed = true;
    j=0;
    do {
	sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[j];
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    SplineCharFindBounds(sf->chars[i],&bb);
	    if ( width!=0x80000000 && sf->chars[i]->width!=width ) at->isfixed = false;
	    if ( sf->chars[i]->width>width ) width = sf->chars[i]->width;
	    if ( sf->chars[i]->width-bb.maxx < rbearing ) rbearing = sf->chars[i]->width-bb.maxx;
	}
	++j;
    } while ( j<_sf->subfontcnt );
    hhead->maxwidth = width;
    hhead->minlsb = at->head.xmin;
    hhead->minrsb = rbearing;
    hhead->maxextent = at->head.xmax;
    hhead->caretSlopeRise = 1;
}

void SFDefaultOS2Info(struct pfminfo *pfminfo,SplineFont *_sf,char *fontname) {
    int i, samewid= -1, j;
    SplineFont *sf;

    if ( !pfminfo->pfmset ) {
	memset(pfminfo,'\0',sizeof(*pfminfo));
	j=0;
	do {
	    sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[j];
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
		if ( SCWorthOutputting(sf->chars[i]) ) {
		    if ( samewid==-1 )
			samewid = sf->chars[i]->width;
		    else if ( samewid!=sf->chars[i]->width )
			samewid = -2;
		}
	    }
	    ++j ;
	} while ( j<_sf->subfontcnt );
	sf = _sf;

	pfminfo->pfmfamily = 0x10;
	pfminfo->panose[0] = 2;
	if ( samewid>0 )
	    pfminfo->pfmfamily = 0x30;
	else if ( strstrmatch(fontname,"sans")!=NULL )
	    pfminfo->pfmfamily = 0x20;
	else if ( strstrmatch(fontname,"script")!=NULL ) {
	    pfminfo->pfmfamily = 0x40;
	    pfminfo->panose[0] = 3;
	}
	pfminfo->pfmfamily |= 0x1;	/* Else it assumes monospace */

	pfminfo->weight = 400;
	pfminfo->panose[2] = 5;
	if ( strstrmatch(fontname,"medium")!=NULL ) {
	    pfminfo->weight = 500;
	    pfminfo->panose[2] = 6;
	} else if ( (strstrmatch(fontname,"demi")!=NULL ||
		    strstrmatch(fontname,"semi")!=NULL) &&
		strstrmatch(fontname,"bold")!=NULL ) {
	    pfminfo->weight = 600;
	    pfminfo->panose[2] = 7;
	} else if ( strstrmatch(fontname,"bold")!=NULL ) {
	    pfminfo->weight = 700;
	    pfminfo->panose[2] = 8;
	} else if ( strstrmatch(fontname,"heavy")!=NULL ) {
	    pfminfo->weight = 800;
	    pfminfo->panose[2] = 9;
	} else if ( strstrmatch(fontname,"black")!=NULL ) {
	    pfminfo->weight = 900;
	    pfminfo->panose[2] = 10;
	} else if ( strstrmatch(fontname,"nord")!=NULL ) {
	    pfminfo->weight = 950;
	    pfminfo->panose[2] = 11;
	} else if ( strstrmatch(fontname,"thin")!=NULL ) {
	    pfminfo->weight = 100;
	    pfminfo->panose[2] = 2;
	} else if ( strstrmatch(fontname,"extra")!=NULL ||
		strstrmatch(fontname,"light")!=NULL ) {
	    pfminfo->weight = 200;
	    pfminfo->panose[2] = 3;
	} else if ( strstrmatch(fontname,"light")!=NULL ) {
	    pfminfo->weight = 300;
	    pfminfo->panose[2] = 4;
/* urw uses 4 character abreviations */
	} else if ( strstrmatch(fontname,"demi")!=NULL ) {
	    pfminfo->weight = 600;
	    pfminfo->panose[2] = 7;
	} else if ( strstrmatch(fontname,"medi")!=NULL ) {
	    pfminfo->weight = 500;
	    pfminfo->panose[2] = 6;
	}

	pfminfo->width = 5;
	pfminfo->panose[3] = 3;
	if ( strstrmatch(fontname,"ultra")!=NULL &&
		strstrmatch(fontname,"condensed")!=NULL ) {
	    pfminfo->width = 1;
	    pfminfo->panose[3] = 8;
	} else if ( strstrmatch(fontname,"extra")!=NULL &&
		strstrmatch(fontname,"condensed")!=NULL ) {
	    pfminfo->width = 2;
	    pfminfo->panose[3] = 8;
	} else if ( strstrmatch(fontname,"semi")!=NULL &&
		strstrmatch(fontname,"condensed")!=NULL ) {
	    pfminfo->width = 4;
	    pfminfo->panose[3] = 6;
	} else if ( strstrmatch(fontname,"condensed")!=NULL ) {
	    pfminfo->width = 3;
	    pfminfo->panose[3] = 6;
	} else if ( strstrmatch(fontname,"ultra")!=NULL &&
		strstrmatch(fontname,"expanded")!=NULL ) {
	    pfminfo->width = 9;
	    pfminfo->panose[3] = 7;
	} else if ( strstrmatch(fontname,"extra")!=NULL &&
		strstrmatch(fontname,"expanded")!=NULL ) {
	    pfminfo->width = 8;
	    pfminfo->panose[3] = 7;
	} else if ( strstrmatch(fontname,"semi")!=NULL &&
		strstrmatch(fontname,"expanded")!=NULL ) {
	    pfminfo->width = 6;
	    pfminfo->panose[3] = 5;
	} else if ( strstrmatch(fontname,"expanded")!=NULL ) {
	    pfminfo->width = 7;
	    pfminfo->panose[3] = 5;
	}
	if ( samewid>0 )
	    pfminfo->panose[3] = 9;
    }
}

static void setos2(struct os2 *os2,struct alltabs *at, SplineFont *_sf,
	enum fontformat format) {
    int i,j,cnt1,cnt2,first,last,avg1,avg2,k;
    SplineFont *sf = _sf;

    SFDefaultOS2Info(&sf->pfminfo,sf,sf->fontname);

    os2->version = 1;
    os2->weightClass = sf->pfminfo.weight;
    os2->widthClass = sf->pfminfo.width;
    os2->fstype = 0xc;
    if ( sf->pfminfo.fstype!=-1 )
	os2->fstype = sf->pfminfo.fstype;
    os2->ysubYSize = os2->ysubXSize = os2->ysubYOff = (sf->ascent+sf->descent)/5;
    os2->ysubXOff = os2->ysupXOff = 0;
    os2->ysupYSize = os2->ysupXSize = os2->ysupYOff = (sf->ascent+sf->descent)/5;
    os2->yStrikeoutSize = 102*(sf->ascent+sf->descent)/2048;
    os2->yStrikeoutPos = 530*(sf->ascent+sf->descent)/2048;
    os2->fsSel = (at->head.macstyle&1?32:0)|(at->head.macstyle&2?1:0);
    if ( strstrmatch(sf->fullname,"outline")!=NULL ) os2->fsSel |= 8;
    if ( os2->fsSel==0 ) os2->fsSel = 64;		/* Regular */
    os2->ascender = os2->winascent = at->head.ymax;
    os2->descender = at->head.ymin;
    os2->windescent = -at->head.ymin;
    os2->linegap = 0;

    avg1 = avg2 = last = 0; first = 0x10000;
    cnt1 = cnt2 = 0;
    k = 0;
    do {
	sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->unicodeenc!=-1 ) {
	    for ( j=0; j<sizeof(uniranges)/sizeof(uniranges[0]); ++j )
		if ( sf->chars[i]->unicodeenc>=uniranges[j][0] &&
			sf->chars[i]->unicodeenc<=uniranges[j][1] ) {
		    os2->unicoderange[j>>5] |= (1<<(j&31));
	    break;
		}
	    if ( sf->chars[i]->unicodeenc<first ) first = sf->chars[i]->unicodeenc;
	    if ( sf->chars[i]->unicodeenc>last ) last = sf->chars[i]->unicodeenc;
	    avg2 += sf->chars[i]->width; ++cnt2;
	    if ( sf->chars[i]->unicodeenc==' ' ||
		    (sf->chars[i]->unicodeenc>='a' && sf->chars[i]->unicodeenc<='z')) {
		avg1 += sf->chars[i]->width; ++cnt1;
	    }
	}
	++k;
    } while ( k<_sf->subfontcnt );
    sf = _sf;

    if ( BDFFoundry!=NULL && strcmp(BDFFoundry,"PfaEdit")!=0 )
	strncpy(os2->achVendID,BDFFoundry,4);
    else
	memcpy(os2->achVendID,"PfEd",4);

    if ( cnt1==27 )
	os2->avgCharWid = avg1/cnt1;
    else if ( cnt2!=0 )
	os2->avgCharWid = avg2/cnt2;
    memcpy(os2->panose,sf->pfminfo.panose,sizeof(os2->panose));
    os2->firstcharindex = first;
    os2->lastcharindex = last;
    if ( sf->encoding_name==em_iso8859_1 || sf->encoding_name==em_iso8859_15 ||
	    sf->encoding_name==em_win )
	os2->ulCodePage[0] |= 1<<0;	/* latin1 */
    else if ( sf->encoding_name==em_iso8859_2 )
	os2->ulCodePage[0] |= 1<<1;	/* latin2 */
    else if ( sf->encoding_name==em_iso8859_9 )
	os2->ulCodePage[0] |= 1<<4;	/* turkish */
    else if ( sf->encoding_name==em_iso8859_4 )
	os2->ulCodePage[0] |= 1<<7;	/* baltic */
    else if ( sf->encoding_name==em_iso8859_5 || sf->encoding_name==em_koi8_r )
	os2->ulCodePage[0] |= 1<<2;	/* cyrillic */
    else if ( sf->encoding_name==em_iso8859_7 )
	os2->ulCodePage[0] |= 1<<3;	/* greek */
    else if ( sf->encoding_name==em_iso8859_8 )
	os2->ulCodePage[0] |= 1<<5;	/* hebrew */
    else if ( sf->encoding_name==em_iso8859_8 )
	os2->ulCodePage[0] |= 1<<6;	/* arabic */
    else if ( sf->encoding_name==em_iso8859_11 )
	os2->ulCodePage[0] |= 1<<16;	/* thai */
    else if ( sf->encoding_name==em_jis201 || sf->encoding_name==em_jis208 ||
	    sf->encoding_name==em_jis212 )
	os2->ulCodePage[0] |= 1<<17;	/* japanese */
    else if ( sf->encoding_name==em_gb2312 )
	os2->ulCodePage[0] |= 1<<18;	/* simplified chinese */
    else if ( sf->encoding_name==em_ksc5601 )
	os2->ulCodePage[0] |= 1<<19;	/* korean wansung */
    else if ( sf->encoding_name==em_big5 )
	os2->ulCodePage[0] |= 1<<20;	/* traditional chinese */
#if 0
    else if ( sf->encoding_name==em_ksc5601 )
	os2->ulCodePage[0] |= 1<<21;	/* korean johab */
#endif
    else if ( sf->encoding_name==em_mac )
	os2->ulCodePage[0] |= 1<<29;	/* mac */
    else if ( sf->encoding_name==em_symbol )
	os2->ulCodePage[0] |= 1<<31;	/* symbol */

    k=0;
    do {
	sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    if ( sf->chars[i]->unicodeenc==0xde )
		os2->ulCodePage[0] |= 1<<0;		/* latin1 */
	    else if ( sf->chars[i]->unicodeenc==0x13d )
		os2->ulCodePage[0] |= 1<<1;		/* latin2 */
	    else if ( sf->chars[i]->unicodeenc==0x411 )
		os2->ulCodePage[0] |= 1<<2;		/* cyrillic */
	    else if ( sf->chars[i]->unicodeenc==0x386 )
		os2->ulCodePage[0] |= 1<<3;		/* greek */
	    else if ( sf->chars[i]->unicodeenc==0x130 )
		os2->ulCodePage[0] |= 1<<4;		/* turkish */
	    else if ( sf->chars[i]->unicodeenc==0x5d0 )
		os2->ulCodePage[0] |= 1<<5;		/* hebrew */
	    else if ( sf->chars[i]->unicodeenc==0x631 )
		os2->ulCodePage[0] |= 1<<6;		/* arabic */
	    else if ( sf->chars[i]->unicodeenc==0x157 )
		os2->ulCodePage[0] |= 1<<7;		/* baltic */
	    else if ( sf->chars[i]->unicodeenc==0xe45 )
		os2->ulCodePage[0] |= 1<<16;	/* thai */
	    else if ( sf->chars[i]->unicodeenc==0x30a8 )
		os2->ulCodePage[0] |= 1<<17;	/* japanese */
	    else if ( sf->chars[i]->unicodeenc==0x3105 )
		os2->ulCodePage[0] |= 1<<18;	/* simplified chinese */
	    else if ( sf->chars[i]->unicodeenc==0x3131 )
		os2->ulCodePage[0] |= 1<<19;	/* korean wansung */
	    else if ( sf->chars[i]->unicodeenc==0xacf4 )
		os2->ulCodePage[0] |= 1<<21;	/* korean Johab */
	    else if ( sf->chars[i]->unicodeenc==0x21d4 )
		os2->ulCodePage[0] |= 1<<31;	/* symbol */
	}
	++k;
    } while ( k<_sf->subfontcnt );
    sf = _sf;

    if ( os2->ulCodePage[0]==0 )
	os2->ulCodePage[0] |= 1;
    if ( format==ff_otf || format==ff_otfcid ) {
	BlueData bd;

	QuickBlues(sf,&bd);		/* This handles cid fonts properly */
	os2->version = 2;
	os2->xHeight = bd.xheight;
	os2->capHeight = bd.caph;
	os2->defChar = ' ';
	os2->breakChar = ' ';
	os2->maxContext = 2;	/* Might do some kerning */
    }
}

static void redoloca(struct alltabs *at) {
    int i;

    at->loca = tmpfile();
    if ( at->head.locais32 ) {
	for ( i=0; i<=at->maxp.numGlyphs; ++i )
	    dumplong(at->loca,at->gi.loca[i]);
	at->localen = sizeof(int32)*(at->maxp.numGlyphs+1);
    } else {
	for ( i=0; i<=at->maxp.numGlyphs; ++i )
	    dumpshort(at->loca,at->gi.loca[i]/2);
	at->localen = sizeof(int16)*(at->maxp.numGlyphs+1);
	if ( ftell(at->loca)&2 )
	    dumpshort(at->loca,0);
    }
    free(at->gi.loca);
}

static void redohead(struct alltabs *at) {
    at->headf = tmpfile();

    dumplong(at->headf,at->head.version);
    dumplong(at->headf,at->head.revision);
    dumplong(at->headf,at->head.checksumAdj);
    dumplong(at->headf,at->head.magicNum);
    dumpshort(at->headf,at->head.flags);
    dumpshort(at->headf,at->head.emunits);
    dumplong(at->headf,at->head.createtime[0]);
    dumplong(at->headf,at->head.createtime[1]);
    dumplong(at->headf,at->head.modtime[0]);
    dumplong(at->headf,at->head.modtime[1]);
    dumpshort(at->headf,at->head.xmin);
    dumpshort(at->headf,at->head.ymin);
    dumpshort(at->headf,at->head.xmax);
    dumpshort(at->headf,at->head.ymax);
    dumpshort(at->headf,at->head.macstyle);
    dumpshort(at->headf,at->head.lowestreadable);
    dumpshort(at->headf,at->head.dirhint);
    dumpshort(at->headf,at->head.locais32);
    dumpshort(at->headf,at->head.glyphformat);

    at->headlen = ftell(at->headf);
    if ( (at->headlen&2)!=0 )
	dumpshort(at->headf,0);
}

static void redohhead(struct alltabs *at) {
    int i;
    at->hheadf = tmpfile();

    dumplong(at->hheadf,at->hhead.version);
    dumpshort(at->hheadf,at->hhead.ascender);
    dumpshort(at->hheadf,at->hhead.descender);
    dumpshort(at->hheadf,at->hhead.linegap);
    dumpshort(at->hheadf,at->hhead.maxwidth);
    dumpshort(at->hheadf,at->hhead.minlsb);
    dumpshort(at->hheadf,at->hhead.minrsb);
    dumpshort(at->hheadf,at->hhead.maxextent);
    dumpshort(at->hheadf,at->hhead.caretSlopeRise);
    dumpshort(at->hheadf,at->hhead.caretSlopeRun);
    for ( i=0; i<5; ++i )
	dumpshort(at->hheadf,at->hhead.mbz[i]);
    dumpshort(at->hheadf,at->hhead.metricformat);
    dumpshort(at->hheadf,at->hhead.numMetrics);

    at->hheadlen = ftell(at->hheadf);
    if ( (at->hheadlen&2)!=0 )
	dumpshort(at->hheadf,0);
}

static void redomaxp(struct alltabs *at,enum fontformat format) {
    at->maxpf = tmpfile();

    dumplong(at->maxpf,at->maxp.version);
    dumpshort(at->maxpf,at->maxp.numGlyphs);
    if ( format!=ff_otf && format!=ff_otfcid ) {
	dumpshort(at->maxpf,at->maxp.maxPoints);
	dumpshort(at->maxpf,at->maxp.maxContours);
	dumpshort(at->maxpf,at->maxp.maxCompositPts);
	dumpshort(at->maxpf,at->maxp.maxCompositCtrs);
	dumpshort(at->maxpf,at->maxp.maxZones);
	dumpshort(at->maxpf,at->maxp.maxTwilightPts);
	dumpshort(at->maxpf,at->maxp.maxStorage);
	dumpshort(at->maxpf,at->maxp.maxFDEFs);
	dumpshort(at->maxpf,at->maxp.maxIDEFs);
	dumpshort(at->maxpf,at->maxp.maxStack);
	dumpshort(at->maxpf,at->maxp.maxglyphInstr);
	dumpshort(at->maxpf,at->maxp.maxnumcomponents);
	dumpshort(at->maxpf,at->maxp.maxcomponentdepth);
    }

    at->maxplen = ftell(at->maxpf);
    if ( (at->maxplen&2)!=0 )
	dumpshort(at->maxpf,0);
}

static void redoos2(struct alltabs *at) {
    int i;
    at->os2f = tmpfile();

    dumpshort(at->os2f,at->os2.version);
    dumpshort(at->os2f,at->os2.avgCharWid);
    dumpshort(at->os2f,at->os2.weightClass);
    dumpshort(at->os2f,at->os2.widthClass);
    dumpshort(at->os2f,at->os2.fstype);
    dumpshort(at->os2f,at->os2.ysubXSize);
    dumpshort(at->os2f,at->os2.ysubYSize);
    dumpshort(at->os2f,at->os2.ysubXOff);
    dumpshort(at->os2f,at->os2.ysubYOff);
    dumpshort(at->os2f,at->os2.ysupXSize);
    dumpshort(at->os2f,at->os2.ysupYSize);
    dumpshort(at->os2f,at->os2.ysupXOff);
    dumpshort(at->os2f,at->os2.ysupYOff);
    dumpshort(at->os2f,at->os2.yStrikeoutSize);
    dumpshort(at->os2f,at->os2.yStrikeoutPos);
    dumpshort(at->os2f,at->os2.sFamilyClass);
    for ( i=0; i<10; ++i )
	putc(at->os2.panose[i],at->os2f);
    for ( i=0; i<4; ++i )
	dumplong(at->os2f,at->os2.unicoderange[i]);
    for ( i=0; i<4; ++i )
	putc(at->os2.achVendID[i],at->os2f);
    dumpshort(at->os2f,at->os2.fsSel);
    dumpshort(at->os2f,at->os2.firstcharindex);
    dumpshort(at->os2f,at->os2.lastcharindex);
    dumpshort(at->os2f,at->os2.ascender);
    dumpshort(at->os2f,at->os2.descender);
    dumpshort(at->os2f,at->os2.linegap);
    dumpshort(at->os2f,at->os2.winascent);
    dumpshort(at->os2f,at->os2.windescent);
    dumplong(at->os2f,at->os2.ulCodePage[0]);
    dumplong(at->os2f,at->os2.ulCodePage[1]);

    if ( at->os2.version>=2 ) {
	dumpshort(at->os2f,at->os2.xHeight);
	dumpshort(at->os2f,at->os2.capHeight);
	dumpshort(at->os2f,at->os2.defChar);
	dumpshort(at->os2f,at->os2.breakChar);
	dumpshort(at->os2f,at->os2.maxContext);
    }

    at->os2len = ftell(at->os2f);
    if ( (at->os2len&2)!=0 )
	dumpshort(at->os2f,0);
}

static void redocvt(struct alltabs *at) {
    int i;
    at->cvtf = tmpfile();

    for ( i=0; i<at->gi.cvtcur; ++i )
	dumpshort(at->cvtf,at->gi.cvt[i]);
    at->cvtlen = ftell(at->cvtf);
    if ( 1&at->gi.cvtcur )
	dumpshort(at->cvtf,0);
}

static void dumpkerns(struct alltabs *at, SplineFont *sf) {
    int i, cnt, j, k, m, threshold;
    KernPair *kp;
    uint16 *glnum, *offsets;

    /* I'm told that at most 2048 kern pairs are allowed in a ttf font */
    threshold = KernThreshold(sf,2048);

    cnt = m = 0;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	j = 0;
	for ( kp = sf->chars[i]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->off>=threshold || kp->off<=-threshold ) 
		++cnt, ++j;
	if ( j>m ) m=j;
    }
    if ( cnt==0 )
return;

    at->kern = tmpfile();
    dumpshort(at->kern,0);		/* version */
    dumpshort(at->kern,1);		/* number of subtables */
    dumpshort(at->kern,0);		/* subtable version */
    dumpshort(at->kern,(7+3*cnt)*sizeof(uint16)); /* subtable length */
    dumpshort(at->kern,1);		/* coverage, flags&format */
    dumpshort(at->kern,cnt);
    for ( i=1,j=0; i<=cnt; i<<=1, ++j );
    i>>=1; --j;
    dumpshort(at->kern,i*6);		/* binary search headers */
    dumpshort(at->kern,j);
    dumpshort(at->kern,6*(i-cnt));

    glnum = galloc(m*sizeof(uint16));
    offsets = galloc(m*sizeof(uint16));
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	m = 0;
	for ( kp = sf->chars[i]->kerns; kp!=NULL; kp=kp->next ) {
	    if ( kp->off>=threshold || kp->off<=-threshold ) {
		/* order the pairs */
		for ( j=0; j<m; ++j )
		    if ( kp->sc->ttf_glyph<glnum[j] )
		break;
		for ( k=m; k>j; --k ) {
		    glnum[k] = glnum[k-1];
		    offsets[k] = offsets[k-1];
		}
		glnum[j] = kp->sc->ttf_glyph;
		offsets[j] = kp->off;
		++m;
	    }
	}
	for ( j=0; j<m; ++j ) {
	    dumpshort(at->kern,sf->chars[i]->ttf_glyph);
	    dumpshort(at->kern,glnum[j]);
	    dumpshort(at->kern,offsets[j]);
	}
    }
    at->kernlen = ftell(at->kern);
    if ( at->kernlen&2 )
	dumpshort(at->kern,0);		/* pad it */
}

static void dumpmacstr(FILE *file,unichar_t *str) {
    int ch;
    unsigned char *table;

    do {
	ch = *str++;
	if ( ch==0 )
	    putc('\0',file);
	else if ( (ch>>8)>=mac_from_unicode.first && (ch>>8)<=mac_from_unicode.last &&
		(table = mac_from_unicode.table[(ch>>8)-mac_from_unicode.first])!=NULL &&
		table[ch&0xff]!=0 )
	    putc(table[ch&0xff],file);
	else
	    putc('?',file);	/* if we were to omit an unencoded char all our position calculations would be off */
    } while ( ch!='\0' );
}

#if 0
static void dumpstr(FILE *file,unichar_t *str) {
    do {
	putc(*str,file);
    } while ( *str++!='\0' );
}
#endif

static void dumpustr(FILE *file,unichar_t *str) {
    do {
	putc(*str>>8,file);
	putc(*str&0xff,file);
    } while ( *str++!='\0' );
}

static void dumppstr(FILE *file,char *str) {
    putc(strlen(str),file);
    fwrite(str,sizeof(char),strlen(str),file);
}

#if 0
/* Languages on the mac presumably imply an encoding, but that encoding is not*/
/*  listed in the language table in the name table docs. I think it is safe to*/
/*  guess that these first 10 languages all use the MacRoman encoding that */
/*  is designed for western europe. I could handle that... */
/* The complexities of locale (british vs american english) don't seem to be */
/*  present on the mac */
/* I don't think I'll make use of this table for the mac though... */
static struct { int mslang, maclang, enc, used; } mactrans[] = {
    { 0x09, 0, em_mac },		/* English */
    { 0x0c, 1, em_mac },		/* French */
    { 0x07, 2, em_mac },		/* German */
    { 0x10, 3, em_mac },		/* Italian */
    { 0x13, 4, em_mac },		/* Dutch */
    { 0x1d, 5, em_mac },		/* Swedish */
    { 0x0a, 6, em_mac },		/* Spanish */
    { 0x06, 7, em_mac },		/* Danish */
    { 0x16, 8, em_mac },		/* Portuguese */
    { 0x14, 9, em_mac },		/* Norwegian */
    { 0 }};
#endif

void DefaultTTFEnglishNames(struct ttflangname *dummy, SplineFont *sf) {
    time_t now;
    struct tm *tm;
    char buffer[200];
    char *temp;

    if ( dummy->names[0]==NULL ) dummy->names[0] = uc_copy(sf->copyright);
    if ( dummy->names[1]==NULL ) dummy->names[1] = uc_copy(sf->familyname);
    if ( dummy->names[2]==NULL ) {
	temp = SFGetModifiers(sf);
	dummy->names[2] = uc_copy(temp && *temp?temp:"Regular");
    }
    if ( dummy->names[3]==NULL ) {
	time(&now);
	tm = localtime(&now);
	sprintf( buffer, "%s : %s : %d-%d-%d", BDFFoundry?BDFFoundry:"PfaEdit 1.0",
		sf->fullname, tm->tm_mday, tm->tm_mon, tm->tm_year+1970 );
	dummy->names[3] = uc_copy(buffer);
    }
    if ( dummy->names[4]==NULL ) dummy->names[4] = uc_copy(sf->fullname);
    if ( dummy->names[5]==NULL ) dummy->names[5] = uc_copy(sf->version);
    if ( dummy->names[6]==NULL ) dummy->names[6] = uc_copy(sf->fontname);
}

static void dumpnames(struct alltabs *at, SplineFont *sf) {
    int pos=0,i,j;
    struct ttflangname dummy, *cur, *useng;
    int strcnt=0;
    int posses[ttf_namemax];

    memset(&dummy,'\0',sizeof(dummy));
    useng = NULL;
    for ( cur=sf->names; cur!=NULL; cur=cur->next ) {
	if ( cur->lang!=0x409 ) {
	    for ( i=0; i<ttf_namemax; ++i )
		if ( cur->names[i]!=NULL ) ++strcnt;
	} else {
	    dummy = *cur;
	    useng = cur;
	}
    }
    DefaultTTFEnglishNames(&dummy, sf);
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL ) strcnt+=3;
    	/* once of mac roman encoding, once for mac unicode and once for windows unicode 409 */

    at->name = tmpfile();
    dumpshort(at->name,0);	/* format */
    dumpshort(at->name,strcnt);	/* numrec */
    dumpshort(at->name,(3+strcnt*6)*sizeof(int16));	/* offset to strings */
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL ) {
	dumpshort(at->name,1);	/* apple */
	dumpshort(at->name,0);	/*  */
	dumpshort(at->name,0);	/* Roman alphabet */
	dumpshort(at->name,i);
	dumpshort(at->name,u_strlen(dummy.names[i]));
	dumpshort(at->name,pos);
	pos += u_strlen(dummy.names[i])+1;
    }
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL ) {
	dumpshort(at->name,0);	/* apple unicode */
	dumpshort(at->name,3);	/* 3 => Unicode 2.0 semantics */ /* 0 ("default") is also a reasonable value */
	dumpshort(at->name,0);	/*  */
	dumpshort(at->name,i);
	dumpshort(at->name,2*u_strlen(dummy.names[i]));
	dumpshort(at->name,pos);
	posses[i] = pos;
	pos += 2*u_strlen(dummy.names[i])+2;
    }
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL ) {
	dumpshort(at->name,3);	/* MS platform */
	dumpshort(at->name,1);	/* not symbol */
	dumpshort(at->name,0x0409);	/* american english language */
	dumpshort(at->name,i);
	dumpshort(at->name,2*u_strlen(dummy.names[i]));
	dumpshort(at->name,posses[i]);
    }

    for ( cur=sf->names; cur!=NULL; cur=cur->next ) if ( cur->lang!=0x409 ) {
	for ( i=0; i<ttf_namemax; ++i ) if ( cur->names[i]!=NULL ) {
	    dumpshort(at->name,3);	/* MS platform */
	    dumpshort(at->name,1);	/* not symbol */
	    dumpshort(at->name,cur->lang);/* american english language */
	    dumpshort(at->name,i);
	    dumpshort(at->name,2*u_strlen(cur->names[i]));
	    dumpshort(at->name,pos);
	    pos += 2*u_strlen(cur->names[i])+2;
	}
    }

    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL )
	dumpmacstr(at->name,dummy.names[i]);
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL )
	dumpustr(at->name,dummy.names[i]);
    for ( cur=sf->names; cur!=NULL; cur=cur->next ) if ( cur->lang!=0x409 )
	for ( i=0; i<ttf_namemax; ++i ) if ( cur->names[i]!=NULL )
	    dumpustr(at->name,cur->names[i]);
    at->namelen = ftell(at->name);
    if ( (at->namelen&3)!=0 )
	for ( j= 4-(at->namelen&3); j>0; --j )
	    putc('\0',at->name);
    for ( i=0; i<ttf_namemax; ++i )
	if ( useng==NULL || dummy.names[i]!=useng->names[i] )
	    free( dummy.names[i]);
}

static void dumppost(struct alltabs *at, SplineFont *sf, enum fontformat format) {
    int pos, i,j;

    at->post = tmpfile();

    dumplong(at->post,format!=ff_otf && format!=ff_otfcid?0x00020000:0x00030000);	/* formattype */
    dumpfixed(at->post,sf->italicangle);
    dumpshort(at->post,sf->upos);
    dumpshort(at->post,sf->uwidth);
    dumplong(at->post,at->isfixed);
    dumplong(at->post,0);		/* no idea about memory */
    dumplong(at->post,0);		/* no idea about memory */
    dumplong(at->post,0);		/* no idea about memory */
    dumplong(at->post,0);		/* no idea about memory */
    if ( format!=ff_otf && format!=ff_otfcid ) {
	dumpshort(at->post,at->maxp.numGlyphs);

	dumpshort(at->post,0);		/* glyph 0 is named .notdef */
	dumpshort(at->post,1);		/* glyphs 1&2 are tab and cr */
	dumpshort(at->post,2);		/* or something */
	for ( i=pos=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=0 ) {
	    if ( sf->chars[i]->unicodeenc<128 && sf->chars[i]->unicodeenc!=-1 )
		dumpshort(at->post,sf->chars[i]->unicodeenc-32+3);
	    else if ( strcmp(sf->chars[i]->name,".notdef")==0 )
		dumpshort(at->post,0);
	    else {
		for ( j=127-32+3; j<258; ++j )
		    if ( strcmp(sf->chars[i]->name,ttfstandardnames[j])==0 )
		break;
		if ( j!=258 )
		    dumpshort(at->post,j);
		else {
		    dumpshort(at->post,pos+258);
		    /*pos += strlen( sf->chars[i]->name)+1*/ ++pos;
		}
	    }
	}
	if ( pos!=0 ) {
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=0 ) {
		if ( sf->chars[i]->unicodeenc<128 && sf->chars[i]->unicodeenc!=-1 )
		    /* Do Nothing */;
		else if ( strcmp(sf->chars[i]->name,".notdef")==0 )
		    /* Do Nothing */;
		else {
		    for ( j=127-32+3; j<258; ++j )
			if ( strcmp(sf->chars[i]->name,ttfstandardnames[j])==0 )
		    break;
		    if ( j!=258 )
			/* Do Nothing */;
		    else
			dumppstr(at->post,sf->chars[i]->name);
		}
	    }
	}
    }
    at->postlen = ftell(at->post);
    if ( (at->postlen&3)!=0 )
	for ( j= 4-(at->postlen&3); j>0; --j )
	    putc('\0',at->post);
}

static void SetTo(uint16 *avail,uint16 *sfind,int to,int from) {
    avail[to] = avail[from];
    if ( sfind!=NULL ) sfind[to] = sfind[from];
}

static int figureencoding(SplineFont *sf,int i) {
    switch ( sf->encoding_name ) {
      default:				/* Unicode */
return( sf->chars[i]->unicodeenc );
      case em_ksc5601:			/* Wansung */
	if ( (i/96)>=94 )
return( -1 );

return( ((i/96)<<8) + (i%96) + 0xa1a0 );
      case em_jis208: {			/* SJIS */
	int ch1, ch2, ro, co;
	ch1 = i/96; ch2 = i%96;
	if ( ch1>=94 )
return( -1 );
	ro = ch1<95 ? 112 : 176;
	co = (ch1&1) ? (ch2>95?32:31) : 126;

return(  ((((ch1+1)>>1) + ro )<<8 )    |    (ch2+co) );
      }
    }
}

static void dumpcmap(struct alltabs *at, SplineFont *_sf,enum fontformat format) {
    uint16 *avail = galloc(65536*sizeof(uint16));
    uint16 *sfind = NULL;
    int i,j,k,l,charcnt;
    int segcnt, cnt=0, delta, rpos;
    struct cmapseg { uint16 start, end; uint16 delta; uint16 rangeoff; } *cmapseg;
    uint16 *ranges;
    char table[256];
    SplineFont *sf = _sf;
    SplineChar *sc;

    memset(avail,0xff,65536*sizeof(uint16));
    at->cmap = tmpfile();

    /* Mac, symbol encoding table */ /* Not going to bother with this for cid fonts */
    memset(table,'\0',sizeof(table));
    for ( i=0; i<sf->charcnt && i<256; ++i )
	if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=0 )
	    table[sf->chars[i]->enc] = sf->chars[i]->ttf_glyph;
    table[0] = table[8] = table[13] = table[29] = 1;
    table[9] = 3;

    if ( format==ff_ttfsym || sf->encoding_name==em_symbol ) {
	/* Two encoding table pointers, one for ms, one for mac */
	dumpshort(at->cmap,0);		/* version */
	dumpshort(at->cmap,2);		/* num tables */
	dumpshort(at->cmap,3);		/* ms platform */
	dumpshort(at->cmap,0);		/* plat specific enc, symbol */
	dumplong(at->cmap,2*sizeof(uint16)+2*(2*sizeof(uint16)+sizeof(uint32)));	/* offset from tab start to sub tab start */
	dumpshort(at->cmap,1);		/* mac platform */
	dumpshort(at->cmap,32);		/* plat specific enc, uninterpretted */
	dumplong(at->cmap,2*sizeof(uint16)+2*(2*sizeof(uint16)+sizeof(uint32))+262);	/* offset from tab start to sub tab start */

	dumpshort(at->cmap,0);		/* format */
	dumpshort(at->cmap,262);	/* length = 256bytes + 6 header bytes */
	dumpshort(at->cmap,0);		/* language = meaningless */
	for ( i=0; i<256; ++i )
	    putc(table[i],at->cmap);
    } else {
	if ( _sf->subfontcnt!=0 ) sfind = gcalloc(65536,sizeof(uint16));
	charcnt = _sf->charcnt;
	if ( _sf->subfontcnt!=0 ) {
	    for ( k=0; k<_sf->subfontcnt; ++k )
		if ( _sf->subfonts[k]->charcnt > charcnt )
		    charcnt =  _sf->subfonts[k]->charcnt;
	}
	for ( i=0; i<charcnt; ++i ) {
	    k = 0;
	    do {
		sf = (_sf->subfontcnt==0 ) ? _sf : _sf->subfonts[k];
		if ( i<sf->charcnt &&
			sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=0 &&
			(l = figureencoding(sf,i))!=-1 ) {
		    avail[l] = i;
		    if ( sfind!=NULL ) sfind[l] = k;
		    ++cnt;
	    break;
		}
		++k;
	    } while ( k<_sf->subfontcnt );
	}
	if ( _sf->encoding_name!=em_jis208 && _sf->encoding_name!=em_ksc5601 ) {
	    /* Duplicate glyphs for greek */	/* Only meaningful if unicode */
	    if ( avail[0xb5]==0xffff && avail[0x3bc]!=0xffff )
		SetTo(avail,sfind,0xb5,0x3bc);
	    else if ( avail[0x3bc]==0xffff && avail[0xb5]!=0xffff )
		SetTo(avail,sfind,0x3bc,0xb5);
	    if ( avail[0x394]==0xffff && avail[0x2206]!=0xffff )
		SetTo(avail,sfind,0x394,0x2206);
	    else if ( avail[0x2206]==0xffff && avail[0x394]!=0xffff )
		SetTo(avail,sfind,0x2206,0x394);
	    if ( avail[0x3a9]==0xffff && avail[0x2126]!=0xffff )
		SetTo(avail,sfind,0x3a9,0x2126);
	    else if ( avail[0x2126]==0xffff && avail[0x3a9]!=0xffff )
		SetTo(avail,sfind,0x2126,0x3a9);
	}

	j = -1;
	for ( i=segcnt=0; i<65536; ++i ) {
	    if ( avail[i]!=0xffff && j==-1 ) {
		j=i;
		++segcnt;
	    } else if ( j!=-1 && avail[i]==0xffff )
		j = -1;
	}
	cmapseg = gcalloc(segcnt+1,sizeof(struct cmapseg));
	ranges = galloc(cnt*sizeof(int16));
	j = -1;
	for ( i=segcnt=0; i<65536; ++i ) {
	    if ( avail[i]!=0xffff && j==-1 ) {
		j=i;
		cmapseg[segcnt].start = j;
		++segcnt;
	    } else if ( j!=-1 && avail[i]==0xffff ) {
		cmapseg[segcnt-1].end = i-1;
		j = -1;
	    }
	}
	if ( j!=-1 )
	    cmapseg[segcnt-1].end = i-1;
	/* create a dummy segment to mark the end of the table */
	cmapseg[segcnt].start = cmapseg[segcnt].end = 0xffff;
	cmapseg[segcnt++].delta = 1;
	rpos = 0;
	for ( i=0; i<segcnt-1; ++i ) {
	    l = avail[cmapseg[i].start];
	    sc = sfind==NULL ? _sf->chars[l] : _sf->subfonts[sfind[cmapseg[i].start]]->chars[l];
	    delta = sc->ttf_glyph-cmapseg[i].start;
	    for ( j=cmapseg[i].start; j<=cmapseg[i].end; ++j ) {
		l = avail[j];
		sc = sfind==NULL ? _sf->chars[l] : _sf->subfonts[sfind[j]]->chars[l];
		if ( delta != sc->ttf_glyph-j )
	    break;
	    }
	    if ( j>cmapseg[i].end )
		cmapseg[i].delta = delta;
	    else {
		cmapseg[i].rangeoff = (rpos + (segcnt-i)) * sizeof(int16);
		for ( j=cmapseg[i].start; j<=cmapseg[i].end; ++j ) {
		    l = avail[j];
		    sc = sfind==NULL ? _sf->chars[l] : _sf->subfonts[sfind[j]]->chars[l];
		    ranges[rpos++] = sc->ttf_glyph;
		}
	    }
	}
	free(avail);
	if ( _sf->subfontcnt!=0 ) free(sfind);

	/* Two encoding table pointers, one for ms, one for mac */
	dumpshort(at->cmap,0);		/* version */
	dumpshort(at->cmap,2);		/* num tables */
	dumpshort(at->cmap,3);		/* ms platform */
	dumpshort(at->cmap,		/* plat specific enc */
		sf->encoding_name==em_ksc5601 ? 5 :	/* Wansung */
		sf->encoding_name==em_jis208 ? 2 :	/* SJIS */
		 1 );					/* Unicode */
	dumplong(at->cmap,2*sizeof(uint16)+2*(2*sizeof(uint16)+sizeof(uint32)));	/* offset from tab start to sub tab start */
	dumpshort(at->cmap,1);		/* mac platform */
	dumpshort(at->cmap,0);		/* plat specific enc, script=roman */
	dumplong(at->cmap,2*sizeof(uint16)+2*(2*sizeof(uint16)+sizeof(uint32))+(8+4*segcnt+rpos)*sizeof(int16));	/* offset from tab start to sub tab start */

	dumpshort(at->cmap,4);		/* format */
	dumpshort(at->cmap,(8+4*segcnt+rpos)*sizeof(int16));
	dumpshort(at->cmap,0);		/* language/version */
	dumpshort(at->cmap,2*segcnt);	/* segcnt */
	for ( j=0,i=1; i<=segcnt; i<<=1, ++j );
	dumpshort(at->cmap,i);		/* 2*2^floor(log2(segcnt)) */
	dumpshort(at->cmap,j-1);
	dumpshort(at->cmap,2*segcnt-i);
	for ( i=0; i<segcnt; ++i )
	    dumpshort(at->cmap,cmapseg[i].end);
	dumpshort(at->cmap,0);
	for ( i=0; i<segcnt; ++i )
	    dumpshort(at->cmap,cmapseg[i].start);
	for ( i=0; i<segcnt; ++i )
	    dumpshort(at->cmap,cmapseg[i].delta);
	for ( i=0; i<segcnt; ++i )
	    dumpshort(at->cmap,cmapseg[i].rangeoff);
	for ( i=0; i<rpos; ++i )
	    dumpshort(at->cmap,ranges[i]);
	free(ranges);
	free(cmapseg);
    }

    /* Mac table just same as symbol table */
    dumpshort(at->cmap,0);		/* format */
    dumpshort(at->cmap,262);	/* length = 256bytes + 6 header bytes */
    dumpshort(at->cmap,0);		/* language = english */
    for ( i=0; i<256; ++i )
	putc(table[i],at->cmap);

    at->cmaplen = ftell(at->cmap);
    if ( (at->cmaplen&2)!=0 )
	dumpshort(at->cmap,0);
}

static int32 filecheck(FILE *file) {
    uint32 sum = 0, chunk;

    rewind(file);
    while ( 1 ) {
	chunk = getuint32(file);
	if ( feof(file))
    break;
	sum += chunk;
    }
return( sum );
}

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))
static void initTables(struct alltabs *at, SplineFont *sf,enum fontformat format) {
    int i, pos;

    memset(at,'\0',sizeof(struct alltabs));

    at->maxp.version = 0x00010000;
    if ( format==ff_otf || format==ff_otfcid )
	at->maxp.version = 0x00005000;
    at->maxp.maxZones = 2;
    at->maxp.maxFDEFs = 1;
    at->maxp.maxStorage = 64;
    at->maxp.maxStack = 64;
    at->gi.maxp = &at->maxp;
    if ( format==ff_otf )
	dumptype2glyphs(sf,at);
    else if ( format==ff_otfcid )
	dumpcidglyphs(sf,at);
    else
	dumpglyphs(sf,&at->gi);
    at->head.xmin = at->gi.xmin;
    at->head.ymin = at->gi.ymin;
    at->head.xmax = at->gi.xmax;
    at->head.ymax = at->gi.ymax;
    sethead(&at->head,sf);
    sethhead(&at->hhead,at,sf);
    setos2(&at->os2,at,sf,format);
    dumpnames(at,sf);
    at->hhead.numMetrics = at->maxp.numGlyphs;
    if ( at->gi.glyph_len<0x20000 )
	at->head.locais32 = 0;
    if ( format!=ff_otf && format!=ff_otfcid )
	redoloca(at);
    redohead(at);
    redohhead(at);
    redomaxp(at,format);
    redoos2(at);
    redocvt(at);
    dumpkerns(at,sf);
    dumppost(at,sf,format);
    dumpcmap(at,sf,format);

    if ( format==ff_otf || format==ff_otfcid ) {
	at->tabdir.version = CHR('O','T','T','O');
	at->tabdir.numtab = 9+(at->kernlen!=0);
    } else {
	at->tabdir.version = 0x00010000;
	at->tabdir.numtab = 11+(at->kernlen!=0);
    }
    at->tabdir.searchRange = 8*16;
    at->tabdir.entrySel = 3;
    at->tabdir.rangeShift = at->tabdir.numtab*16-at->tabdir.searchRange;

    i = 0;
    pos = sizeof(int32)+4*sizeof(int16) + at->tabdir.numtab*4*sizeof(int32);

    if ( format==ff_otf || format==ff_otfcid ) {
	at->tabdir.tabs[i].tag = CHR('C','F','F',' ');
	at->tabdir.tabs[i].checksum = filecheck(at->cfff);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->cfflen;
	pos += ((at->cfflen+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('O','S','/','2');
    at->tabdir.tabs[i].checksum = filecheck(at->os2f);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->os2len;
    pos += ((at->os2len+3)>>2)<<2;

    at->tabdir.tabs[i].tag = CHR('c','m','a','p');
    at->tabdir.tabs[i].checksum = filecheck(at->cmap);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->cmaplen;
    pos += ((at->cmaplen+3)>>2)<<2;

    if ( format!=ff_otf && format!=ff_otfcid ) {
	at->tabdir.tabs[i].tag = CHR('c','v','t',' ');
	at->tabdir.tabs[i].checksum = filecheck(at->cvtf);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->cvtlen;
	pos += ((at->cvtlen+3)>>2)<<2;

	at->tabdir.tabs[i].tag = CHR('g','l','y','f');
	at->tabdir.tabs[i].checksum = filecheck(at->gi.glyphs);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->gi.glyph_len;
	pos += ((at->gi.glyph_len+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('h','e','a','d');
    at->tabdir.tabs[i].checksum = filecheck(at->headf);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->headlen;
    pos += ((at->headlen+3)>>2)<<2;

    at->tabdir.tabs[i].tag = CHR('h','h','e','a');
    at->tabdir.tabs[i].checksum = filecheck(at->hheadf);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->hheadlen;
    pos += ((at->hheadlen+3)>>2)<<2;

    at->tabdir.tabs[i].tag = CHR('h','m','t','x');
    at->tabdir.tabs[i].checksum = filecheck(at->gi.hmtx);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->gi.hmtxlen;
    pos += sizeof(struct hmtx)*at->maxp.numGlyphs;

    if ( at->kern!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('k','e','r','n');
	at->tabdir.tabs[i].checksum = filecheck(at->kern);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->kernlen;
	pos += ((at->kernlen+3)>>2)<<2;
    }

    if ( format!=ff_otf && format!=ff_otfcid  ) {
	at->tabdir.tabs[i].tag = CHR('l','o','c','a');
	at->tabdir.tabs[i].checksum = filecheck(at->loca);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->localen;
	pos += ((at->localen+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('m','a','x','p');
    at->tabdir.tabs[i].checksum = filecheck(at->maxpf);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->maxplen;
    pos += ((at->maxplen+3)>>2)<<2;

    at->tabdir.tabs[i].tag = CHR('n','a','m','e');
    at->tabdir.tabs[i].checksum = filecheck(at->name);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->namelen;
    pos += ((at->namelen+3)>>2)<<2;

    at->tabdir.tabs[i].tag = CHR('p','o','s','t');
    at->tabdir.tabs[i].checksum = filecheck(at->post);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->postlen;
    pos += ((at->postlen+3)>>2)<<2;
}

static void dumpttf(FILE *ttf,struct alltabs *at, enum fontformat format) {
    int32 checksum;
    int i, head_index;
    /* I can't use fwrite because I have to byte swap everything */

    dumplong(ttf,at->tabdir.version);
    dumpshort(ttf,at->tabdir.numtab);
    dumpshort(ttf,at->tabdir.searchRange);
    dumpshort(ttf,at->tabdir.entrySel);
    dumpshort(ttf,at->tabdir.rangeShift);
    for ( i=0; i<at->tabdir.numtab; ++i ) {
	dumplong(ttf,at->tabdir.tabs[i].tag);
	dumplong(ttf,at->tabdir.tabs[i].checksum);
	dumplong(ttf,at->tabdir.tabs[i].offset);
	dumplong(ttf,at->tabdir.tabs[i].length);
    }

    i=0;
    if ( format==ff_otf || format==ff_otfcid)
	copyfile(ttf,at->cfff,at->tabdir.tabs[i++].offset);
    copyfile(ttf,at->os2f,at->tabdir.tabs[i++].offset);
    copyfile(ttf,at->cmap,at->tabdir.tabs[i++].offset);
    if ( format!=ff_otf && format!= ff_otfcid ) {
	copyfile(ttf,at->cvtf,at->tabdir.tabs[i++].offset);
	copyfile(ttf,at->gi.glyphs,at->tabdir.tabs[i++].offset);
    }
    head_index = i;
    copyfile(ttf,at->headf,at->tabdir.tabs[i++].offset);
    copyfile(ttf,at->hheadf,at->tabdir.tabs[i++].offset);
    copyfile(ttf,at->gi.hmtx,at->tabdir.tabs[i++].offset);
    if ( at->kern!=NULL )
	copyfile(ttf,at->kern,at->tabdir.tabs[i++].offset);
    if ( format!=ff_otf && format!=ff_otfcid )
	copyfile(ttf,at->loca,at->tabdir.tabs[i++].offset);
    copyfile(ttf,at->maxpf,at->tabdir.tabs[i++].offset);
    copyfile(ttf,at->name,at->tabdir.tabs[i++].offset);
    copyfile(ttf,at->post,at->tabdir.tabs[i++].offset);

    checksum = filecheck(ttf);
    checksum = 0xb1b0afba-checksum;
    fseek(ttf,at->tabdir.tabs[head_index].offset+2*sizeof(int32),SEEK_SET);
    dumplong(ttf,checksum);

    /* copyfile closed all the files (except ttf) */
}

int WriteTTFFont(char *fontname,SplineFont *sf,enum fontformat format) {
    struct alltabs at;
    FILE *ttf;
    char *oldloc;

    if (( ttf=fopen(fontname,"w+"))==NULL )
return( 0 );
    oldloc = setlocale(LC_NUMERIC,"C");		/* TrueType probably doesn't need this, but OpenType does for floats in dictionaries */
    if ( format==ff_otfcid ) {
	if ( sf->cidmaster ) sf = sf->cidmaster;
    } else {
	if ( sf->subfontcnt!=0 ) sf = sf->subfonts[0];
    }
    initTables(&at,sf,format);
    dumpttf(ttf,&at,format);
    setlocale(LC_NUMERIC,oldloc);
    if ( ferror(ttf)) {
	fclose(ttf);
return( 0 );
    }
    if ( fclose(ttf)==-1 )
return( 0 );
return( 1 );
}
    
/* Fontograpgher also generates: fpgm, hdmx, prep */
