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
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <utype.h>
#include <ustring.h>
#include <chardata.h>
#include <gwidget.h>

#include "ttf.h"

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

static SplinePoint *MakeQuadSpline(SplinePoint *start,Spline *ttf,real x,
	real y, real tmax,SplinePoint *oldend) {
    Spline *new = chunkalloc(sizeof(Spline));
    SplinePoint *end = chunkalloc(sizeof(SplinePoint));

    if ( tmax==1 ) {
	end->roundx = oldend->roundx; end->roundy = oldend->roundy; end->dontinterpolate = oldend->dontinterpolate;
	x = oldend->me.x; y = oldend->me.y;	/* Want it to compare exactly */
    }
    end->ptindex = -1;
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
    if ( tmax==1 ) {
	SplinePoint *oldend = ps->to;
	end->roundx = oldend->roundx; end->roundy = oldend->roundy; end->dontinterpolate = oldend->dontinterpolate;
	x = oldend->me.x; y = oldend->me.y;	/* Want it to compare exactly */
    }
    end->ptindex = -1;
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
return( MakeQuadSpline(start,&ttf,xmax,ymax,tmax,ps->to));
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
	sp->roundx = ps->to->roundx; sp->roundy = ps->to->roundy; sp->dontinterpolate = ps->to->dontinterpolate;
	sp->ptindex = -1;
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
	if ( dxdt ==0 )
	    cy = y;
	else
	    cy = (dydt/dxdt)*(cx-x)+y;
	/* Make the quadratic spline from (xmin,ymin) through (cx,cy) to (x,y)*/
	ttf.splines[0].d = xmin;
	ttf.splines[0].c = 2*(cx-xmin);
	ttf.splines[0].b = xmin+x-2*cx;
	ttf.splines[1].d = ymin;
	ttf.splines[1].c = 2*(cy-ymin);
	ttf.splines[1].b = ymin+y-2*cy;
	if ( comparespline(ps,&ttf,tmin,t) ) {
	    sp = MakeQuadSpline(start,&ttf,x,y,t,ps->to);
/* Without these two lines we would turn out the cps for cubic splines */
/*  (what postscript uses). With the lines we get the cp for the quadratic */
/* So with them we've got the wrong cps for cubic splines and can't manipulate*/
/*  em properly within the editor */
/* I made this conditional so that I could look at what I was generating with */
/*  pfaedit, but it's probably pointless now */
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
	GPOS		(opentype, if kern data are present)
	cvt		for hinting
*/

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
    { 0x370, 0x3ff },
    { 0,0 },		/* Obsolete */
    { 0x400, 0x4ff },
    { 0x530, 0x58f },
    { 0x590, 0x5ff },
    { 0,0 },		/* Obsolete */
    { 0x600, 0x6ff },
    { 0,0 },		/* Obsolete */
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
    { 0x10a0, 0x10ff },
    { 0, 0 },			/* bit 27, obsolete */
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
    { 0x3100, 0x312f },			/* Also includes 3180-31AF */
    { 0x3130, 0x318f },
    { 0x3190, 0x31ff },
    { 0x3200, 0x32ff },
    { 0x3300, 0x33ff },		/* bit 55 CJK compatability */
    { 0xac00, 0xd7af },		/* bit 56 Hangul */
    { 0, 0 },			/* subranges */
    { 0, 0 },			/* subranges */
    { 0x3400, 0x9fff },		/* bit 59, CJK */ /* also 2E80-29FF */
    { 0xE000, 0xF8FF },		/* bit 60, private use */
    { 0xf900, 0xfaff },
    { 0xfb00, 0xfb4f },
    { 0xfb50, 0xfdff },

    { 0xfe20, 0xfe2f },		/* bit 64 combining half marks */
    { 0xfe30, 0xfe4f },
    { 0xfe50, 0xfe6f },
    { 0xfe70, 0xfeef },
    { 0xff00, 0xffef },
    { 0xfff0, 0xffff },
    { 0x0f00, 0x0fff },		/* bit 70 tibetan */
    { 0x0700, 0x074f },
    { 0x0780, 0x07Bf },
    { 0x0D80, 0x0Dbf },
    { 0x1000, 0x109f },
    { 0x1200, 0x12ff },		/* bit 75 ethiopic */
    { 0x13A0, 0x13ff },
    { 0x1400, 0x167f },
    { 0x1680, 0x169f },
    { 0x16A0, 0x16ff },
    { 0x1780, 0x17ff },		/* bit 80 khmer */
    { 0x1800, 0x18af },
    { 0x2800, 0x28ff },
    { 0xA000, 0xa4af },		/* bit 83, Yi & Yi radicals */
    { 0xffff, 0xffff}
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

void putshort(FILE *file,int sval) {
    putc((sval>>8)&0xff,file);
    putc(sval&0xff,file);
}

void putlong(FILE *file,int val) {
    putc((val>>24)&0xff,file);
    putc((val>>16)&0xff,file);
    putc((val>>8)&0xff,file);
    putc(val&0xff,file);
}
#define dumpabsoffset	putlong

static void dumpoffset(FILE *file,int offsize,int val) {
    if ( offsize==1 )
	putc(val,file);
    else if ( offsize==2 )
	putshort(file,val);
    else if ( offsize==3 ) {
	putc((val>>16)&0xff,file);
	putc((val>>8)&0xff,file);
	putc(val&0xff,file);
    } else
	putlong(file,val);
}

static void put2d14(FILE *file,real dval) {
    int val;
    int mant;

    val = floor(dval);
    mant = floor(16384.*(dval-val));
    val = (val<<14) | mant;
    putshort(file,val);
}

void putfixed(FILE *file,real dval) {
    int val;
    int mant;

    val = floor(dval);
    mant = floor(65536.*(dval-val));
    val = (val<<16) | mant;
    putlong(file,val);
}

int ttfcopyfile(FILE *ttf, FILE *other, int pos) {
    int ch;
    int ret = 1;

    if ( pos!=ftell(ttf))
	GDrawIError("File Offset wrong for ttf table, %d expected %d", ftell(ttf), pos );
    rewind(other);
    while (( ch = getc(other))!=EOF )
	putc(ch,ttf);
    if ( ferror(other)) ret = 0;
    if ( fclose(other)) ret = 0;
return( ret );
}

static int _getcvtval(struct glyphinfo *gi,int val) {
    int i;

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

static int getcvtval(struct glyphinfo *gi,int val) {

    /* by default sign is unimportant in the cvt */
    /* For some instructions anyway, but not for MIAP so this routine has */
    /*  been broken in two. */
    if ( val<0 ) val = -val;
return( _getcvtval(gi,val));
}

static void FigureFullMetricsEnd(SplineFont *sf,struct glyphinfo *gi) {
    /* We can reduce the size of the width array by removing a run at the end */
    /*  of the same width. So start at the end, find the width of the last */
    /*  character we'll output, then run backwards as long as we've got the */
    /*  same width */
    /* (do same thing for vertical metrics too */
    int i,j, maxc, lasti, lastv;
    int width, vwidth;

    maxc = sf->charcnt;
    for ( j=0; j<sf->subfontcnt; ++j )
	if ( sf->subfonts[j]->charcnt>maxc ) maxc = sf->subfonts[j]->charcnt;
    if ( sf->subfontcnt==0 ) {
	for ( i=maxc-1; i>0; --i )
	    if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]==SCDuplicate(sf->chars[i]))
	break;
    } else {
	for ( i=maxc-1; i>0; --i ) {
	    for ( j=0; j<sf->subfontcnt; ++j )
		if ( i<sf->subfonts[j]->charcnt && sf->subfonts[j]->chars[i]!=NULL )
	    break;
	    if ( j<sf->subfontcnt && SCWorthOutputting(sf->subfonts[j]->chars[i]))
	break;	/* no duplicates in cid font */
	}
    }

    if ( i>0 ) {
	lasti = lastv = i;
	if ( sf->subfontcnt==0 ) {
	    width = sf->chars[i]->width;
	    vwidth = sf->chars[i]->vwidth;
	    for ( --i; i>0; --i ) {
		if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]==SCDuplicate(sf->chars[i])) {
		    if ( sf->chars[i]->width!=width )
	    break;
		    else
			lasti = i;
		}
	    }
	    gi->lasthwidth = lasti;
	    if ( sf->hasvmetrics ) {
		for ( i=lastv-1; i>0; --i ) {
		    if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]==SCDuplicate(sf->chars[i])) {
			if ( sf->chars[i]->vwidth!=vwidth )
		break;
			else
			    lastv = i;
		    }
		}
		gi->lastvwidth = lastv;
	    }
	} else {
	    width = sf->subfonts[j]->chars[i]->width;
	    vwidth = sf->subfonts[j]->chars[i]->vwidth;
	    for ( --i; i>0; --i ) {
		for ( j=0; j<sf->subfontcnt; ++j )
		    if ( i<sf->subfonts[j]->charcnt && sf->subfonts[j]->chars[i]!=NULL )
		break;
		if ( j<sf->subfontcnt && SCWorthOutputting(sf->subfonts[j]->chars[i])) {
		    if ( sf->subfonts[j]->chars[i]->width!=width )
	    break;
		    else
			lasti = i;
		}
	    }
	    gi->lasthwidth = lasti;
	    if ( sf->hasvmetrics ) {
		for ( i=lastv-1; i>0; --i ) {
		    for ( j=0; j<sf->subfontcnt; ++j )
			if ( i<sf->subfonts[j]->charcnt && sf->subfonts[j]->chars[i]!=NULL )
		    break;
		    if ( j<sf->subfontcnt && SCWorthOutputting(sf->subfonts[j]->chars[i])) {
			if ( sf->subfonts[j]->chars[i]->vwidth!=vwidth )
		break;
			else
			    lastv = i;
		    }
		}
		gi->lastvwidth = lastv;
	    }
	}
    }
}

static void dumpghstruct(struct glyphinfo *gi,struct glyphhead *gh) {

    putshort(gi->glyphs,gh->numContours);
    putshort(gi->glyphs,gh->xmin);
    putshort(gi->glyphs,gh->ymin);
    putshort(gi->glyphs,gh->xmax);
    putshort(gi->glyphs,gh->ymax);
    if ( gh->xmin<gi->xmin ) gi->xmin = gh->xmin;
    if ( gh->ymin<gi->ymin ) gi->ymin = gh->ymin;
    if ( gh->xmax>gi->xmax ) gi->xmax = gh->xmax;
    if ( gh->ymax>gi->ymax ) gi->ymax = gh->ymax;
}

static void ttfdumpmetrics(SplineChar *sc,struct glyphinfo *gi,DBounds *b) {

    if ( sc->enc<=gi->lasthwidth )
	putshort(gi->hmtx,sc->width);
    putshort(gi->hmtx,b->minx);
    if ( sc->parent->hasvmetrics ) {
	if ( sc->enc<=gi->lastvwidth )
	    putshort(gi->vmtx,sc->vwidth);
	putshort(gi->vmtx,sc->parent->vertical_origin-b->maxy);
    }
    if ( sc->enc==gi->lasthwidth )
	gi->hfullcnt = sc->ttf_glyph+1;
    if ( sc->enc==gi->lastvwidth )
	gi->vfullcnt = sc->ttf_glyph+1;
}

static SplineSet *SCttfApprox(SplineChar *sc) {
    SplineSet *head=NULL, *last, *ss, *tss;
    RefChar *ref;

    for ( ss=sc->splines; ss!=NULL; ss=ss->next ) {
	tss = SSttfApprox(ss);
	if ( head==NULL ) head = tss;
	else last->next = tss;
	last = tss;
    }
    for ( ref=sc->refs; ref!=NULL; ref=ref->next ) {
	for ( ss=ref->splines; ss!=NULL; ss=ss->next ) {
	    tss = SSttfApprox(ss);
	    if ( head==NULL ) head = tss;
	    else last->next = tss;
	    last = tss;
	}
    }
return( head );
}
    
static int SSPointCnt(SplineSet *ss) {
    SplinePoint *sp, *first=NULL;
    int cnt;

    for ( sp=ss->first, cnt=0; sp!=first ; ) {
	if ( sp==ss->first || sp->nonextcp || sp->noprevcp ||
		(sp->dontinterpolate || sp->roundx || sp->roundy) ||
		(sp->prevcp.x+sp->nextcp.x)/2!=sp->me.x ||
		(sp->prevcp.y+sp->nextcp.y)/2!=sp->me.y )
	    ++cnt;
	if ( !sp->nonextcp ) ++cnt;
	if ( sp->next==NULL )
    break;
	if ( first==NULL ) first = sp;
	sp = sp->next->to;
    }
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

    first = NULL;
    for ( sp=ss->first; sp!=first ; ) {
	if ( sp==ss->first || sp->nonextcp || sp->noprevcp ||
		(sp->dontinterpolate || sp->roundx || sp->roundy) ||
		(sp->prevcp.x+sp->nextcp.x)/2!=sp->me.x ||
		(sp->prevcp.y+sp->nextcp.y)/2!=sp->me.y ) {
	    /* If an on curve point is midway between two off curve points*/
	    /*  it may be omitted and will be interpolated on read in */
	    if ( flags!=NULL ) flags[ptcnt] = _On_Curve;
	    bp[ptcnt].x = rint(sp->me.x);
	    bp[ptcnt].y = rint(sp->me.y);
	    sp->ptindex = ptcnt++;
	}
	if ( !sp->nonextcp ) {
	    if ( flags!=NULL ) flags[ptcnt] = 0;
	    bp[ptcnt].x = rint(sp->nextcp.x);
	    bp[ptcnt++].y = rint(sp->nextcp.y);
	}
	if ( sp->next==NULL )
    break;
	if ( first==NULL ) first = sp;
	sp = sp->next->to;
    }
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
	    putshort(gi->glyphs,bp[i].x-last.x);
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
	    putshort(gi->glyphs,bp[i].y-last.y);
	last.y = bp[i].y;
    }
    if ( ftell(gi->glyphs)&1 )		/* Pad the file so that the next glyph */
	putc('\0',gi->glyphs);		/* on a word boundary */
}

static void dumpinstrs(struct glyphinfo *gi,uint8 *instrs,int cnt) {
    int i;

 /*cnt = 0;		/* Debug */
    if ( gi->maxp->maxglyphInstr<cnt ) gi->maxp->maxglyphInstr=cnt;
    putshort(gi->glyphs,cnt);
    for ( i=0; i<cnt; ++i )
	putc( instrs[i],gi->glyphs );
}

static void dumpmissingglyph(SplineFont *sf,struct glyphinfo *gi,int fixedwidth) {
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
    putshort(gi->glyphs,4-1);
    putshort(gi->glyphs,8-1);
	/* instruction length&instructions */
    dumpinstrs(gi,instrs,46);

    dumppointarrays(gi,bp,NULL,8);

    if ( fixedwidth==-1 )
	putshort(gi->hmtx,gh.xmax + 2*stem);
    else
	putshort(gi->hmtx,fixedwidth);
    putshort(gi->hmtx,stem);
    if ( sf->hasvmetrics ) {
	putshort(gi->vmtx,sf->ascent+sf->descent);
	putshort(gi->vmtx,sf->vertical_origin-gh.ymax);
    }
}

static void dumpblankglyph(struct glyphinfo *gi,SplineFont *sf) {
    /* These don't get a glyph header, because there are no contours */
    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);
    putshort(gi->hmtx,gi->next_glyph==2?0:(sf->ascent+sf->descent)/3);
    putshort(gi->hmtx,0);
    if ( sf->hasvmetrics ) {
	putshort(gi->vmtx,gi->next_glyph==2?0:(sf->ascent+sf->descent));
	putshort(gi->vmtx,0);
    }
}

static void dumpspace(SplineChar *sc, struct glyphinfo *gi) {
    /* These don't get a glyph header, because there are no contours */
    DBounds b;
    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);
    memset(&b,0,sizeof(b));
    ttfdumpmetrics(sc,gi,&b);
}

static uint8 *pushheader(uint8 *instrs, int isword, int tot) {
    if ( isword ) {
	if ( tot>8 ) {
	    *instrs++ = 0x41;		/* N(next byte) Push words */
	    *instrs++ = tot;
	} else
	    *instrs++ = 0xb8+(tot-1);	/* Push Words */
    } else {
	if ( tot>8 ) {
	    *instrs++ = 0x40;		/* N(next byte) Push bytes */
	    *instrs++ = tot;
	} else
	    *instrs++ = 0xb0+(tot-1);	/* Push bytes */
    }
return( instrs );
}

static uint8 *addpoint(uint8 *instrs,int isword,int pt) {
    if ( !isword ) {
	*instrs++ = pt;
    } else {
	*instrs++ = pt>>8;
	*instrs++ = pt&0xff;
    }
return( instrs );
}

static uint8 *pushpoint(uint8 *instrs,int pt) {
    instrs = pushheader(instrs,pt>255,1);
return( addpoint(instrs,pt>255,pt));
}

static uint8 *pushpointstem(uint8 *instrs,int pt, int stem) {
    int isword = pt>255 || stem>255;
    instrs = pushheader(instrs,isword,2);
    instrs = addpoint(instrs,isword,pt);
return( addpoint(instrs,isword,stem));
}


static uint8 *SetRP0To(uint8 *instrs, real hbase, BasePoint *bp, int ptcnt,
	int xdir, real fudge) {
    int i;

    for ( i=0; i<ptcnt; ++i ) {
	if ( (xdir && bp[i].x==hbase) || (!xdir && bp[i].y==hbase) ) {
	    instrs = pushpoint(instrs,i);
	    *instrs++ = 0x10;		/* Set RP0, SRP0 */
return( instrs );
	}
    }
    /* couldn't find anything, fudge a little... */
    for ( i=0; i<ptcnt; ++i ) {
	if ( (xdir && bp[i].x>=hbase-fudge && bp[i].x<=hbase+fudge) ||
		(!xdir && bp[i].y>=hbase-fudge && bp[i].y<=hbase+fudge) ) {
	    instrs = pushpoint(instrs,i);
	    *instrs++ = 0x10;		/* Set RP0, SRP0 */
return( instrs );
	}
    }
return( instrs );
}

static real BDFindValue(real base, BlueData *bd, int isbottom ) {
    real replace = 0x80000000;

    if ( isbottom ) {
	if ( base>= bd->basebelow && base<=bd->base )
	    replace = 0;
    } else {
	if ( base>=bd->caph && base<=bd->caphtop )
	    replace = bd->caph;
	else if ( base>=bd->xheight && base<=bd->xheighttop )
	    replace = bd->xheight;
    }
    /* I'm allowing ascent and descent's to wiggle */
return( replace );
}

/* Run through all points. For any on this hint's start, position them */
/*  If the first point of this hint falls in a blue zone, do a cvt based */
/*	positioning, else
/*  The first point on the first hint is positioned to where it is (dull) */
/*	(or if the last hint overlaps this hint) */
/*  The first point of this hint is positioned offset from the last hint */
/*	(unless the two overlap) */
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
	int *contourends, BasePoint *bp, int ptcnt, StemInfo *firsthint, int xdir,
	uint8 *touched) {
    int i;
    int last= -1;
    int stem, basecvt=-1;
    real hbase, base, width, newbase;
    StemInfo *h;
    real fudge = gi->fudge;
    int inrp;
    StemInfo *lasthint=NULL, *testhint;
    int first;

    for ( testhint = firsthint; testhint!=NULL && testhint!=hint; testhint = testhint->next ) {
	if ( HIoverlap(testhint->where,hint->where)!=0 )
	    lasthint = testhint;
    }
    first = lasthint==NULL;
    if ( hint->hasconflicts )
	first = true;		/* if this hint has conflicts don't try to establish a minimum distance between it and the last stem, there might not be one */

    hbase = base = rint(hint->start); width = rint(hint->width);
    if ( !xdir ) {
	/* check the "bluevalues" for things like cap height, xheight and */
	/*  baseline. But only check if the top of a stem is at capheight */
	/*  or xheight, and only if the bottom is at baseline */
	if ( width<0 ) {
	    hbase = (base += width);
	    width = -width;
	}
	if ( (newbase = BDFindValue(base,&gi->bd,true))!= 0x80000000 ) {
	    base = newbase;
	    basecvt = _getcvtval(gi,(int)base);
	}
	if ( basecvt == -1 && !hint->startdone ) {
	    hbase = (base += width);
	    if ( (newbase = BDFindValue(base,&gi->bd,false))!= 0x80000000 ) {
		base = newbase;
		basecvt = _getcvtval(gi,(int)base);
	    }
	    if ( basecvt!=-1 )
		width = -width;
	    else
		hbase = (base -= width);
	}
    }
    if ( hbase==rint(hint->start) && hint->enddone ) {
	base = (hbase += width);
	width = -width;
	basecvt = -1;
    }

    /* Position all points on this hint's base */
    if (( width>0 && !hint->startdone) || (width<0 && !hint->enddone)) {
	inrp = -1;
	for ( i=0; i<ptcnt; ++i ) {
	    if ( (xdir && bp[i].x>=hbase-fudge && bp[i].x<=hbase+fudge) ||
		    (!xdir && bp[i].y>=hbase-fudge && bp[i].y<=hbase+fudge) ) {
		if ( basecvt!=-1 && last==-1 ) {
		    instrs = pushpointstem(instrs,i,basecvt);
		    *instrs++ = 0x3f;		/* MIAP, rounded, set rp0,rp1 */
		    first = false;
		    inrp = 1;
		} else {
		    instrs = pushpoint(instrs,i);
		    if ( first ) {
			/* set rp0 */
			*instrs++ = 0x2f;	/* MDAP, rounded, set rp0,rp1 */
			first = false;
			inrp = 1;
		    } else if ( last==-1 ) {
			/* set rp0 relative to last hint */
			instrs = SetRP0To(instrs,lasthint->width>0?lasthint->start+lasthint->width:lasthint->start,
				bp,ptcnt,xdir,fudge);
			*instrs++ = 0xc0+0x1e;	/* MDRP, set rp0,rp2, minimum, rounded, white */
			inrp = 2;
		    } else {
			*instrs++ = inrp==1?0x33:0x32;	/* SHP, rp1 or rp2 */
		    }
		}
		touched[i] |= (xdir?1:2);
		last = i;
	    }
	}
	if ( last==-1 )		/* I'm confused. But if the hint doesn't start*/
return(instrs);			/*  anywhere, can't give it a width */
				/* Some hints aren't associated with points */
    } else {
	/* Need to find something to be a reference point, doesn't matter */
	/*  what. Note that it should already have been positioned */
	instrs = SetRP0To(instrs,hbase, bp,ptcnt,xdir,fudge);
    }

    /* Position all points on this hint's base+width */
    stem = getcvtval(gi,width);
    last = -1;
    for ( i=0; i<ptcnt; ++i ) {
	if ( (xdir && bp[i].x>=hbase+width-fudge && bp[i].x<=hbase+width+fudge) ||
		(!xdir && bp[i].y>=hbase+width-fudge && bp[i].y<=hbase+width+fudge) ) {
	    instrs = pushpointstem(instrs,i,stem);
	    *instrs++ = 0xe0+0x0d;	/* MIRP, minimum, rounded, black */
	    touched[i] |= (xdir?1:2);
	    last = i;
	}
    }

    for ( h=hint->next; h!=NULL && h->start<=hint->start+hint->width; h=h->next ) {
	if ( (h->start>=hint->start-gi->fudge && h->start<=hint->start+gi->fudge) ||
		(h->start>=hint->start+hint->width-gi->fudge && h->start<=hint->start+hint->width+gi->fudge) )
	    h->startdone = true;
	if ( (h->start+h->width>=hint->start-gi->fudge && h->start+h->width<=hint->start+gi->fudge) ||
		(h->start+h->width>=hint->start+hint->width-gi->fudge && h->start+h->width<=hint->start+hint->width+gi->fudge) )
	    h->enddone = true;
    }

return( instrs );
}

/* diagonal stem hints */
/* Several DStemInfo hints may actually be colinear. This structure contains all on the line */
typedef struct dstem {
    struct dstem *next;
    BasePoint leftedgetop, leftedgebottom, rightedgetop, rightedgebottom;
    int pnum[4];
    /*struct dsteminfolist { struct dsteminfolist *next; DStemInfo *d; int pnum[4];} *dl;*/
    uint8 *used;	/* Points lying near this diagonal 1=>left edge, 2=> right, 0=> off */
    struct dstemlist { struct dstemlist *next; struct dstem *ds; BasePoint *is[4]; int pnum[4]; int done;} *intersects;
    struct dstemlist *top, *bottom;
    unsigned int done: 1;
} DStem;
enum intersect { in_ll, in_lr, in_rl, in_rr };	/* intersection of: two left edges, left/right, right/left, right/right */

static int CoLinear(BasePoint *top1, BasePoint *bottom1,
			BasePoint *top2, BasePoint *bottom2 ) {
    double scale, slope, y;

    if ( top1->y==bottom1->y )
return( RealWithin(top1->y,top2->y,1) && RealWithin(top1->y,bottom2->y,1) );
    else if ( top2->y==bottom2->y )
return( RealWithin(top2->y,top1->y,1) && RealWithin(top2->y,bottom1->y,1) );

    scale = (top2->y-bottom2->y)/(top1->y-bottom1->y);
    slope = (top1->y-bottom1->y)/(top1->x-bottom1->x);
    if ( !RealWithin(top2->x,bottom2->x+(top1->x-bottom1->x)*scale,10) &&
	    !RealWithin(top2->y,bottom2->y+slope*(top2->x-bottom2->x),12) )
return( false );

    y = bottom1->y + slope*(top2->x-bottom1->x);
    if ( !RealWithin(y,top2->y,6) )
return( false );

return( true );
}

static int BpIndex(BasePoint *search,BasePoint *bp,int ptcnt) {
    int i;

    for ( i=0; i<ptcnt; ++i )
	if ( rint(search->x) == bp[i].x && rint(search->y)==bp[i].y )
return( i );

return( -1 );
}

static DStem *DStemMerge(DStemInfo *d, BasePoint *bp, int ptcnt, uint8 *touched) {
    DStemInfo *di, *di2;
    DStem *head=NULL, *cur;
    int i, j, nexti;
    BasePoint *top, *bottom;
    DStem **map[2];

    for ( di=d; di!=NULL; di=di->next ) di->used = false;
    for ( di=d; di!=NULL; di=di->next ) if ( !di->used ) {
	cur = chunkalloc(sizeof(DStem));
	memset(cur->pnum,-1,sizeof(cur->pnum));
	cur->used = gcalloc(ptcnt,sizeof(uint8));
	cur->leftedgetop = di->leftedgetop;
	cur->pnum[0] = BpIndex(&di->leftedgetop,bp,ptcnt);
	cur->leftedgebottom = di->leftedgebottom;
	cur->pnum[1] = BpIndex(&di->leftedgebottom,bp,ptcnt);
	cur->rightedgetop = di->rightedgetop;
	cur->pnum[2] = BpIndex(&di->rightedgetop,bp,ptcnt);
	cur->rightedgebottom = di->rightedgebottom;
	cur->pnum[3] = BpIndex(&di->rightedgebottom,bp,ptcnt);
	cur->used[cur->pnum[0]] = cur->used[cur->pnum[1]] = 1;
	cur->used[cur->pnum[2]] = cur->used[cur->pnum[3]] = 2;
	cur->next = head;
	head = cur;
	for ( di2 = di->next; di2!=NULL; di2 = di2->next ) if ( !di2->used ) {
	    if (( CoLinear(&di->leftedgetop, &di->leftedgebottom,
			&di2->leftedgetop, &di2->leftedgebottom) &&
		    CoLinear(&di->rightedgetop, &di->rightedgebottom,
			&di2->rightedgetop, &di2->rightedgebottom) ) ||
		    (di->leftedgetop.x==di2->leftedgetop.x &&
		     di->leftedgetop.y==di2->leftedgetop.y &&
		     di->leftedgebottom.x==di2->leftedgebottom.x &&
		     di->leftedgebottom.y==di2->leftedgebottom.y ) ||
		    (di->rightedgetop.x==di2->rightedgetop.x &&
		     di->rightedgetop.y==di2->rightedgetop.y &&
		     di->rightedgebottom.x==di2->rightedgebottom.x &&
		     di->rightedgebottom.y==di2->rightedgebottom.y )) {
		if ( di->leftedgetop.x!=di2->leftedgetop.x ||
			 di->leftedgetop.y!=di2->leftedgetop.y ) {
		    i = BpIndex(&di2->leftedgetop,bp,ptcnt);
		    cur->used[i] = 1;
		    if ( cur->leftedgetop.y<di2->leftedgetop.y ) {
			cur->leftedgetop = di2->leftedgetop;
			cur->pnum[0] = i;
		    }
		}
		if ( di->leftedgebottom.x!=di2->leftedgebottom.x ||
			 di->leftedgebottom.y!=di2->leftedgebottom.y ) {
		    i = BpIndex(&di2->leftedgebottom,bp,ptcnt);
		    cur->used[i] = 1;
		    if ( cur->leftedgebottom.y>di2->leftedgebottom.y ) {
			cur->leftedgebottom = di2->leftedgebottom;
			cur->pnum[1] = i;
		    }
		}
		if ( di->rightedgetop.x!=di2->rightedgetop.x ||
			 di->rightedgetop.y!=di2->rightedgetop.y ) {
		    i = BpIndex(&di2->rightedgetop,bp,ptcnt);
		    cur->used[i] = 2;
		    if ( cur->rightedgetop.y<di2->rightedgetop.y ) {
			cur->rightedgetop = di2->rightedgetop;
			cur->pnum[2] = i;
		    }
		}
		if ( di->rightedgebottom.x!=di2->rightedgebottom.x ||
			 di->rightedgebottom.y!=di2->rightedgebottom.y ) {
		    i = BpIndex(&di2->rightedgebottom,bp,ptcnt);
		    cur->used[i] = 2;
		    if ( cur->rightedgebottom.y>di2->rightedgebottom.y ) {
			cur->rightedgebottom = di2->rightedgebottom;
			cur->pnum[3] = i;
		    }
		}
		di2->used = true;
	    }
	}
    }

    map[0] = gcalloc(ptcnt,sizeof(DStem *));
    map[1] = gcalloc(ptcnt,sizeof(DStem *));
    for ( cur = head; cur!=NULL; cur=cur->next ) {
	for ( i=0; i<ptcnt; ++i ) {
	    if ( cur->used[i]) {
		if ( map[0][i]==NULL ) map[0][i] = cur;
		else map[1][i] = cur;
	    }
	}
    }

    /* sometimes we don't find all the bits of a diagonal stem */
    /* "k" is an example, we don't notice the stub between the vertical stem and the lower diagonal stem */
    for ( cur = head; cur!=NULL; cur=cur->next ) {
	for ( i=0; i<ptcnt; ++i ) {
	    nexti = i+1;
	    if ( touched[i]&tf_endcontour ) {
		for ( j=i; j>0 && !(touched[j]&tf_startcontour); --j);
		nexti = j;
	    }
	    if ( map[1][i]!=NULL || map[1][nexti]!=NULL ||
		    (map[0][i]!=NULL && map[0][i]==map[0][nexti] ))
	continue;	/* These points are already on a diagonal, off just enough that we can't find a line... */
	    if ( CoLinear(&cur->leftedgetop,&cur->leftedgebottom,&bp[i],&bp[nexti])) {
		top = &bp[i]; bottom = &bp[nexti];
		if ( top->y<bottom->y ) { top = bottom; bottom = &bp[i]; }
		cur->used[i] = cur->used[nexti] = 1;
		if ( cur->leftedgetop.y>top->y ) {
		    cur->leftedgetop = *top;
		    cur->pnum[0] = i;
		}
		if ( cur->leftedgebottom.y>bottom->y ) {
		    cur->leftedgebottom = *bottom;
		    cur->pnum[1] = i;
		}
		if ( map[0][i]==NULL ) map[0][i] = cur;
		else map[1][i] = cur;
		if ( map[0][nexti]==NULL ) map[0][nexti] = cur;
		else map[1][nexti] = cur;
	    } else if ( CoLinear(&cur->rightedgetop,&cur->rightedgebottom,&bp[i],&bp[nexti])) {
		top = &bp[i]; bottom = &bp[nexti];
		if ( top->y<bottom->y ) { top = bottom; bottom = &bp[i]; }
		cur->used[i] = cur->used[nexti] = 2;
		if ( cur->rightedgetop.y>top->y ) {
		    cur->rightedgetop = *top;
		    cur->pnum[2] = i;
		}
		if ( cur->rightedgebottom.y>bottom->y ) {
		    cur->rightedgebottom = *bottom;
		    cur->pnum[3] = i;
		}
		if ( map[0][i]==NULL ) map[0][i] = cur;
		else map[1][i] = cur;
		if ( map[0][nexti]==NULL ) map[0][nexti] = cur;
		else map[1][nexti] = cur;
	    }
	}
    }

    free(map[0]);
    free(map[1]);

return( head );
}

static void DStemFree(DStem *d) {
    DStem *next;
    struct dstemlist *dsl, *dslnext;

    while ( d!=NULL ) {
	next = d->next;
	for ( dsl=d->intersects; dsl!=NULL; dsl=dslnext ) {
	    dslnext = dsl->next;
	    chunkfree(dsl,sizeof(struct dstemlist));
	}
	free(d->used);
	chunkfree(d,sizeof(DStem));
	d = next;
    }
}

static struct dstemlist *BuildDStemIntersection(DStem *d,DStem *ds,
	struct dstemlist *i1, struct dstemlist **_i2,
	BasePoint *inter, enum intersect which, int pnum) {
    struct dstemlist *i2 = *_i2;

    if ( i1==NULL ) {
	i1 = chunkalloc(sizeof(struct dstemlist));
	memset(i1->pnum,-1,sizeof(i1->pnum));
	i1->ds = ds;
	i1->next = d->intersects;
	d->intersects = i1;
	*_i2 = i2 = chunkalloc(sizeof(struct dstemlist));
	memset(i2->pnum,-1,sizeof(i2->pnum));
	i2->ds = d;
	i2->next = ds->intersects;
	ds->intersects = i2;
    }
    if ( i1->is[which]!=NULL )
	GDrawIError("BuildDStemIntersection: is[%d] set twice", which);
    i1->is[which] = inter;
    i1->pnum[which] = pnum;
    if ( which==in_rl ) which = in_lr;
    else if ( which==in_lr ) which = in_rl;
    i2->is[which] = inter;
    i2->pnum[which] = pnum;
return( i1 );
}

static void DStemIntersect(DStem *d,BasePoint *bp,int ptcnt) {
    /* For each DStem, see if it intersects any other dstems */
    /* This is combinatorial hell. the only relief is that there are usually */
    /*  not very many dstems */
    DStem *ds;
    struct dstemlist *i1, *i2;
    int i;
    real maxy1, miny1, maxy2, miny2;

    while ( d!=NULL ) {
	maxy1 = d->leftedgetop.y>d->rightedgetop.y?d->leftedgetop.y:d->rightedgetop.y;
	miny1 = d->leftedgebottom.y<d->rightedgebottom.y?d->leftedgebottom.y:d->rightedgebottom.y;
	for ( ds=d->next; ds!=NULL; ds=ds->next ) {
	    maxy2 = ds->leftedgetop.y>ds->rightedgetop.y?ds->leftedgetop.y:ds->rightedgetop.y;
	    miny2 = ds->leftedgebottom.y<ds->rightedgebottom.y?ds->leftedgebottom.y:ds->rightedgebottom.y;
	    if ( maxy1<miny2 || maxy2<miny1 )
	continue;

	    i1 = i2 = NULL;
	    for ( i=0; i<ptcnt; ++i ) {
		if ( d->used[i] && ds->used[i] ) {
		    i1 = BuildDStemIntersection(d,ds,i1,&i2,
			    &bp[i],(d->used[i]==1?0:1)+(ds->used[i]==1?0:2),i);
		}
	    }
	}
	d = d->next;
    }
}

/* There is always the possibility that the top might intersect two different*/
/*  dstems (an example would be a misaligned X) */
static struct dstemlist *DStemTopAtIntersection(DStem *ds) {
    struct dstemlist *dl, *found=NULL;

    for ( dl=ds->intersects; dl!=NULL; dl=dl->next ) {
	/* check left and right top points */
	if ( ds->pnum[0] == dl->pnum[in_ll] || ds->pnum[0]==dl->pnum[in_lr] || ds->pnum[0]==dl->pnum[in_rl] ||
		ds->pnum[2] == dl->pnum[in_rr] || ds->pnum[2]==dl->pnum[in_lr] || ds->pnum[2]==dl->pnum[in_rl] ) {
	    if ( found!=NULL )
return( NULL );
	    else
		found = dl;
	}
    }
return( found );
}

static struct dstemlist *DStemBottomAtIntersection(DStem *ds) {
    struct dstemlist *dl, *found=NULL;

    for ( dl=ds->intersects; dl!=NULL; dl=dl->next ) {
	/* check left and right bottom points */
	if ( ds->pnum[1] == dl->pnum[in_ll] || ds->pnum[1]==dl->pnum[in_lr] || ds->pnum[1]==dl->pnum[in_rl] ||
		ds->pnum[3] == dl->pnum[in_rr] || ds->pnum[3]==dl->pnum[in_lr] || ds->pnum[3]==dl->pnum[in_rl] ) {
	    if ( found!=NULL )
return( NULL );
	    else
		found = dl;
	}
    }
return( found );
}

static int IsVStemIntersection(struct dstemlist *dl) {
    DStem *ds = dl->ds;

    /* We are passed an intersection at the end point of one stem. We want to */
    /*  know if the intersection is also at an end-point of the other stem */
    /*  if it is at the end-point then it is a "V" intersection, otherwise */
    /*  it might be a "k". */
    if ( ds->pnum[0] == dl->pnum[in_ll] || ds->pnum[0]==dl->pnum[in_lr] || ds->pnum[0]==dl->pnum[in_rl] ||
	    ds->pnum[1] == dl->pnum[in_ll] || ds->pnum[1]==dl->pnum[in_lr] || ds->pnum[1]==dl->pnum[in_rl] ||
	    ds->pnum[2] == dl->pnum[in_rr] || ds->pnum[2]==dl->pnum[in_lr] || ds->pnum[2]==dl->pnum[in_rl] ||
	    ds->pnum[3] == dl->pnum[in_rr] || ds->pnum[3]==dl->pnum[in_lr] || ds->pnum[3]==dl->pnum[in_rl] )
return( true );

return( false );
}

static int DStemWidth(DStem *ds) {
    /* find the orthogonal distance from the left stem to the right. Make it positive */
    /*  (just a dot product with the unit vector orthog to the left edge) */
    double tempx, tempy, len, stemwidth;

    tempx = ds->leftedgetop.x-ds->leftedgebottom.x;
    tempy = ds->leftedgetop.y-ds->leftedgebottom.y;
    len = sqrt(tempx*tempx+tempy*tempy);
    stemwidth = ((ds->rightedgetop.x-ds->leftedgetop.x)*tempy -
	    (ds->rightedgetop.y-ds->leftedgetop.y)*tempx)/len;
    if ( stemwidth<0 ) stemwidth = -stemwidth;
return( rint(stemwidth));
}

static int ExamineFreedom(int index,BasePoint *bp,int ptcnt,int def) {
    if ( index>0 && bp[index-1].x==bp[index].x )
	def = 1;
    else if ( index>0 && bp[index-1].y==bp[index].y )
	def = 0;
    else if ( index==0 && bp[ptcnt-1].x==bp[index].x )
	def = 1;
    else if ( index==0 && bp[ptcnt-1].y==bp[index].y )
	def = 0;
    else if ( index<ptcnt-1 && bp[index+1].x==bp[index].x )
	def = 1;
    else if ( index<ptcnt-1 && bp[index+1].y==bp[index].y )
	def = 0;
    else if ( index==ptcnt-1 && bp[0].x==bp[index].x )
	def = 1;
    else if ( index==ptcnt-1 && bp[0].y==bp[index].y )
	def = 0;
return( def );
}

static uint8 *KStemMoveToEdge(struct glyphinfo *gi, uint8 *pt,
	struct dstemlist *dl,DStem *ds,int top,uint8 *touched) {
    /* we want to insure that either the top two points of ds or the bottom */
    /*  two are on the line specified by dl->ds */
    int e1, e2, m1, m2;
    real d1, d2;
    BasePoint *bp = top ? &ds->leftedgetop : &ds->leftedgebottom;
    int zerocvt = getcvtval(gi,0);
    int data[7];
    int isword, i;

    d1 = (dl->ds->leftedgetop.y-dl->ds->leftedgebottom.y)*(bp->x-dl->ds->leftedgebottom.x) -
	 (dl->ds->leftedgetop.x-dl->ds->leftedgebottom.x)*(bp->y-dl->ds->leftedgebottom.y);
    d2 = (dl->ds->rightedgetop.y-dl->ds->rightedgebottom.y)*(bp->x-dl->ds->rightedgebottom.x) -
	 (dl->ds->rightedgetop.x-dl->ds->rightedgebottom.x)*(bp->y-dl->ds->rightedgebottom.y);
    if ( d1<0 ) d1 = -d1;
    if ( d2<0 ) d2 = -d2;

    if ( d1<d2 ) {
	e1 = dl->ds->pnum[0];
	e2 = dl->ds->pnum[1];
    } else {
	e1 = dl->ds->pnum[2];
	e2 = dl->ds->pnum[3];
    }
    m1 = ds->pnum[1-top];
    m2 = ds->pnum[3-top];

    data[0] = e2;
    data[1] = e1;
    data[2] = e1;
    data[3] = zerocvt;
    data[4] = m1;
    data[5] = zerocvt;
    data[6] = m2;
    isword = false;
    for ( i=0; i<7 ; ++i )
	if ( data[i]<0 || data[i]>255 ) {
	    isword = true;
    break;
	}
    pt = pushheader(pt,isword,7);
    for ( i=6; i>=0; --i )
	pt = addpoint(pt,isword,data[i]);

    *pt++ = 0x07;		/* SPVTL[orthog] */
    *pt++ = 0x0e;		/* SFVTP */
    *pt++ = 0x10;		/* SRP0 */
    *pt++ = 0xe0;		/* MIRP[00000] */
    *pt++ = 0xe0;		/* MIRP[00000] */
	/* If we set rnd then cvt cutin can screw us up */
	/* If we set min then... well we won't get a 0 distance */

    dl->done = true;

    touched[m1] |= 7;
    touched[m2] |= 7;
return( pt );
}

static void SetupFPGM(struct glyphinfo *gi) {
    int len;

    if ( gi->fpgmf!=NULL )
return;
    gi->fpgmf = tmpfile();

    /* Routine 0 takes 7 args:
	pt to move			(bottom of stack)
	cvt for stem 2
	bottom of edge2
	top point of edge2
	cvt for stem 1
	bottom point of edge1
	top point of edge1
       given two edges and a stem width for each edge, it moves four twilight
       points (two for each edge) to form parallel edges the stem width away
       from the originals. Then it intersects those two lines and moves the
       given point to that intersection. This is used for positioning the
       inner point of "V"
    */
    putc(0xb0,gi->fpgmf);		/* PUSHB[1] */
    putc(0x00,gi->fpgmf);		/*  0 */
    putc(0x2c,gi->fpgmf);		/* FDEF */

    putc(0x4e,gi->fpgmf);		/* FLIPOFF */

    putc(0x20,gi->fpgmf);		/* DUP */
    putc(0xb0,gi->fpgmf);		/* PUSHB[1] */
    putc(0x03,gi->fpgmf);		/*  3 */
    putc(0x25,gi->fpgmf);		/* CINDEX */
    putc(0x23,gi->fpgmf);		/* SWAP */
    putc(0x07,gi->fpgmf);		/* SPVTL[orthog] */
	/* Set the projection vector to be orthogonal to stem 1 */
	/*  (and leave stem 1 on the stack) */
    putc(0x10,gi->fpgmf);		/* SRP0 */
    putc(0xb0,gi->fpgmf);		/* PUSHB[1] */
    putc(0x02,gi->fpgmf);		/*  2 */
    putc(0x25,gi->fpgmf);		/* CINDEX */
	/* Get the cvt value for the stem width */
    putc(0xb1,gi->fpgmf);		/* PUSHB[2] */
    putc(0x01,gi->fpgmf);		/*  1 */	/* Twighlight point 1 */
    putc(0x00,gi->fpgmf);		/*  0 */	/* Zone pointer 0 (twilight zone) */
    putc(0x14,gi->fpgmf);		/* SZP1 */
    putc(0x23,gi->fpgmf);		/* SWAP */
    putc(0xe8,gi->fpgmf);		/* MIRP[01000] */ /* No round, min dist, grey */

    putc(0x10,gi->fpgmf);		/* SRP0 */
    putc(0xb0,gi->fpgmf);		/* PUSHB[1] */
    putc(0x02,gi->fpgmf);		/*  2 */	/* Twighlight point 2 */
    putc(0x23,gi->fpgmf);		/* SWAP */
    putc(0xe8,gi->fpgmf);		/* MIRP[01000] */ /* No round, min dist, grey */

    putc(0xb0,gi->fpgmf);		/* PUSHB[1] */
    putc(0x01,gi->fpgmf);		/*  1 */	/* Zone pointer 1 (normal zone) */
    putc(0x14,gi->fpgmf);		/* SZP1 */

    putc(0x20,gi->fpgmf);		/* DUP */
    putc(0xb0,gi->fpgmf);		/* PUSHB[1] */
    putc(0x03,gi->fpgmf);		/*  3 */
    putc(0x25,gi->fpgmf);		/* CINDEX */
    putc(0x23,gi->fpgmf);		/* SWAP */
    putc(0x07,gi->fpgmf);		/* SPVTL[orthog] */
	/* Set the projection vector to be orthogonal to stem 2 */
	/*  (and leave stem 1 on the stack) */
    putc(0x10,gi->fpgmf);		/* SRP0 */
    putc(0xb0,gi->fpgmf);		/* PUSHB[1] */
    putc(0x02,gi->fpgmf);		/*  2 */
    putc(0x25,gi->fpgmf);		/* CINDEX */
	/* Get the cvt value for the stem width */
    putc(0xb1,gi->fpgmf);		/* PUSHB[2] */
    putc(0x03,gi->fpgmf);		/*  3 */	/* Twighlight point 3 */
    putc(0x00,gi->fpgmf);		/*  0 */	/* Zone pointer 0 (twilight zone) */
    putc(0x14,gi->fpgmf);		/* SZP1 */
    putc(0x23,gi->fpgmf);		/* SWAP */
    putc(0xe8,gi->fpgmf);		/* MIRP[01000] */ /* No round, min dist, grey */

    putc(0x10,gi->fpgmf);		/* SRP0 */
    putc(0xb0,gi->fpgmf);		/* PUSHB[1] */
    putc(0x04,gi->fpgmf);		/*  4 */	/* Twighlight point 4 */
    putc(0x23,gi->fpgmf);		/* SWAP */
    putc(0xe8,gi->fpgmf);		/* MIRP[01000] */ /* No round, min dist, grey */

    putc(0xb4,gi->fpgmf);		/* PUSHB[5] */
    putc(0x01,gi->fpgmf);		/*  1 */	/* line1 */
    putc(0x02,gi->fpgmf);		/*  2 */
    putc(0x03,gi->fpgmf);		/*  3 */	/* line2 */
    putc(0x04,gi->fpgmf);		/*  4 */
    putc(0x00,gi->fpgmf);		/*  0 */	/* zone pointer */
    putc(0x13,gi->fpgmf);		/* SZP0 */
    putc(0x0f,gi->fpgmf);		/* ISECT */

	/* Untouch all four twilight points in both axes so we can use them */
	/*  again. (The SFVTPV sets the freedom vector to a diagonal so the */
	/*  untouch in in both x and y ) */
    putc(0x0e,gi->fpgmf);		/* SFVTPV */
    putc(0xb3,gi->fpgmf);		/* PUSHB[4] */
    putc(0x01,gi->fpgmf);		/*  1 */	/* line1 */
    putc(0x02,gi->fpgmf);		/*  2 */
    putc(0x03,gi->fpgmf);		/*  3 */	/* line2 */
    putc(0x04,gi->fpgmf);		/*  4 */
    putc(0x29,gi->fpgmf);		/* UTP */
    putc(0x29,gi->fpgmf);		/* UTP */
    putc(0x29,gi->fpgmf);		/* UTP */
    putc(0x29,gi->fpgmf);		/* UTP */

    putc(0xb0,gi->fpgmf);		/* PUSHB[1] */
    putc(0x01,gi->fpgmf);		/*  1 */	/* normal zone */
    putc(0x16,gi->fpgmf);		/* SZPS */
    putc(0x4d,gi->fpgmf);		/* FLIPON */
    putc(0x03,gi->fpgmf);		/* SPVTCA[x] */
    putc(0x2d,gi->fpgmf);		/* ENDF */

    gi->fpgmlen = len = ftell(gi->fpgmf);
    if ( len&1 ) {
	putc(0,gi->fpgmf);
	++len;
    }
    if ( len&2 ) {
	putc(0,gi->fpgmf);
	putc(0,gi->fpgmf);
    }

    gi->maxp->maxFDEFs = 1;
    gi->maxp->maxTwilightPts = 5;
}

/* If the dot product of the freedom and projection vectors is negative */
/*  then I am unable to understand truetype's behavior. So insure it is */
/*  positive by flipping the stem used to compute it */
static uint8 *fixProjectionDir(uint8 *pt,int **ppt,BasePoint *bp,int top,
	int bottom, real freedomx, real freedomy, int *olddir) {
    int dir;

    dir = ((bp[top].y-bp[bottom].y)*freedomx - (bp[top].x-bp[bottom].x)*freedomy>0);
    if ( dir==*olddir )
return(pt);
    *olddir = dir;
    if ( dir ) {
	int temp = bottom;
	bottom = top;
	top = temp;
    }
    *(*ppt)++ = bottom;
    *(*ppt)++ = top;
    *pt++ = 7;			/* SPVTL[orthog] */
return( pt );
}

static uint8 *fixProjectionPtDir(uint8 *pt,int **ppt,BasePoint *bp,int top,
	int bottom, int free1, int free2, int parallel, int *olddir) {
    if ( parallel ) {
return( fixProjectionDir(pt,ppt,bp,top,bottom,
	bp[free1].x-bp[free2].x,bp[free1].y-bp[free2].y,olddir));
    } else {
return( fixProjectionDir(pt,ppt,bp,top,bottom,
	bp[free1].y-bp[free2].y,bp[free2].x-bp[free1].x,olddir));
    }
}

/* This is a simple stem (like "/" or "N" or "X") where none of the end points*/
/*  are at intersections with other diagonal stems (there may be intersections*/
/*  in the middle of the stem, but we're only interested in the edges so that */
/*  is ok */
/* I've extended it to handle the case of the lower stem in "k" where the stem*/
/*  does end at an interesection, but that intersection is NOT the endpoint */
/*  of the other stem */
/* This routine has far too many cases. */
/*  If either the top or bottom (or both) ends in another diagonal stem (I */
/*   refer to this as a "kstem" because that's what the lower stem of k does) */
/*   then we fix the two points at top or bottom so that they are on the */
/*   other diagonal stem. */
/*  Then we fix one edge (we choose the edge which is already most touched) */
/*   we fix each point on that edge in both axes (assuming it hasn't already */
/*   been fixed in that axis) */
/*  The other edge is harder. */
/*   We pick one of the two points on that edge, we prefer a point which is on*/
/*    another edge (diagonal, horizontal, vertical) */
/*   If we find an edge we will move the current point parallel to that edge */
/*    until it is stemwidth from the other side of the diagonal we are fixing */
/*   If we don't find an edge then move it perpendicular to our own diagonal */
/*   Then do the same for the other point, except that if we don't have an */
/*    edge then move the other point so that it is zero distance from the point*/
/*    we fixed above */
static uint8 *DStemFix(DStem *ds,struct glyphinfo *gi,uint8 *pt,uint8 *touched,
	BasePoint *bp, int ptcnt) {
    int topvert, bottomvert;
    int topclosed, bottomclosed;
    int stemwidth = DStemWidth(ds);
    int stemcvt = getcvtval(gi,stemwidth);
    int dx, dy, tdiff;
    int movement_axis[4];
    int freedom;
    int t1,t2, b1,b2, topfixed, rp0= -1, isword;
    int pushes[40], *ppt = pushes;
    uint8 tempinstrs[40], *ipt = tempinstrs;
    int i;
    int kstemtop, kstembottom;
    int move, based;
    int examined;
    int projection_dir;

    if ( (dx = ds->rightedgetop.x - ds->leftedgetop.x )<0 ) dx = -dx;
    if ( (dy = ds->rightedgetop.y - ds->leftedgetop.y )<0 ) dy = -dy;
    topvert = dy>dx;
    tdiff = dy+dx;
    if ( (dx = ds->rightedgebottom.x - ds->leftedgebottom.x )<0 ) dx = -dx;
    if ( (dy = ds->rightedgebottom.y - ds->leftedgebottom.y )<0 ) dy = -dy;
    bottomvert = dy>dx;
    topfixed = ( tdiff < dy+dx );

    /* A stem which is closed (like that of / or x) where the two bottom(top) */
    /*  end points are connected is much easier to deal with than an open one */
    /*  like N where there is no connection */
    topclosed = (ds->pnum[0]==ds->pnum[2]+1) || (ds->pnum[0]==ds->pnum[2]-1);
    bottomclosed = (ds->pnum[1]==ds->pnum[3]+1) || (ds->pnum[1]==ds->pnum[3]-1);

    movement_axis[0] = movement_axis[2] = topvert;
    movement_axis[1] = movement_axis[3] = bottomvert;
    if ( !topclosed ) {
	movement_axis[0] = ExamineFreedom(ds->pnum[0],bp,ptcnt,topvert);
	movement_axis[2] = ExamineFreedom(ds->pnum[2],bp,ptcnt,topvert);
    }
    if ( !bottomclosed ) {
	movement_axis[1] = ExamineFreedom(ds->pnum[1],bp,ptcnt,bottomvert);
	movement_axis[3] = ExamineFreedom(ds->pnum[3],bp,ptcnt,bottomvert);
    }

    /* which of the two edges is most heavily positioned? */
    if ( (touched[ds->pnum[0]]&(1<<movement_axis[0])) + (touched[ds->pnum[1]]&(1<<movement_axis[1])) >=
	    (touched[ds->pnum[2]]&(1<<movement_axis[2])) + (touched[ds->pnum[3]]&(1<<movement_axis[3])) ) {
	t1 = 0; t2 = 2;
	b1 = 1; b2 = 3;
    } else {
	t1 = 2; t2 = 0;
	b1 = 3; b2 = 1;
    }

    /* freedom vector starts in x direction */
    freedom = 0;

    /* I build the push stack sort of backwards */

    /* First we fix whichever edge we found above (if it needs to be), then */
    /*  establish the projection vector, then fix the other edge */
    /* (We fix the one edge before we define the projection vector as rounding*/
    /*  the points may warp the edge meaning the projection won't be right... */
    kstemtop = (ds->top!=NULL && !IsVStemIntersection(ds->top));
    if ( kstemtop ) {
	pt = KStemMoveToEdge(gi,pt,ds->top,ds,1,touched);
	freedom = 2;
    }
    kstembottom = (ds->bottom!=NULL && !IsVStemIntersection(ds->bottom));
    if ( kstembottom ) {
	pt = KStemMoveToEdge(gi,pt,ds->bottom,ds,0,touched);
	freedom = 3;
    }
    for ( i=0; i<2; ++i ) {
	if ( (touched[ds->pnum[t1]]&(1<<i)) || kstemtop )
	    /* Don't worry about it, already fixed in this axis */;
	else {
	    if ( freedom!=i ) { *ipt++ = 1-i; freedom = i; }
	    if ( i!=movement_axis[t1] ) {
		*ppt++ = ds->pnum[t1];
		*ipt++ = 0x2e;		/* MDAP [nornd] */
	    } else if ( (touched[ds->pnum[t2]]&(1<<i)) ) {
		/* if this point has not been touched, but it's opposite point */
		/*  has been then reestablish the distance between them. */
		/*  Note that we are interested in whether the opposite point has */
		/*  been touched in the direction that T1 is free to move, not the*/
		/*  dir t2 is free in */
		*ppt++ = ds->pnum[t2];
		*ipt++ = 0x10;		/* SRP0 */
		*ppt++ = ds->pnum[t1];
		*ipt++ = 0xcd;		/* MDRP [rp0, min, rnd, black] */
	    } else {
		*ppt++ = ds->pnum[t1];
		*ipt++ = 0x2f;		/* MDAP[1round] */
	    }
	    rp0 = ds->pnum[t1];
	    touched[ds->pnum[t1]] |= (1<<i)|4;
	}
	if ( (touched[ds->pnum[b1]]&(1<<i)) || kstembottom )
	    /* Don't worry about it */;
	else {
	    if ( freedom!=i ) { *ipt++ = 1-i; freedom = i; }
	    if ( i!=movement_axis[b1] ) {
		*ppt++ = ds->pnum[b1];
		*ipt++ = 0x2e;		/* MDAP [nornd] */
	    } else if ( (touched[ds->pnum[b2]]&(1<<i)) ) {
		*ppt++ = ds->pnum[b2];
		*ipt++ = 0x10;		/* SRP0 */
		*ppt++ = ds->pnum[b1];
		*ipt++ = 0xdd;		/* MDRP [rp0, min, rnd, black] */
	    } else {
		*ppt++ = ds->pnum[b1];
		*ipt++ = 0x2f;		/* MDAP[1round] */
	    }
	    rp0 = ds->pnum[b1];
	    touched[ds->pnum[b1]] |= (1<<i)|4;
	}
    }

    projection_dir = -1;

    if ( kstemtop || ds->rightedgetop.x == ds->leftedgetop.x ||
		ds->rightedgetop.y == ds->leftedgetop.y ||
	    (topfixed && !(kstembottom || ds->rightedgebottom.x == ds->leftedgebottom.x ||
		ds->rightedgetop.y == ds->leftedgetop.y )) ) {
	examined = ExamineFreedom(ds->pnum[t2],bp,ptcnt,-1);
	if ( examined==-1 || topclosed ) {
	    if ( ds->rightedgetop.x == ds->leftedgetop.x )
		examined = 1;
	    else if ( ds->rightedgetop.y == ds->leftedgetop.y )
		examined = 0;
	}
	if ( kstemtop ) {
	    /* set the freedom vector along the edge of the other stem */
	    *ppt++ = ds->top->ds->pnum[0];
	    *ppt++ = ds->top->ds->pnum[1];
	    *ipt++ = 8;			/* SFVTL[parallel] */
	    freedom = 3;
	    ipt = fixProjectionPtDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    ds->top->ds->pnum[0],ds->top->ds->pnum[1],1,&projection_dir);
	} else if ( examined!=-1 ) {
	    if ( freedom!=examined ) { *ipt++ = 0x05-examined; freedom = examined; }
	    ipt = fixProjectionDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    !freedom,freedom,&projection_dir);
	} else {
	    ipt = fixProjectionPtDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    ds->pnum[t1],ds->pnum[b1],0,&projection_dir);
	    *ipt++ = 0x0e;		/* SFVTPV */
	    freedom = 4;
	}
	if ( freedom<2 ) movement_axis[t2] = freedom;
	move = ds->pnum[t2];
	based = ds->pnum[t1];
    } else {
	examined = ExamineFreedom(ds->pnum[b2],bp,ptcnt,-1);
	if ( examined==-1 || bottomclosed ) {
	    if ( ds->rightedgebottom.x == ds->leftedgebottom.x )
		examined = 1;
	    else if ( ds->rightedgebottom.y == ds->leftedgebottom.y )
		examined = 0;
	}
	if ( kstembottom ) {
	    /* set the freedom vector along the edge of the other stem */
	    *ppt++ = ds->bottom->ds->pnum[0];
	    *ppt++ = ds->bottom->ds->pnum[1];
	    *ipt++ = 8;			/* SFVTL[parallel] */
	    freedom = 3;
	    ipt = fixProjectionPtDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    ds->bottom->ds->pnum[0],ds->bottom->ds->pnum[1],1,
		    &projection_dir);
	} else if ( examined!=-1 ) {
	    if ( freedom!=examined ) { *ipt++ = 0x05-examined; freedom = examined; }
	    ipt = fixProjectionDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    !freedom,freedom,&projection_dir);
	} else {
	    ipt = fixProjectionPtDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    ds->pnum[t1],ds->pnum[b1],0,&projection_dir);
	    *ipt++ = 0x0e;		/* SFVTPV */
	    freedom = 4;
	}
	if ( freedom<2 ) movement_axis[b2] = freedom;
	move = ds->pnum[b2];
	based = ds->pnum[b1];
    }
    if ( rp0!=based ) {
	*ppt++ = based;
	*ipt++ = 16;		/* SRP0 */
    }
    *ppt++ = stemcvt;
    *ppt++ = move;
    *ipt++ = 0xed;		/* MIRP[01101] */
    if ( freedom==1 || freedom==0 )
	touched[move] |= (1<<freedom)|4;
    else
	touched[move] |= 7;
    if ( move==ds->pnum[t2] ) {
	move = ds->pnum[b2];
	based = ds->pnum[b1];
	examined = ExamineFreedom(ds->pnum[b2],bp,ptcnt,-1);
	if ( examined==-1 || bottomclosed ) {
	    if ( ds->rightedgebottom.x == ds->leftedgebottom.x )
		examined = 1;
	    else if ( ds->rightedgebottom.y == ds->leftedgebottom.y )
		examined = 0;
	}
	if ( kstembottom ) {
	    /* set the freedom vector along the edge of the other stem */
	    *ppt++ = ds->bottom->ds->pnum[0];
	    *ppt++ = ds->bottom->ds->pnum[1];
	    *ipt++ = 8;			/* SFVTL[parallel] */
	    freedom = 3;
	    ipt = fixProjectionPtDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    ds->bottom->ds->pnum[0],ds->bottom->ds->pnum[1],1,&projection_dir);
	} else if ( examined!=-1 ) {
	    if ( freedom!=examined ) { *ipt++ = 0x05-examined; freedom = examined; }
	    ipt = fixProjectionDir(ipt,&ppt,bp,ds->pnum[t1],ds->pnum[b1],
		    !freedom,freedom,&projection_dir);
	} else {
	    *ipt++ = 0x0e;		/* SFVTPV */
	    freedom = 4;
	    based = ds->pnum[t2];
	    stemcvt = getcvtval(gi,0);
	}
	if ( freedom<2 ) movement_axis[b2] = freedom;
    } else {
	/* If the top were a kstem, or if either of the coord were equal */
	/*  then we would not have taken this branch. The only choice if */
	/*  we get here is the final case above where the best we can do */
	/*  is make the point be on the stem */
	*ipt++ = 0x0e;			/* SFVTPV */
	freedom = 4;
	based = ds->pnum[b2];
	stemcvt = getcvtval(gi,0);
	move = ds->pnum[t2];
    }
    *ppt++ = based;
    *ipt++ = 16;		/* SRP0 */
    *ppt++ = stemcvt;
    *ppt++ = move;
    *ipt++ = stemcvt!=getcvtval(gi,0)?0xed:0xe1;	/* MIRP[0??01] */
    if ( freedom==1 || freedom==0 )
	touched[move] |= (1<<freedom)|4;
    else
	touched[move] |= 7;

    freedom = 5;	/* Actually the freedom vector might be ok, but the projection won't be and we are interested in both now */

    /* Make sure all points are touched in both directions */
    if ( touched[ds->pnum[t2]]&(1<<(1-movement_axis[t2])) ) {
	if ( freedom!=1-movement_axis[t2] ) { *ipt++ = movement_axis[t2]; freedom = 1-movement_axis[t2]; }
	*ppt++ = ds->pnum[t2];
	*ipt++ = 0x2e;		/* MDAP[noround] */
	touched[ds->pnum[t2]] |= (1<<(1-movement_axis[t2]));
    }
    if ( touched[ds->pnum[b2]]&(1<<(1-movement_axis[b2])) ) {
	if ( freedom!=1-movement_axis[b2] ) { *ipt++ = movement_axis[b2]; freedom = 1-movement_axis[b2]; }
	*ppt++ = ds->pnum[b2];
	*ipt++ = 0x2e;		/* MDAP[noround] */
	touched[ds->pnum[b2]] |= (1<<(1-movement_axis[b2]));
    }
	
    if ( freedom!=0 )
	*ipt++ = 1;			/* always leave both vectors in x */

    /* Ok, now figure out the instructions for the push stack */
    isword = false;
    for ( i=0; pushes+i < ppt ; ++i )
	if ( pushes[i]<0 || pushes[i]>255 ) {
	    isword = true;
    break;
	}
    pt = pushheader(pt,isword,ppt-pushes);
    while ( ppt>pushes )
	pt = addpoint(pt,isword,*--ppt);
    for ( i=0; tempinstrs+i<ipt; ++i )
	*pt++ = tempinstrs[i];

    ds->done = true;
return( pt );
}

static uint8 *DStemFixConnected(DStem *ds,int top_is_inter,struct glyphinfo *gi,
	uint8 *pt, uint8 *touched, BasePoint *bp, int ptcnt) {
    struct dstemlist *dl = top_is_inter ? ds->top : ds->bottom;
    DStem *nextds;
    int next_top_is_inter;
    int inner, outer1, outer2;
    int t1, t2, b1, b2;
    real sw1, sw2;
    int pushes[8];
    int isword, i;
    real dx, dy;

return( pt );
    /* This method doesn't work, I can't project backwards, or at least */
    /*  freetype won't let me. That may be a bug on their part, but even if */
    /*  it is I need to work around it */
    SetupFPGM(gi);

    pt = DStemFix(ds,gi,pt,touched,bp,ptcnt);
    dl = top_is_inter ? ds->top : ds->bottom;
    sw1 = DStemWidth(ds);
    do {
	nextds = dl->ds;
	sw2 = DStemWidth(nextds);
	if ( top_is_inter ) {
	    if ( ds->pnum[0]==nextds->pnum[2] || ds->pnum[2]==nextds->pnum[0] ) {
		next_top_is_inter = true;
		if ( ds->pnum[0]!=nextds->pnum[2] ) {
		    inner = ds->pnum[2];
		    outer1 = ds->pnum[0];
		    outer2 = nextds->pnum[2];
		    t1 = outer1; t2 = outer2; b1 = ds->pnum[1]; b2 = nextds->pnum[3];
		} else if ( ds->pnum[2]!=nextds->pnum[0] ) {
		    inner = ds->pnum[0];
		    outer1 = ds->pnum[2];
		    outer2 = nextds->pnum[0];
		    t1 = outer1; t2 = outer2; b1 = ds->pnum[3]; b2 = nextds->pnum[1];
		} else if ( ds->leftedgetop.y>ds->rightedgetop.y ) {
		    inner = ds->pnum[2];
		    outer1 = outer2 = ds->pnum[0];
		    t1 = t2 = outer1; b1 = ds->pnum[1]; b2 = nextds->pnum[3];
		} else {
		    inner = ds->pnum[0];
		    outer1 = outer2 = ds->pnum[2];
		    t1 = outer1; t2 = outer2; b1 = ds->pnum[1]; b2 = nextds->pnum[3];
		}
	    } else if ( ds->pnum[0]==nextds->pnum[1] || ds->pnum[2] == nextds->pnum[3] ) {
		next_top_is_inter = false;
		if ( ds->pnum[0]!=nextds->pnum[3] ) {
		    inner = ds->pnum[2];
		    outer1 = ds->pnum[0];
		    outer2 = nextds->pnum[3];
		    t1 = outer1; b2 = outer2; b1 = ds->pnum[1]; t2 = nextds->pnum[2];
		} else if ( ds->pnum[2]!=nextds->pnum[1] ) {
		    inner = ds->pnum[0];
		    outer1 = ds->pnum[2];
		    outer2 = nextds->pnum[1];
		    t1 = outer1; b2 = outer2; b1 = ds->pnum[3]; t2 = nextds->pnum[0];
		} else {
		    /* This case doesn't matter much, so don't work hard to get it right */
		    inner = ds->pnum[0];
		    outer1 = outer2 = ds->pnum[2];
		    t1 = outer1; b2 = outer2; b1 = ds->pnum[3]; t2 = nextds->pnum[0];
		}
	    } else {
		GDrawIError( "Failed to find intersection in DStemFixConnected" );
return( pt );
	    }
	    if ( inner == ds->pnum[0] ) sw1 = -sw1;
	    else sw2 = -sw2;
	} else {
	    if ( ds->pnum[1]==nextds->pnum[3] || ds->pnum[3]==nextds->pnum[1] ) {
		next_top_is_inter = false;
		if ( ds->pnum[1]!=nextds->pnum[3] ) {
		    inner = ds->pnum[3];
		    outer1 = ds->pnum[1];
		    outer2 = nextds->pnum[3];
		    b1 = outer1; b2 = outer2; t1 = ds->pnum[0]; t2 = nextds->pnum[2];
		} else if ( ds->pnum[3]!=nextds->pnum[1] ) {
		    inner = ds->pnum[1];
		    outer1 = ds->pnum[3];
		    outer2 = nextds->pnum[1];
		    b1 = outer1; b2 = outer2; t1 = ds->pnum[2]; t2 = nextds->pnum[0];
		} else if ( ds->leftedgetop.y>ds->rightedgetop.y ) {
		    inner = ds->pnum[3];
		    outer1 = outer2 = ds->pnum[1];
		    b1 = b2 = outer1; t1 = ds->pnum[0]; t2 = nextds->pnum[2];
		} else {
		    inner = ds->pnum[1];
		    outer1 = outer2 = ds->pnum[3];
		    b1 = b2 = outer1; t1 = ds->pnum[2]; t2 = nextds->pnum[0];
		}
	    } else if ( ds->pnum[1]==nextds->pnum[0] || ds->pnum[3] == nextds->pnum[2] ) {
		next_top_is_inter = false;
		if ( ds->pnum[1]!=nextds->pnum[2] ) {
		    inner = ds->pnum[3];
		    outer1 = ds->pnum[1];
		    outer2 = nextds->pnum[2];
		} else if ( ds->pnum[3]!=nextds->pnum[0] ) {
		    inner = ds->pnum[1];
		    outer1 = ds->pnum[3];
		    outer2 = nextds->pnum[0];
		} else {
		    /* This case doesn't matter much, so don't work hard to get it right */
		    inner = ds->pnum[1];
		    outer1 = outer2 = ds->pnum[3];
		}
	    } else {
		GDrawIError( "Failed to find intersection in DStemFixConnected" );
return( pt );
	    }
	    if ( inner == ds->pnum[1] ) sw1 = -sw1;
	    else sw2 = -sw2;
	}
	if ( outer1!=outer2 ) {
	    /* if the outer edges don't come to a point, then make sure */
	    /*  there's a minimum distance between them */
	    if ( (dx = bp[outer2].x - bp[outer1].x )<0 ) dx = -dx;
	    if ( (dy = bp[outer2].y - bp[outer1].y )<0 ) dy = -dy;
	    /* outer1 should have been fixed already */
	    pt = pushpoint(pt,outer1);
	    *pt++ = 0x10;		/* SRP0 */
	    if ( dx>dy ) {
		pt = pushpointstem(pt,outer2,getcvtval(gi,bp[outer1].x-bp[outer2].x));
		*pt++ = 0xe0+0x0d;	/* MIRP, minimum, rounded, black */
		touched[outer2] |= 1|4;
	    } else {
		*pt++ = 0x00;		/* SVTCA[y] */
		pt = pushpointstem(pt,outer2,getcvtval(gi,bp[outer1].y-bp[outer2].y));
		*pt++ = 0xe0+0x0d;	/* MIRP, minimum, rounded, black */
		*pt++ = 0x01;		/* SVTCA[x] */
		touched[outer2] |= 2|4;
	    }
	}
	pt = DStemFix(nextds,gi,pt,touched,bp,ptcnt);

    /* I push the bottom then the top because that way the projection vector */
    /*  dot the freedom vector is positive. It it is negative the bytecode */
    /*  interpretters produce strange results */
	/* Do Intersect */
	pushes[0] = inner;
	pushes[1] = _getcvtval(gi,-sw1);
	pushes[3] = b1;
	pushes[2] = t1;
	pushes[4] = _getcvtval(gi,-sw2);
	pushes[6] = b2;
	pushes[5] = t2;
	pushes[7] = 0;
	isword = false;
	for ( i=0; i<8 ; ++i )
	    if ( pushes[i]<0 || pushes[i]>255 ) {
		isword = true;
	break;
	    }
	pt = pushheader(pt,isword,8);
	for ( i=0; i<8 ; ++i )
	    pt = addpoint(pt,isword,pushes[i]);
	*pt++ = 0x2b;			/* CALL (Function 0) */

	dl->done = true;

	ds = nextds;
	sw1 = sw2;
	if ( sw1<0 ) sw1 = -sw1;
	top_is_inter = !next_top_is_inter;
	dl = top_is_inter ? ds->top : ds->bottom;
    } while ( dl!=NULL && IsVStemIntersection(dl));

return( pt );
}

static uint8 *DStemCheckKStems(DStem *ds,struct glyphinfo *gi,
	uint8 *pt, uint8 *touched, BasePoint *bp, int ptcnt) {
    /* Find all dstems like the lower stem of "k" where the other stem */
    /*  has already been set */
    DStem *ds2;
    int any;

    do {
	any = false;
	for ( ds2 = ds; ds2!=NULL; ds2 = ds2->next ) if ( !ds2->done ) {
	    if ( (ds2->top==NULL || (ds2->top->done && !IsVStemIntersection(ds2->top))) &&
		    (ds2->bottom==NULL || (ds2->bottom->done && !IsVStemIntersection(ds2->bottom))) ) {
		pt = DStemFix(ds2,gi,pt,touched,bp,ptcnt);
		any = true;
	    }
	}
    } while ( any );
return( pt );
}

/* EndPoints have several different forms:
    /	easy, no complications
    x	easy, no (end point) complications
    N	mild difficulties in that some points are free to move in x and others
	in y (the points in the armpits of the stems move in y, the points
	outside move in x)
    V	have to go through machinations to figure out the bottom end point
    W	machinations for all the middle end points
    M
    k	Special case for the vertical end point
	Special case for diagonal end point
    25CA (diamond character) similar to V except all points are intersections
To establish a simple end point we pick one side of the stem and MDAP it (both top and bottom)
    then we set the projection vector orthogonal to the stem
    Usually we set the freedom vector to x (but for vertical k set to y)
    And move the other end point the desired stem width from the picked one

So...
    Find all endpoints with no intersections (simple end points) and establish them
    Find all intersection (V like) end points
	Try to find one where at least one of the sides is fixed above
		(if there are none then just pick one and fix it arbetrarily)
	Does the V come to a point? (or is its base flattened)
	    MDAP the pointy end
	else
	    MDAP one of the ends
	    Make the other end an minimum distance from it
	Call our intersection routine to figure where the intersection of the
		other sides goes
    Find all diagonal (k like) end points
	Make sure that the stem which does not end here has its endpoints fixed
		(the upper stem of "k")
	ISECT that stem with one of the edges of the other stem (lower k) and
		fix one point there.
	Set the projection vector orthog to the other stem (lower k) and move
		the end points of its untouched edge to be the stem width away
		from the fixed edge
	ISECT that edge with the non-ending stem (upper k)
*/
static uint8 *DStemEstablishEndPoints(struct glyphinfo *gi,uint8 *pt,DStem *ds,
	uint8 *touched, BasePoint *bp, int ptcnt) {
    DStem *ds2;

    for ( ds2 = ds; ds2!=NULL; ds2 = ds2->next ) {
	ds2->top = DStemTopAtIntersection(ds2);
	ds2->bottom = DStemBottomAtIntersection(ds2);
    }

    /* Find all dstems where neither end is an intersection and position them */
    /*  handles things like "/", "x", and the upper stem of "k" */
    for ( ds2 = ds; ds2!=NULL; ds2 = ds2->next ) {
	if ( ds2->top==NULL && ds2->bottom==NULL )
	    pt = DStemFix(ds2,gi,pt,touched,bp,ptcnt);
    }

    /* Find all dstems like the lower stem of "k" where the other stem */
    /*  has already been set */
    pt = DStemCheckKStems(ds,gi,pt,touched,bp,ptcnt);

    /* Find all dstems where one end is not an intersection and position them */
    /*  and position anything connected to them */
    /*  handles things like "V", "W" */
    for ( ds2 = ds; ds2!=NULL; ds2 = ds2->next ) if ( !ds2->done ) {
	if ( ds2->top!=NULL && IsVStemIntersection(ds2->top) &&
		(ds2->bottom==NULL || (ds2->bottom->done && !IsVStemIntersection(ds2->bottom))))
	    pt = DStemFixConnected(ds2,1,gi,pt,touched,bp,ptcnt);
	else if ( ds2->bottom!=NULL && IsVStemIntersection(ds2->bottom) &&
		(ds2->top==NULL || (ds2->top->done && !IsVStemIntersection(ds2->top))))
	    pt = DStemFixConnected(ds2,0,gi,pt,touched,bp,ptcnt);
    }

    pt = DStemCheckKStems(ds,gi,pt,touched,bp,ptcnt);

    /* Find all remaining dstems and arbetarily fix something and then */
    /*  position anything connected */
    /*  handles things like diamond */
    for ( ds2 = ds; ds2!=NULL; ds2 = ds2->next ) if ( !ds2->done ) {
	if ( ds2->top && IsVStemIntersection(ds2->top))
	    pt = DStemFixConnected(ds2,1,gi,pt,touched,bp,ptcnt);
	else if ( ds2->bottom && IsVStemIntersection(ds2->bottom))
	    pt = DStemFixConnected(ds2,0,gi,pt,touched,bp,ptcnt);
	else
	    pt = DStemFix(ds2,gi,pt,touched,bp,ptcnt);
    }
return( pt );
}

static uint8 *DStemISectStem(uint8 *pt,DStem *ds,struct dstemlist *dl,
	uint8 *touched) {
    int pushes[32], *ppt = pushes;
    int cnt, isword, i;
    struct dstemlist *dl2;


    for ( dl2 = dl->ds->intersects; dl2!=NULL && dl2->ds!=ds; dl2=dl2->next );
    if ( dl2->done )
return( pt );
    if ( dl2!=NULL ) dl2->done = true;

    /* I have to push things backwards */
    for ( i=cnt=0; i<4; ++i ) if ( dl->is[i]!=NULL && dl->pnum[i]!=-1 ) {
	if ( i&1 ) {
	    *ppt++ = ds->pnum[2];
	    *ppt++ = ds->pnum[3];
	} else {
	    *ppt++ = ds->pnum[0];
	    *ppt++ = ds->pnum[1];
	}
	if ( i&2 ) {
	    *ppt++ = dl->ds->pnum[2];
	    *ppt++ = dl->ds->pnum[3];
	} else {
	    *ppt++ = dl->ds->pnum[0];
	    *ppt++ = dl->ds->pnum[1];
	}
	touched[dl->pnum[i]] |= 3|4;
	*ppt++ = dl->pnum[i];
	++cnt;
    }

    /* Ok, now figure out the instructions for the push stack */
    isword = false;
    for ( i=0; pushes+i < ppt ; ++i )
	if ( pushes[i]<0 || pushes[i]>255 ) {
	    isword = true;
    break;
	}
    pt = pushheader(pt,isword,ppt-pushes);
    while ( ppt>pushes )
	pt = addpoint(pt,isword,*--ppt);

    for ( i=0; i<cnt; ++i )
	*pt++ = 0x0f;		/* ISECT */
return( pt );
}

static uint8 *DStemISectMidPoints(uint8 *pt,DStem *ds,uint8 *touched) {
    struct dstemlist *dl;

    while ( ds!=NULL ) {
	for ( dl=ds->intersects; dl!=NULL; dl=dl->next ) if ( !dl->done )
	    pt = DStemISectStem(pt,ds,dl,touched);
	ds = ds->next;
    }
return( pt );
}

static uint8 *SetPFVectorsToSlope(uint8 *pt,int e1, int e2) {
    pt = pushpointstem(pt,e1,e2);
    *pt++ = 0x07;		/* SPVTL[orthog] */
    *pt++ = 0x0e;		/* SFVTP */
return( pt );
}

/* There may be other points on the diagonal stems which aren't at intersections */
/*  they still need to be moved so that they lie on the (new position of the) */
/*  stem */
static uint8 *DStemMakeColinear(struct glyphinfo *gi,uint8 *pt,DStem *ds,
	uint8 *touched, BasePoint *bp, int ptcnt) {
    int i, established, positioned, any=0;
    int zerocvt = getcvtval(gi,0);

    while ( ds!=NULL ) {
	/* touched[]&4 indicates a diagonal touch, only interested in points */
	/*  which have not been diagonally touched */
	established = positioned = 0;
	for ( i=0; i<ptcnt; ++i ) if ( !(touched[i]&4) && ds->used[i]==1 &&
		i!=ds->pnum[0] && i!=ds->pnum[1]) {
	    if ( !established ) {
		pt = SetPFVectorsToSlope(pt,ds->pnum[0],ds->pnum[1]);
		pt = pushpoint(pt,ds->pnum[0]);
		*pt++ = 0x10;		/* SRP0 */
		established = any = true;
	    }
	    pt = pushpointstem(pt,i,zerocvt);
	    *pt++ = 0xe0;			/* MIRP[00000] */
	    touched[i] |= 4;
	}
	for ( i=0; i<ptcnt; ++i ) if ( !(touched[i]&4) && ds->used[i]==2 &&
		i!=ds->pnum[2] && i!=ds->pnum[3]) {
	    if ( !established ) {
		pt = SetPFVectorsToSlope(pt,ds->pnum[0],ds->pnum[1]);
		established = any = true;
	    }
	    if ( !positioned ) {
		pt = pushpoint(pt,ds->pnum[2]);
		*pt++ = 0x10;		/* SRP0 */
		positioned = true;
	    }
	    pt = pushpointstem(pt,i,zerocvt);
	    *pt++ = 0xe0;			/* MIRP[00000] */
	    touched[i] |= 4;
	}
	ds = ds->next;
    }
    if ( any )
	*pt++ = 0x01;				/* SVTCA[x] */
return( pt );
}

static uint8 *DStemInfoGeninst(struct glyphinfo *gi,uint8 *pt,DStemInfo *d,
	uint8 *touched, BasePoint *bp, int ptcnt) {
    DStem *ds;

#if TEST_DIAGHINTS
    if ( ds==NULL )		/* Comment out this line to turn off diagonal hinting !!!*/
#endif
return( pt );
    ds = DStemMerge(d,bp,ptcnt,touched);

    *pt++ = 0x30;	/* Interpolate Untouched Points y */
    *pt++ = 0x31;	/* Interpolate Untouched Points x */

    DStemIntersect(ds,bp,ptcnt);
    pt = DStemEstablishEndPoints(gi,pt,ds,touched,bp,ptcnt);
    pt = DStemISectMidPoints(pt,ds,touched);
    pt = DStemMakeColinear(gi,pt,ds,touched,bp,ptcnt);
    DStemFree(ds);
return( pt );
}

static int MapSP2Index(SplineSet *ttfss,SplinePoint *csp, int ptcnt) {
    SplineSet *ss;
    SplinePoint *sp;

    if ( csp==NULL )
return( ptcnt+1 );		/* ptcnt+1 is the phantom point for the width */
    for ( ss=ttfss; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first;; ) {
	    if ( sp->me.x==csp->me.x && sp->me.y==csp->me.y )
return( sp->ptindex );
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
return( -1 );
}

/* Order of the md hints is important hence the double loop. */
/* We only do a hint if one edge has been fixed (or if we have no choice) */
static uint8 *gen_md_instrs(struct glyphinfo *gi, uint8 *instrs,MinimumDistance *_md,
	SplineSet *ttfss, BasePoint *bp, int ptcnt, int xdir, uint8 *touched) {
    int mask = xdir ? 1 : 2;
    int pt1, pt2;
    int any, graspatstraws=false, undone;
    MinimumDistance *md;

    do {
	any = false; undone = false;
	for ( md=_md; md!=NULL; md=md->next ) {
	    if ( md->x==xdir && !md->done ) {
		pt1 = MapSP2Index(ttfss,md->sp1,ptcnt);
		pt2 = MapSP2Index(ttfss,md->sp2,ptcnt);
		if ( pt1==ptcnt+1 ) {
		    pt1 = pt2;
		    pt2 = ptcnt+1;
		}
		if ( pt1==-1 || pt2==-1 )
		    fprintf(stderr, "Internal Error: Failed to find point in minimum distance check\n" );
		else if ( pt1!=ptcnt+1 && (touched[pt1]&mask) &&
			pt2!=ptcnt+1 && (touched[pt2]&mask) )
		    md->done = true;	/* somebody else did it, might not be right for us, but... */
		else if ( !graspatstraws &&
			!(touched[pt1]&mask) &&
			 (pt2==ptcnt+1 || !(touched[pt2]&mask)) )
		     /* If neither edge has been touched, then don't process */
		     /*  it now. hope that by filling in some other mds we will*/
		     /*  establish one edge or the other, and then come back to*/
		     /*  it */
		    undone = true;
		else if ( pt2==ptcnt+1 || !(touched[pt2]&mask)) {
		    md->done = true;
		    instrs = pushpointstem(instrs,pt2,pt1);	/* Misnomer, I'm pushing two points */
		    if ( !(touched[pt1]&mask))
			*instrs++ = 0x2f;			/* MDAP[rnd] */
		    else
			*instrs++ = 0x10;			/* SRP0 */
		    *instrs++ = 0xcc;			/* MDRP[01100] min, rnd, grey */
		    touched[pt1] |= mask;
		    if ( pt2!=ptcnt+1 )
			touched[pt2]|= mask;
		    any = true;
		} else {
		    md->done = true;
		    instrs = pushpointstem(instrs,pt1,pt2);	/* Misnomer, I'm pushing two points */
		    *instrs++ = 0x10;			/* SRP0 */
		    *instrs++ = 0xcc;			/* MDRP[01100] min, rnd, grey */
		    touched[pt1] |= mask;
		    any = true;
		}
	    }
	}
	graspatstraws = undone && !any;
    } while ( undone );
return(instrs);
}

static uint8 *gen_rnd_instrs(struct glyphinfo *gi, uint8 *instrs,SplineSet *ttfss,
	BasePoint *bp, int ptcnt, int xdir, uint8 *touched) {
    int mask = xdir ? 1 : 2;
    SplineSet *ss;
    SplinePoint *sp;

    for ( ss=ttfss; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ((( sp->roundx && xdir ) || ( sp->roundy && !xdir )) &&
		    !(touched[sp->ptindex]&mask)) {
		instrs = pushpoint(instrs,sp->ptindex);
		*instrs++ = 0x2f;		/* MDAP[rnd] */
		touched[sp->ptindex] |= mask;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp == ss->first )
	break;
	}
    }
return( instrs );
}

static uint8 *gen_extremum_instrs(struct glyphinfo *gi, uint8 *instrs,
	BasePoint *bp, int ptcnt, uint8 *touched) {
    int i;
    real min, max;

    max = (int) 0x80000000; min = 0x7fffffff;
    for ( i=0; i<ptcnt; ++i ) {
	if ( min>bp[i].y ) min = bp[i].y;
	else if ( max<bp[i].y ) max = bp[i].y;
    }
    for ( i=0; i<ptcnt; ++i ) if ( !(touched[i]&2) && (bp[i].y==min || bp[i].y==max) ) {
	instrs = pushpoint(instrs,i);
	*instrs++ = 0x2f;			/* MDAP[rnd] */
	touched[i] |= 0x2;
    }
return( instrs );
}

static void initforinstrs(SplineChar *sc) {
    SplineSet *ss;
    SplinePoint *sp;
    MinimumDistance *md;

    for ( ss=sc->splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    sp->dontinterpolate = false;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp == ss->first )
	break;
	}
    }

    for ( md=sc->md; md!=NULL; md=md->next ) {
	if ( md->sp1!=NULL ) md->sp1->dontinterpolate = true;
	if ( md->sp2!=NULL ) md->sp2->dontinterpolate = true;
    }
}

static void dumpgeninstructions(SplineChar *sc, struct glyphinfo *gi,
	int *contourends, BasePoint *bp, int ptcnt, SplineSet *ttfss,
	uint8 *touched) {
    StemInfo *hint;
    uint8 *instrs, *pt;
    int max;
    DStemInfo *dstem;

    if ( sc->vstem==NULL && sc->hstem==NULL && sc->dstem==NULL && sc->md==NULL ) {
	putshort(gi->glyphs,0);		/* no instructions */
return;
    }
    /* Maximum instruction length is 6 bytes for each point in each dimension */
    /*  2 extra bytes to finish up. And one byte to switch from x to y axis */
    /* Diagonal take more space because we need to set the orientation on */
    /*  each stem, and worry about intersections, etc. */
    /*  That should be an over-estimate */
    max=2;
    if ( sc->vstem!=NULL ) max += 6*ptcnt;
    if ( sc->hstem!=NULL ) max += 6*ptcnt+1;
    for ( dstem=sc->dstem; dstem!=NULL; max+=7+4*6+100, dstem=dstem->next );
    if ( sc->md!=NULL ) max += 12*ptcnt;
    max += 6*ptcnt;			/* in case there are any rounds */
    max += 6*ptcnt;			/* paranoia */
    instrs = pt = galloc(max);

    for ( hint=sc->vstem; hint!=NULL; hint=hint->next )
	hint->enddone = hint->startdone = false;
    for ( hint=sc->hstem; hint!=NULL; hint=hint->next )
	hint->enddone = hint->startdone = false;

    /* first instruct horizontal stems (=> movement in y) */
    /*  do this first so that the diagonal hinter will have everything moved */
    /*  properly when it sets the projection vector */
    if ( sc->hstem!=NULL ) {
	*pt++ = 0x00;	/* Set Vectors to y */
	for ( hint=sc->hstem; hint!=NULL; hint=hint->next ) {
	    if ( !hint->startdone || !hint->enddone )
		pt = geninstrs(gi,pt,hint,contourends,bp,ptcnt,sc->hstem,false,touched);
	}
    }
    pt = gen_md_instrs(gi,pt,sc->md,ttfss,bp,ptcnt,false,touched);
    pt = gen_rnd_instrs(gi,pt,ttfss,bp,ptcnt,false,touched);
    pt = gen_extremum_instrs(gi,pt,bp,ptcnt,touched);

    /* next instruct vertical stems (=> movement in x) */
    if ( pt != instrs )
	*pt++ = 0x01;	/* Set Vectors to x */
    for ( hint=sc->vstem; hint!=NULL; hint=hint->next ) {
	if ( !hint->startdone || !hint->enddone )
	    pt = geninstrs(gi,pt,hint,contourends,bp,ptcnt,sc->vstem,true,touched);
    }

    /* finally instruct diagonal stems (=> movement in x) */
    pt = DStemInfoGeninst(gi,pt,sc->dstem,touched,bp,ptcnt);

    pt = gen_md_instrs(gi,pt,sc->md,ttfss,bp,ptcnt,true,touched);
    pt = gen_rnd_instrs(gi,pt,ttfss,bp,ptcnt,true,touched);

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

#if 0
/* The following is undocumented, but vaguely implied: */
/* Composit characters have their components grid fitted BEFORE they go into */
/*  the final character. (That's why round to grid is important). So if we try*/
/*  to generated hints for them we'll just duplicate earlier work and move */
/*  stuff where it should not be */
static void dumpcompositinstrs(SplineChar *sc, struct glyphinfo *gi,RefChar *refs) {
    RefChar *ref;
    SplineSet *ss, *head=NULL, *last, *ttfss;
    int contourcnt=0, ptcnt=0;
    int *contourends;
    BasePoint *bp;

    /* We can't just convert the refs->splines fields because they may have */
    /*  been transformed and may end up with a different number of points than*/
    /*  the original (which would mean our instructions wouldn't match) */
    /* Instead we must approximate the original and then transform it */

    for ( ref=refs; ref!=NULL; ref=ref->next ) {
	ttfss = SplinePointListTransform(SCttfApprox(ref->sc),ref->transform,true);
	if ( head==NULL ) head = ttfss;
	else last->next = ttfss;
	for ( ss=ttfss; ss!=NULL; ss=ss->next ) {
	    ++contourcnt;
	    ptcnt += SSPointCnt(ss);
	    last = ss;
	}
    }

    contourends = galloc((contourcnt+1)*sizeof(int));
    bp = galloc(ptcnt*sizeof(BasePoint));
    contourcnt = ptcnt = 0;
    for ( ss=head; ss!=NULL; ss=ss->next ) {
	ptcnt = SSAddPoints(ss,ptcnt,bp,NULL);
	contourends[contourcnt++] = ptcnt-1;
    }
    contourends[contourcnt] = 0;
    SplinePointListFree(head);
    
    dumpgeninstructions(sc,gi,contourends,bp,ptcnt);
    free(contourends);
    free(bp);
}
#endif

static void dumpcomposit(SplineChar *sc, RefChar *refs, struct glyphinfo *gi) {
    struct glyphhead gh;
    DBounds bb;
    int i, ptcnt, ctcnt, flags;
    SplineSet *ss;
    RefChar *ref;

    if ( sc->changedsincelasthinted && !sc->manualhints )
	SplineCharAutoHint(sc,true);

    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);

    SplineCharFindBounds(sc,&bb);
    gh.numContours = -1;
    gh.xmin = floor(bb.minx); gh.ymin = floor(bb.miny);
    gh.xmax = ceil(bb.maxx); gh.ymax = ceil(bb.maxy);
    dumpghstruct(gi,&gh);

    i=ptcnt=ctcnt=0;
    for ( ref=refs; ref!=NULL; ref=ref->next, ++i ) {
	flags = (1<<1)|(1<<2)|(1<<12);	/* Args are always values for me */
					/* Always round args to grid */
	    /* There is some very strange stuff wrongly-documented on the apple*/
	    /*  site about how these should be interpretted when there are */
	    /*  scale factors, or rotations */
	    /* That description does not match the behavior of their rasterizer*/
	    /*  I've reverse engineered something else (see parsettf.c) */
	    /*  http://fonts.apple.com/TTRefMan/RM06/Chap6glyf.html */
	    /* Adobe says that setting bit 12 means that this will not happen */
	    /*  Apple doesn't mention bit 12 though...(but they do support it) */
/* if we want a mac style scaled composite then
	flags = (1<<1)|(1<<2)|(1<<11);
    and if we want an ambiguous composite...
	flags = (1<<1)|(1<<2);
*/
	if ( ref->next!=NULL )
	    flags |= (1<<5);		/* More components */
#if 0
	else if ( sc->hstem || sc->vstem || sc->dstem )	/* Composits inherit instructions */
	    flags |= (1<<8);		/* Instructions appear after last ref */
#endif
	if ( ref->transform[1]!=0 || ref->transform[2]!=0 )
	    flags |= (1<<7);		/* Need a full matrix */
	else if ( ref->transform[0]!=ref->transform[3] )
	    flags |= (1<<6);		/* different xy scales */
	else if ( ref->transform[0]!=1. )
	    flags |= (1<<3);		/* xy scale is same */
	if ( ref->transform[4]<-128 || ref->transform[4]>127 ||
		ref->transform[5]<-128 || ref->transform[5]>127 )
	    flags |= (1<<0);
	putshort(gi->glyphs,flags);
	putshort(gi->glyphs,ref->sc->ttf_glyph);
	if ( flags&(1<<0) ) {
	    putshort(gi->glyphs,(short)ref->transform[4]);
	    putshort(gi->glyphs,(short)ref->transform[5]);
	} else {
	    putc((char) (ref->transform[4]),gi->glyphs);
	    putc((char) (ref->transform[5]),gi->glyphs);
	}
	if ( flags&(1<<7) ) {
	    put2d14(gi->glyphs,ref->transform[0]);
	    put2d14(gi->glyphs,ref->transform[1]);
	    put2d14(gi->glyphs,ref->transform[2]);
	    put2d14(gi->glyphs,ref->transform[3]);
	} else if ( flags&(1<<6) ) {
	    put2d14(gi->glyphs,ref->transform[0]);
	    put2d14(gi->glyphs,ref->transform[3]);
	} else if ( flags&(1<<3) ) {
	    put2d14(gi->glyphs,ref->transform[0]);
	}
	for ( ss=ref->splines; ss!=NULL ; ss=ss->next ) {
	    ++ctcnt;
	    ptcnt += SSPointCnt(ss);
	}
    }

#if 0	/* composits inherit the instructions of their references */
	/* we'd only want instructions if we scaled/rotated a ref */
	/* (or if we wanted a seperation between two refs) */
    if ( sc->hstem || sc->vstem || sc->dstem )
	dumpcompositinstrs(sc,gi,refs);
#endif

    if ( gi->maxp->maxnumcomponents<i ) gi->maxp->maxnumcomponents = i;
	/* Assume every font has at least one reference character */
	/* PfaEdit will do a transitive closeur so that we end up with */
	/*  a maximum depth of 1 (according to apple) or 2 (according to Opentype) */
    gi->maxp->maxcomponentdepth = /* Apple docs say: 1, Open type docs say: */ 2;
    if ( gi->maxp->maxCompositPts<ptcnt ) gi->maxp->maxCompositPts=ptcnt;
    if ( gi->maxp->maxCompositCtrs<ctcnt ) gi->maxp->maxCompositCtrs=ctcnt;

    ttfdumpmetrics(sc,gi,&bb);
    if ( ftell(gi->glyphs)&1 )		/* Pad the file so that the next glyph */
	putc('\0',gi->glyphs);		/* on a word boundary, can only happen if odd number of instrs */
    RefCharsFreeRef(refs);
}

static void dumpglyph(SplineChar *sc, struct glyphinfo *gi) {
    struct glyphhead gh;
    DBounds bb;
    SplineSet *ss, *ttfss;
    int contourcnt, ptcnt;
    int *contourends;
    BasePoint *bp;
    char *fs;
    uint8 *touched;

/* I haven't seen this documented, but ttf rasterizers are unhappy with a */
/*  glyph that consists of a single point. Glyphs containing two single points*/
/*  are ok, glyphs with a single point and anything else are ok, glyphs with */
/*  a line are ok. But a single point is not ok. Dunno why */
    if (( sc->splines==NULL && sc->refs==NULL ) ||
	    ( sc->refs==NULL &&
	     (sc->splines->first->next==NULL ||
	      sc->splines->first->next->to == sc->splines->first)) ) {
	dumpspace(sc,gi);
return;
    }

    gi->loca[gi->next_glyph++] = ftell(gi->glyphs);

    if ( sc->changedsincelasthinted && !sc->manualhints )
	SplineCharAutoHint(sc,true);
    initforinstrs(sc);

    contourcnt = ptcnt = 0;
    ttfss = SCttfApprox(sc);
    for ( ss=ttfss; ss!=NULL; ss=ss->next ) {
	++contourcnt;
	ptcnt += SSPointCnt(ss);
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
    touched = gcalloc(ptcnt,1);
    ptcnt = contourcnt = 0;
    for ( ss=ttfss; ss!=NULL; ss=ss->next ) {
	touched[ptcnt] |= tf_startcontour;
	ptcnt = SSAddPoints(ss,ptcnt,bp,fs);
	putshort(gi->glyphs,ptcnt-1);
	touched[ptcnt-1] |= tf_endcontour;
	contourends[contourcnt++] = ptcnt-1;
    }

    contourends[contourcnt] = 0;
    dumpgeninstructions(sc,gi,contourends,bp,ptcnt,ttfss,touched);
    dumppointarrays(gi,bp,fs,ptcnt);
    SplinePointListFree(ttfss);
    free(bp);
    free(contourends);
    free(fs);

    ttfdumpmetrics(sc,gi,&bb);
}

static void dumpglyphs(SplineFont *sf,struct glyphinfo *gi) {
    int i, cnt;
    RefChar *refs;
    int fixed = SFOneWidth(sf);

    GProgressChangeStages(2+gi->strikecnt);
    QuickBlues(sf,&gi->bd);
    /*FindBlues(sf,gi->blues,NULL);*/
    GProgressNextStage();
    gi->fudge = (sf->ascent+sf->descent)/500;	/* fudge factor for hint matches */

    i=0, cnt=0;
    if ( SCIsNotdef(sf->chars[0],fixed) )
	sf->chars[i++]->ttf_glyph = cnt++;
    for ( cnt=3; i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]==SCDuplicate(sf->chars[i]))
	    sf->chars[i]->ttf_glyph = cnt++;

    gi->maxp->numGlyphs = cnt;
    gi->loca = galloc((gi->maxp->numGlyphs+1)*sizeof(uint32));
    gi->next_glyph = 0;
    gi->glyphs = tmpfile();
    gi->hmtx = tmpfile();
    if ( sf->hasvmetrics )
	gi->vmtx = tmpfile();
    FigureFullMetricsEnd(sf,gi);

    i = 0;
    if ( SCIsNotdef(sf->chars[0],fixed) )
	dumpglyph(sf->chars[i++],gi);
    else
	dumpmissingglyph(sf,gi,fixed);
    dumpblankglyph(gi,sf);	/* I'm not sure exactly why but there seem */
    dumpblankglyph(gi,sf);	/* to be a couple of blank glyphs at the start*/
    /* One is for NUL and one for CR I think... but why? */
    for ( cnt=0; i<sf->charcnt; ++i ) {
	if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]==SCDuplicate(sf->chars[i])) {
	    if ( (refs = SCCanonicalRefs(sf->chars[i],false))!=NULL )
		dumpcomposit(sf->chars[i],refs,gi);
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
    if ( gi->hmtxlen&2 ) putshort(gi->hmtx,0);
    if ( gi->loca[gi->next_glyph]&3 ) {
	for ( i=4-(gi->loca[gi->next_glyph]&3); i>0; --i )
	    putc('\0',gi->glyphs);
    }
    if ( sf->hasvmetrics ) {
	gi->vmtxlen = ftell(gi->vmtx);
	if ( gi->vmtxlen&2 ) putshort(gi->vmtx,0);
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
	    putlong(news,getushort(at->sidh));
	fclose(at->sidh);
	at->sidh = news;
    }
    if ( at->sidlongoffset )
	putlong(at->sidh,pos);
    else
	putshort(at->sidh,pos);

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

    putshort(cfff,1);		/* One font name */
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
    if ( SCIsNotdef(sf->chars[0],-1) )
	putshort(at->charset,storesid(at,sf->chars[0]->name));

    for ( i=1; i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i]) )
	    putshort(at->charset,storesid(at,sf->chars[i]->name));
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
		putshort(at->charset,start);
		putshort(at->charset,cid-1-start);	/* Count of glyphs in range excluding first */
		start = -1;
	    }
	} else {
	    if ( start==-1 ) start = cid;
	}
    }
    if ( start!=-1 ) {
	putshort(at->charset,start);
	putshort(at->charset,cid-1-start);	/* Count of glyphs in range excluding first */
    }
}

static void dumpcfffdselect(SplineFont *sf,struct alltabs *at) {
    int cid, k, max=0, lastfd, cnt;
    int gid;

    putc(3,at->fdselect);
    putshort(at->fdselect,0);		/* number of ranges, fill in later */

    for ( k=0; k<sf->subfontcnt; ++k )
	if ( sf->subfonts[k]->charcnt>max ) max = sf->subfonts[k]->charcnt;

    for ( k=0; k<sf->subfontcnt; ++k )
	if ( SCWorthOutputting(sf->subfonts[k]->chars[0]))
    break;
    if ( k==sf->subfontcnt ) --k;	/* If CID 0 not defined, put it in last font */
    putshort(at->fdselect,0);
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
		putshort(at->fdselect,gid);
		putc(k,at->fdselect);
		lastfd = k;
		++cnt;
	    }
	    ++gid;
	}
    }
    putshort(at->fdselect,gid);
    fseek(at->fdselect,1,SEEK_SET);
    putshort(at->fdselect,cnt);
    fseek(at->fdselect,0,SEEK_END);
}

static void dumpcffencoding(SplineFont *sf,struct alltabs *at) {
    int i,offset, pos;

    putc(0,at->encoding);
    /* I always use a format 0 encoding. ie. an array of glyph indexes */
    putc(0xff,at->encoding);
    /* And I put in 255 of them (with the encoding for 0 implied, I hope) */

    offset = 0;
    if ( SCIsNotdef(sf->chars[0],-1))
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
    for ( i=0; i<strs->next; ++i )
	len += strs->lens[i];

    /* Then output the index size and offsets */
    putshort( file, strs->next );
    if ( strs->next!=0 ) {
	offsize = len<=255?1:len<=65535?2:len<=0xffffff?3:4;
	putc(offsize,file);
	len = 1;
	for ( i=0; i<strs->next; ++i ) {
	    dumpoffset(file,offsize,len);
	    len += strs->lens[i];
	}
	dumpoffset(file,offsize,len);

	/* last of all the strings */
	for ( i=0; i<strs->next; ++i ) {
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
	if ( maxw>65535 ) maxw = 3*(sf->ascent+sf->descent);
	widths = gcalloc(maxw,sizeof(uint16));
	cumwid = gcalloc(maxw,sizeof(uint32));
	defwid = 0; cnt=0;
	for ( i=0; i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i]) &&
		    sf->chars[i]->width>=0 &&
		    sf->chars[i]->width<maxw )
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
	     strstrmatch(sf->weight,"Demi")!=NULL ||
	     strstrmatch(sf->weight,"Fett")!=NULL ||
	     strstrmatch(sf->weight,"Gras")!=NULL ||
	     strstrmatch(sf->weight,"Heavy")!=NULL ||
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

    putshort(cfff,1);		/* One top dict */
    putc('\2',cfff);		/* Offset size */
    putshort(cfff,1);		/* Offset to topdict */
    at->lenpos = ftell(cfff);
    putshort(cfff,0);		/* placeholder for final position (final offset in index points beyond last element) */
    dumpsid(cfff,at,sf->version,0);
    dumpsid(cfff,at,sf->copyright,1);
    dumpsid(cfff,at,sf->fullname?sf->fullname:sf->fontname,2);
    dumpsid(cfff,at,sf->familyname,3);
    dumpsid(cfff,at,sf->weight,4);
    if ( SFOneWidth(sf)!=-1 ) dumpintoper(cfff,1,(12<<8)|1);
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

    putshort(at->fdarray,sf->subfontcnt);
    putc('\2',at->fdarray);		/* DICTs aren't very big, and there are at most 255 */\
    putshort(at->fdarray,1);		/* Offset to first dict */
    for ( i=0; i<sf->subfontcnt; ++i )
	putshort(at->fdarray,0);	/* Dump offset placeholders (note there's one extra to mark the end) */
    pos = ftell(at->fdarray)-1;
    for ( i=0; i<sf->subfontcnt; ++i ) {
	at->fds[i].fillindictmark = dumpcffdict(sf->subfonts[i],at);
	at->fds[i].eodictmark = ftell(at->fdarray);
	if ( at->fds[i].eodictmark>65536 )
	    GDrawIError("The DICT INDEX got too big, result won't work");
    }
    fseek(at->fdarray,2*sizeof(short)+sizeof(char),SEEK_SET);
    for ( i=0; i<sf->subfontcnt; ++i )
	putshort(at->fdarray,at->fds[i].eodictmark-pos);
    fseek(at->fdarray,0,SEEK_END);
}

static void dumpcffcidtopdict(SplineFont *sf,struct alltabs *at) {
    char *pt, *end;
    FILE *cfff = at->cfff;
    DBounds b;
    int cidcnt=0, k;

    for ( k=0; k<sf->subfontcnt; ++k )
	if ( sf->subfonts[k]->charcnt>cidcnt ) cidcnt = sf->subfonts[k]->charcnt;

    putshort(cfff,1);		/* One top dict */
    putc('\2',cfff);		/* Offset size */
    putshort(cfff,1);		/* Offset to topdict */
    at->lenpos = ftell(cfff);
    putshort(cfff,0);		/* placeholder for final position */
    dumpsid(cfff,at,sf->cidregistry,-1);
    dumpsid(cfff,at,sf->ordering,-1);
    dumpintoper(cfff,sf->supplement,(12<<8)|30);		/* ROS operator must be first */
    dumpdbloper(cfff,sf->cidversion,(12<<8)|31);
    dumpintoper(cfff,cidcnt,(12<<8)|34);
    dumpintoper(cfff, sf->uniqueid?sf->uniqueid:4000000 + (rand()&0x3ffff), (12<<8)|35 );

    dumpsid(cfff,at,sf->copyright,1);
    dumpsid(cfff,at,sf->fullname?sf->fullname:sf->fontname,2);
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
    putshort(at->cfff,eotop);
    fseek(at->cfff,0,SEEK_END);

    /* String Index */
    putshort(at->cfff,at->sidcnt-1);
    if ( at->sidcnt!=1 ) {		/* Everybody gets an added NULL */
	putc(at->sidlongoffset?4:2,at->cfff);
	if ( !ttfcopyfile(at->cfff,at->sidh,base)) at->error = true;
	if ( !ttfcopyfile(at->cfff,at->sidf,base+shlen)) at->error = true;
    }

    /* Global Subrs */
    putshort(at->cfff,0);

    /* Charset */
    if ( !ttfcopyfile(at->cfff,at->charset,base+strlen+glen)) at->error = true;

    /* Encoding */
    if ( !ttfcopyfile(at->cfff,at->encoding,base+strlen+glen+csetlen)) at->error = true;

    /* Char Strings */
    if ( !ttfcopyfile(at->cfff,at->charstrings,base+strlen+glen+csetlen+enclen)) at->error = true;

    /* Private & Subrs */
    if ( !ttfcopyfile(at->cfff,at->private,base+strlen+glen+csetlen+enclen+cstrlen)) at->error = true;
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
    putshort(at->cfff,eotop);
    fseek(at->cfff,0,SEEK_END);

    /* String Index */
    putshort(at->cfff,at->sidcnt-1);
    if ( at->sidcnt!=1 ) {		/* Everybody gets an added NULL */
	putc(at->sidlongoffset?4:2,at->cfff);
	if ( !ttfcopyfile(at->cfff,at->sidh,base)) at->error = true;
	if ( !ttfcopyfile(at->cfff,at->sidf,base+shlen)) at->error = true;
    }

    /* Global Subrs */
    putshort(at->cfff,0);

    /* Charset */
    if ( !ttfcopyfile(at->cfff,at->charset,base+strlen+glen)) at->error = true;

    /* FDSelect */
    if ( !ttfcopyfile(at->cfff,at->fdselect,base+strlen+glen+csetlen)) at->error = true;

    /* Char Strings */
    if ( !ttfcopyfile(at->cfff,at->charstrings,base+strlen+glen+csetlen+fdsellen)) at->error = true;

    /* FDArray (DICT Index) */
    if ( !ttfcopyfile(at->cfff,at->fdarray,base+strlen+glen+csetlen+fdsellen+cstrlen)) at->error = true;

    /* Private & Subrs */
    prvlen = 0;
    for ( i=0; i<sf->subfontcnt; ++i ) {
	int temp = ftell(at->fds[i].private);
	if ( !ttfcopyfile(at->cfff,at->fds[i].private,
		base+strlen+glen+csetlen+fdsellen+cstrlen+fdarrlen+prvlen)) at->error = true;
	prvlen += temp;
    }

    free(at->fds);
}

static void dumpcffhmtx(struct alltabs *at,SplineFont *sf,int bitmaps) {
    DBounds b;
    SplineChar *sc;
    int i,cnt;
    int dovmetrics = sf->hasvmetrics;
    int width = SFOneWidth(sf);

    at->gi.hmtx = tmpfile();
    if ( dovmetrics )
	at->gi.vmtx = tmpfile();
    FigureFullMetricsEnd(sf,&at->gi);
    if ( SCIsNotdef(sf->chars[0],-1)) {
	putshort(at->gi.hmtx,sf->chars[0]->width);
	SplineCharFindBounds(sf->chars[0],&b);
	putshort(at->gi.hmtx,b.minx);
	if ( dovmetrics ) {
	    putshort(at->gi.vmtx,sf->chars[0]->vwidth);
	    putshort(at->gi.vmtx,sf->vertical_origin-b.miny);
	}
	i = 1;
    } else {
	i = 0;
	putshort(at->gi.hmtx,width==-1?sf->ascent+sf->descent:width);
	putshort(at->gi.hmtx,0);
	if ( dovmetrics ) {
	    putshort(at->gi.vmtx,sf->ascent+sf->descent);
	    putshort(at->gi.vmtx,0);
	}
    }
    cnt = 1;
    if ( bitmaps ) {
	if ( width==-1 ) width = (sf->ascent+sf->descent)/3;
	putshort(at->gi.hmtx,width);
	putshort(at->gi.hmtx,0);
	if ( dovmetrics ) {
	    putshort(at->gi.vmtx,sf->ascent+sf->descent);
	    putshort(at->gi.vmtx,0);
	}
	putshort(at->gi.hmtx,width);
	putshort(at->gi.hmtx,0);
	if ( dovmetrics ) {
	    putshort(at->gi.vmtx,sf->ascent+sf->descent);
	    putshort(at->gi.vmtx,0);
	}
	cnt = 3;
    }
    for ( i=1; i<sf->charcnt; ++i ) {
	sc = sf->chars[i];
	if ( SCWorthOutputting(sc)) {
	    if ( i<=at->gi.lasthwidth )
		putshort(at->gi.hmtx,sc->width);
	    SplineCharFindBounds(sc,&b);
	    putshort(at->gi.hmtx,b.minx);
	    if ( dovmetrics ) {
		if ( i<=at->gi.lasthwidth )
		    putshort(at->gi.vmtx,sc->vwidth);
		putshort(at->gi.vmtx,sf->vertical_origin-b.maxy);
	    }
	    ++cnt;
	    if ( i==at->gi.lasthwidth )
		at->gi.hfullcnt = cnt;
	    if ( i==at->gi.lastvwidth )
		at->gi.vfullcnt = cnt;
	}
    }
    at->gi.hmtxlen = ftell(at->gi.hmtx);
    if ( at->gi.hmtxlen&2 ) putshort(at->gi.hmtx,0);
    if ( dovmetrics ) {
	at->gi.vmtxlen = ftell(at->gi.vmtx);
	if ( at->gi.vmtxlen&2 ) putshort(at->gi.vmtx,0);
    }

    at->gi.maxp->numGlyphs = cnt;
}

static void dumpcffcidhmtx(struct alltabs *at,SplineFont *_sf) {
    DBounds b;
    SplineChar *sc;
    int cid,i,cnt=0,max;
    SplineFont *sf;
    int dovmetrics = _sf->hasvmetrics;

    at->gi.hmtx = tmpfile();
    if ( dovmetrics )
	at->gi.vmtx = tmpfile();
    FigureFullMetricsEnd(_sf,&at->gi);

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
	    if ( cid<=at->gi.lasthwidth )
		putshort(at->gi.hmtx,sc->width);
	    SplineCharFindBounds(sc,&b);
	    putshort(at->gi.hmtx,b.minx);
	    if ( dovmetrics ) {
		if ( cid<=at->gi.lasthwidth )
		    putshort(at->gi.vmtx,sc->vwidth);
		putshort(at->gi.vmtx,sf->vertical_origin-b.maxy);
	    }
	    ++cnt;
	    if ( cid==at->gi.lasthwidth )
		at->gi.hfullcnt = cnt;
	    if ( cid==at->gi.lastvwidth )
		at->gi.vfullcnt = cnt;
	} else if ( cid==0 && i==sf->subfontcnt ) {
	    /* Use final subfont to contain mythical default if there is no real default */
	    putshort(at->gi.hmtx,sf->ascent+sf->descent);
	    putshort(at->gi.hmtx,0);
	    ++cnt;
	    if ( dovmetrics ) {
		putshort(at->gi.vmtx,sf->ascent+sf->descent);
		putshort(at->gi.vmtx,0);
	    }
	}
    }
    at->gi.hmtxlen = ftell(at->gi.hmtx);
    if ( at->gi.hmtxlen&2 ) putshort(at->gi.hmtx,0);
    if ( dovmetrics ) {
	at->gi.vmtxlen = ftell(at->gi.vmtx);
	if ( at->gi.vmtxlen&2 ) putshort(at->gi.vmtx,0);
    }

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
    GProgressChangeStages(2+at->gi.strikecnt);
    dumpcffprivate(sf,at,-1);
    subrs = SplineFont2Subrs2(sf);
    _dumpcffstrings(at->private,subrs);
    GProgressNextStage();
    at->charstrings = dumpcffstrings(SplineFont2Chrs2(sf,at->nomwid,at->defwid,subrs));
    dumpcfftopdict(sf,at);
    finishup(sf,at);

    at->cfflen = ftell(at->cfff);
    if ( at->cfflen&3 ) {
	for ( i=4-(at->cfflen&3); i>0; --i )
	    putc('\0',at->cfff);
    }

    dumpcffhmtx(at,sf,false);
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

static int AnyWidthMDs(SplineFont *sf) {
    int i;
    MinimumDistance *md;

    if ( sf->subfontcnt!=0 ) {
	for ( i=0; i<sf->subfontcnt; ++i )
	    if ( AnyWidthMDs(sf->subfonts[i]))
return( true );
    } else {
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    for ( md=sf->chars[i]->md; md!=NULL; md = md->next )
		if ( md->sp2==NULL )
return( true );
	}
    }
return( false );
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
    head->flags = 3;
    if ( AnyWidthMDs(_sf))
	head->flags = 0x13;		/* baseline at 0, lsbline at 0, instructions change metrics */
    head->emunits = sf->ascent+sf->descent;
    head->macstyle = MacStyleCode(sf);
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
    if ( rl )
	head->flags |= (1<<9);		/* Apple documents this */
    /* if there are any indic characters, set bit 10 */

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

static void sethhead(struct hhead *hhead,struct hhead *vhead,struct alltabs *at, SplineFont *_sf) {
    int i, width, rbearing, height, bbearing;
    SplineFont *sf=NULL;
    DBounds bb;
    int j;
    /* Might as well fill in the vhead even if we don't use it */
    /*  we just won't dump it out if we don't want it */

    hhead->version = 0x00010000;
    hhead->ascender = _sf->ascent;
    hhead->descender = -_sf->descent;
    hhead->linegap = _sf->pfminfo.linegap;

    vhead->version = 0x00011000;
    vhead->ascender = (_sf->ascent+_sf->descent)/2;
    vhead->descender = -vhead->ascender;
    vhead->linegap = _sf->pfminfo.linegap;

    width = 0x80000000; rbearing = 0x7fffffff; height = 0x80000000; bbearing=0x7fffffff;
    j=0;
    do {
	sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[j];
	for ( i=0; i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i])) { /* I don't check for duplicates here. Shouldn't matter, they are duplicates, won't change things */
	    SplineCharFindBounds(sf->chars[i],&bb);
	    if ( sf->chars[i]->width>width ) width = sf->chars[i]->width;
	    if ( sf->chars[i]->vwidth>height ) height = sf->chars[i]->vwidth;
	    if ( sf->chars[i]->width-bb.maxx < rbearing ) rbearing = sf->chars[i]->width-bb.maxx;
	    if ( sf->chars[i]->vwidth-bb.maxy < bbearing ) bbearing = sf->chars[i]->vwidth-bb.maxy;
	}
	++j;
    } while ( j<_sf->subfontcnt );
    at->isfixed = SFOneWidth(sf)!=-1;
    hhead->maxwidth = width;
    hhead->minlsb = at->head.xmin;
    hhead->minrsb = rbearing;
    hhead->maxextent = at->head.xmax;
    hhead->caretSlopeRise = 1;

    vhead->maxwidth = height;
    vhead->minlsb = at->head.ymin;
    vhead->minrsb = bbearing;
    vhead->maxextent = at->head.ymax;
    vhead->caretSlopeRise = 1;

    hhead->numMetrics = at->gi.hfullcnt;
    vhead->numMetrics = at->gi.vfullcnt;
}

static void setvorg(struct vorg *vorg, SplineFont *sf) {
    vorg->majorVersion = 1;
    vorg->minorVersion = 0;
    vorg->defaultVertOriginY = sf->vertical_origin;
    vorg->numVertOriginYMetrics = 0;
}

static void OS2WeightCheck(struct pfminfo *pfminfo,char *weight) {
    if ( strstrmatch(weight,"medi")!=NULL ) {
	pfminfo->weight = 500;
	pfminfo->panose[2] = 6;
    } else if ( strstrmatch(weight,"demi")!=NULL ||
		strstrmatch(weight,"halb")!=NULL ||
		(strstrmatch(weight,"semi")!=NULL &&
		    strstrmatch(weight,"bold")!=NULL) ) {
	pfminfo->weight = 600;
	pfminfo->panose[2] = 7;
    } else if ( strstrmatch(weight,"bold")!=NULL ||
		strstrmatch(weight,"fett")!=NULL ||
		strstrmatch(weight,"gras")!=NULL ) {
	pfminfo->weight = 700;
	pfminfo->panose[2] = 8;
    } else if ( strstrmatch(weight,"heavy")!=NULL ) {
	pfminfo->weight = 800;
	pfminfo->panose[2] = 9;
    } else if ( strstrmatch(weight,"black")!=NULL ) {
	pfminfo->weight = 900;
	pfminfo->panose[2] = 10;
    } else if ( strstrmatch(weight,"nord")!=NULL ) {
	pfminfo->weight = 950;
	pfminfo->panose[2] = 11;
    } else if ( strstrmatch(weight,"thin")!=NULL ) {
	pfminfo->weight = 100;
	pfminfo->panose[2] = 2;
    } else if ( strstrmatch(weight,"extra")!=NULL ||
	    strstrmatch(weight,"light")!=NULL ) {
	pfminfo->weight = 200;
	pfminfo->panose[2] = 3;
    } else if ( strstrmatch(weight,"light")!=NULL ) {
	pfminfo->weight = 300;
	pfminfo->panose[2] = 4;
    }
}

void SFDefaultOS2Info(struct pfminfo *pfminfo,SplineFont *_sf,char *fontname) {
    int i, samewid= -1, j;
    SplineFont *sf, *first=NULL;
    char *weight = _sf->cidmaster==NULL ? _sf->weight : _sf->cidmaster->weight;

    if ( !pfminfo->pfmset ) {
	memset(pfminfo,'\0',sizeof(*pfminfo));
	j=0;
	do {
	    sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[j];
	    if ( first==NULL ) first = sf;
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
/* urw uses 4 character abreviations */
	if ( weight!=NULL )
	    OS2WeightCheck(pfminfo,weight);
	OS2WeightCheck(pfminfo,fontname);

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
	pfminfo->linegap = pfminfo->vlinegap =
		rint(.09*(first->ascent+first->descent));
    }
}

void OS2FigureCodePages(SplineFont *sf, uint32 CodePage[2]) {
    SplineFont *_sf;
    int i, k;

    CodePage[0] = CodePage[1] = 0;
    if ( sf->encoding_name==em_iso8859_1 || sf->encoding_name==em_iso8859_15 ||
	    sf->encoding_name==em_win )
	CodePage[0] |= 1<<0;	/* latin1 */
    else if ( sf->encoding_name==em_iso8859_2 )
	CodePage[0] |= 1<<1;	/* latin2 */
    else if ( sf->encoding_name==em_iso8859_9 )
	CodePage[0] |= 1<<4;	/* turkish */
    else if ( sf->encoding_name==em_iso8859_4 )
	CodePage[0] |= 1<<7;	/* baltic */
    else if ( sf->encoding_name==em_iso8859_5 || sf->encoding_name==em_koi8_r )
	CodePage[0] |= 1<<2;	/* cyrillic */
    else if ( sf->encoding_name==em_iso8859_7 )
	CodePage[0] |= 1<<3;	/* greek */
    else if ( sf->encoding_name==em_iso8859_8 )
	CodePage[0] |= 1<<5;	/* hebrew */
    else if ( sf->encoding_name==em_iso8859_6 )
	CodePage[0] |= 1<<6;	/* arabic */
    else if ( sf->encoding_name==em_iso8859_11 )
	CodePage[0] |= 1<<16;	/* thai */
    else if ( sf->encoding_name==em_jis201 || sf->encoding_name==em_jis208 ||
	    sf->encoding_name==em_jis212 || sf->encoding_name==em_sjis )
	CodePage[0] |= 1<<17;	/* japanese */
    else if ( sf->encoding_name==em_gb2312 )
	CodePage[0] |= 1<<18;	/* simplified chinese */
    else if ( sf->encoding_name==em_ksc5601 || sf->encoding_name==em_wansung )
	CodePage[0] |= 1<<19;	/* korean wansung */
    else if ( sf->encoding_name==em_big5 )
	CodePage[0] |= 1<<20;	/* traditional chinese */
    else if ( sf->encoding_name==em_johab )
	CodePage[0] |= 1<<21;	/* korean johab */
    else if ( sf->encoding_name==em_mac )
	CodePage[0] |= 1<<29;	/* mac */
    else if ( sf->encoding_name==em_symbol )
	CodePage[0] |= 1<<31;	/* symbol */

    k=0; _sf = sf;
    do {
	sf = ( _sf->subfontcnt==0 ) ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    if ( sf->chars[i]->unicodeenc==0xde )
		CodePage[0] |= 1<<0;		/* latin1 */
	    else if ( sf->chars[i]->unicodeenc==0x13d )
		CodePage[0] |= 1<<1;		/* latin2 */
	    else if ( sf->chars[i]->unicodeenc==0x411 )
		CodePage[0] |= 1<<2;		/* cyrillic */
	    else if ( sf->chars[i]->unicodeenc==0x386 )
		CodePage[0] |= 1<<3;		/* greek */
	    else if ( sf->chars[i]->unicodeenc==0x130 )
		CodePage[0] |= 1<<4;		/* turkish */
	    else if ( sf->chars[i]->unicodeenc==0x5d0 )
		CodePage[0] |= 1<<5;		/* hebrew */
	    else if ( sf->chars[i]->unicodeenc==0x631 )
		CodePage[0] |= 1<<6;		/* arabic */
	    else if ( sf->chars[i]->unicodeenc==0x157 )
		CodePage[0] |= 1<<7;		/* baltic */
	    else if ( sf->chars[i]->unicodeenc==0xe45 )
		CodePage[0] |= 1<<16;		/* thai */
	    else if ( sf->chars[i]->unicodeenc==0x30a8 )
		CodePage[0] |= 1<<17;		/* japanese */
	    else if ( sf->chars[i]->unicodeenc==0x3105 )
		CodePage[0] |= 1<<18;		/* simplified chinese */
	    else if ( sf->chars[i]->unicodeenc==0x3131 )
		CodePage[0] |= 1<<19;		/* korean wansung */
	    else if ( sf->chars[i]->unicodeenc==0xacf4 )
		CodePage[0] |= 1<<21;		/* korean Johab */
	    else if ( sf->chars[i]->unicodeenc==0x21d4 )
		CodePage[0] |= 1<<31;		/* symbol */
	}
	++k;
    } while ( k<_sf->subfontcnt );

    if ( CodePage[0]==0 )
	CodePage[0] |= 1;
}

static void setos2(struct os2 *os2,struct alltabs *at, SplineFont *_sf,
	enum fontformat format) {
    int i,j,cnt1,cnt2,first,last,avg1,avg2,k;
    SplineFont *sf = _sf;
    char *pt;

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
    if ( sf->fullname!=NULL && strstrmatch(sf->fullname,"outline")!=NULL )
	os2->fsSel |= 8;
    if ( os2->fsSel==0 ) os2->fsSel = 64;		/* Regular */
    os2->ascender = os2->winascent = at->head.ymax;
    os2->descender = at->head.ymin;
    os2->windescent = -at->head.ymin;
    os2->linegap = sf->pfminfo.linegap;

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

    if ( TTFFoundry!=NULL )
	strncpy(os2->achVendID,TTFFoundry,4);
    else
	memcpy(os2->achVendID,"PfEd",4);
    for ( pt=os2->achVendID; pt<os2->achVendID && *pt!='\0'; ++pt );
    while ( pt<os2->achVendID ) *pt++ = ' ';	/* Pad with spaces not NUL */

    if ( cnt1==27 )
	os2->avgCharWid = avg1/cnt1;
    else if ( cnt2!=0 )
	os2->avgCharWid = avg2/cnt2;
    memcpy(os2->panose,sf->pfminfo.panose,sizeof(os2->panose));
    os2->firstcharindex = first;
    os2->lastcharindex = last;
    OS2FigureCodePages(sf, os2->ulCodePage);
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
	os2->maxContext = 1;	/* Kerning will set this to 2, ligature to whatever */
    }
}

static void redoloca(struct alltabs *at) {
    int i;

    at->loca = tmpfile();
    if ( at->head.locais32 ) {
	for ( i=0; i<=at->maxp.numGlyphs; ++i )
	    putlong(at->loca,at->gi.loca[i]);
	at->localen = sizeof(int32)*(at->maxp.numGlyphs+1);
    } else {
	for ( i=0; i<=at->maxp.numGlyphs; ++i )
	    putshort(at->loca,at->gi.loca[i]/2);
	at->localen = sizeof(int16)*(at->maxp.numGlyphs+1);
	if ( ftell(at->loca)&2 )
	    putshort(at->loca,0);
    }
    free(at->gi.loca);
}

static void redohead(struct alltabs *at) {
    at->headf = tmpfile();

    putlong(at->headf,at->head.version);
    putlong(at->headf,at->head.revision);
    putlong(at->headf,at->head.checksumAdj);
    putlong(at->headf,at->head.magicNum);
    putshort(at->headf,at->head.flags);
    putshort(at->headf,at->head.emunits);
    putlong(at->headf,at->head.createtime[0]);
    putlong(at->headf,at->head.createtime[1]);
    putlong(at->headf,at->head.modtime[0]);
    putlong(at->headf,at->head.modtime[1]);
    putshort(at->headf,at->head.xmin);
    putshort(at->headf,at->head.ymin);
    putshort(at->headf,at->head.xmax);
    putshort(at->headf,at->head.ymax);
    putshort(at->headf,at->head.macstyle);
    putshort(at->headf,at->head.lowestreadable);
    putshort(at->headf,at->head.dirhint);
    putshort(at->headf,at->head.locais32);
    putshort(at->headf,at->head.glyphformat);

    at->headlen = ftell(at->headf);
    if ( (at->headlen&2)!=0 )
	putshort(at->headf,0);
}

static void redohhead(struct alltabs *at,int isv) {
    int i;
    struct hhead *head;
    FILE *f;

    if ( !isv ) {
	f = at->hheadf = tmpfile();
	head = &at->hhead;
    } else {
	f = at->vheadf = tmpfile();
	head = &at->vhead;
    }

    putlong(f,head->version);
    putshort(f,head->ascender);
    putshort(f,head->descender);
    putshort(f,head->linegap);
    putshort(f,head->maxwidth);
    putshort(f,head->minlsb);
    putshort(f,head->minrsb);
    putshort(f,head->maxextent);
    putshort(f,head->caretSlopeRise);
    putshort(f,head->caretSlopeRun);
    for ( i=0; i<5; ++i )
	putshort(f,head->mbz[i]);
    putshort(f,head->metricformat);
    putshort(f,head->numMetrics);

    if ( !isv ) {
	at->hheadlen = ftell(f);
	if ( (at->hheadlen&2)!=0 )
	    putshort(f,0);
    } else {
	at->vheadlen = ftell(f);
	if ( (at->vheadlen&2)!=0 )
	    putshort(f,0);
    }
}

static void redovorg(struct alltabs *at) {

    at->vorgf = tmpfile();
    putshort(at->vorgf,at->vorg.majorVersion);
    putshort(at->vorgf,at->vorg.minorVersion);
    putshort(at->vorgf,at->vorg.defaultVertOriginY);
    putshort(at->vorgf,at->vorg.numVertOriginYMetrics);

    at->vorglen = ftell(at->vorgf);
    if ( (at->vorglen&2)!=0 )
	putshort(at->vorgf,0);
}

static void redomaxp(struct alltabs *at,enum fontformat format) {
    at->maxpf = tmpfile();

    putlong(at->maxpf,at->maxp.version);
    putshort(at->maxpf,at->maxp.numGlyphs);
    if ( format!=ff_otf && format!=ff_otfcid ) {
	putshort(at->maxpf,at->maxp.maxPoints);
	putshort(at->maxpf,at->maxp.maxContours);
	putshort(at->maxpf,at->maxp.maxCompositPts);
	putshort(at->maxpf,at->maxp.maxCompositCtrs);
	putshort(at->maxpf,at->maxp.maxZones);
	putshort(at->maxpf,at->maxp.maxTwilightPts);
	putshort(at->maxpf,at->maxp.maxStorage);
	putshort(at->maxpf,at->maxp.maxFDEFs);
	putshort(at->maxpf,at->maxp.maxIDEFs);
	putshort(at->maxpf,at->maxp.maxStack);
	putshort(at->maxpf,at->maxp.maxglyphInstr);
	putshort(at->maxpf,at->maxp.maxnumcomponents);
	putshort(at->maxpf,at->maxp.maxcomponentdepth);
    }

    at->maxplen = ftell(at->maxpf);
    if ( (at->maxplen&2)!=0 )
	putshort(at->maxpf,0);
}

static void redoos2(struct alltabs *at) {
    int i;
    at->os2f = tmpfile();

    putshort(at->os2f,at->os2.version);
    putshort(at->os2f,at->os2.avgCharWid);
    putshort(at->os2f,at->os2.weightClass);
    putshort(at->os2f,at->os2.widthClass);
    putshort(at->os2f,at->os2.fstype);
    putshort(at->os2f,at->os2.ysubXSize);
    putshort(at->os2f,at->os2.ysubYSize);
    putshort(at->os2f,at->os2.ysubXOff);
    putshort(at->os2f,at->os2.ysubYOff);
    putshort(at->os2f,at->os2.ysupXSize);
    putshort(at->os2f,at->os2.ysupYSize);
    putshort(at->os2f,at->os2.ysupXOff);
    putshort(at->os2f,at->os2.ysupYOff);
    putshort(at->os2f,at->os2.yStrikeoutSize);
    putshort(at->os2f,at->os2.yStrikeoutPos);
    putshort(at->os2f,at->os2.sFamilyClass);
    for ( i=0; i<10; ++i )
	putc(at->os2.panose[i],at->os2f);
    for ( i=0; i<4; ++i )
	putlong(at->os2f,at->os2.unicoderange[i]);
    for ( i=0; i<4; ++i )
	putc(at->os2.achVendID[i],at->os2f);
    putshort(at->os2f,at->os2.fsSel);
    putshort(at->os2f,at->os2.firstcharindex);
    putshort(at->os2f,at->os2.lastcharindex);
    putshort(at->os2f,at->os2.ascender);
    putshort(at->os2f,at->os2.descender);
    putshort(at->os2f,at->os2.linegap);
    putshort(at->os2f,at->os2.winascent);
    putshort(at->os2f,at->os2.windescent);
    putlong(at->os2f,at->os2.ulCodePage[0]);
    putlong(at->os2f,at->os2.ulCodePage[1]);

    if ( at->os2.version>=2 ) {
	putshort(at->os2f,at->os2.xHeight);
	putshort(at->os2f,at->os2.capHeight);
	putshort(at->os2f,at->os2.defChar);
	putshort(at->os2f,at->os2.breakChar);
	putshort(at->os2f,at->os2.maxContext);
    }

    at->os2len = ftell(at->os2f);
    if ( (at->os2len&2)!=0 )
	putshort(at->os2f,0);
}

static void redocvt(struct alltabs *at) {
    int i;
    at->cvtf = tmpfile();

    for ( i=0; i<at->gi.cvtcur; ++i )
	putshort(at->cvtf,at->gi.cvt[i]);
    at->cvtlen = ftell(at->cvtf);
    if ( 1&at->gi.cvtcur )
	putshort(at->cvtf,0);
}

/* scripts (for opentype) that I understand */
/* these are in alphabetical order */
enum scripts { script_arabic, script_armenian, script_bengali, script_bopo,
	script_braille, script_cansyl, script_cherokee, script_cyrillic,
	script_devanagari, script_ethiopic, script_georgian, script_greek,
	script_gujarati, script_gurmukhi, script_hangul, script_cjk,
	script_hebrew, script_hiragana, script_jamo, script_katakana,
	script_khmer, script_kannada, script_latin, script_lao,
	script_malayalam, script_mongolian, script_myamar, script_ogham,
	script_oriya, script_runic, script_sinhala, script_syriac,
	script_tamil, script_telugu, script_thaana, script_thai,
	script_tibetan, script_yi
};

static int scripts[][11] = {
/* Arabic */	{ CHR('a','r','a','b'), 0x0600, 0x06ff, 0xfb50, 0xfdff, 0xfe70, 0xfeff },
/* Armenian */	{ CHR('a','r','m','n'), 0x0530, 0x058f, 0xfb13, 0xfb17 },
/* Bengali */	{ CHR('b','e','n','g'), 0x0980, 0x09ff },
/* Bopomofo */	{ CHR('b','o','p','o'), 0x3100, 0x312f },
/* Braille */	{ CHR('b','r','a','i'), 0x2800, 0x28ff },
/* Canadian Syl*/{CHR('c','a','n','s'), 0x1400, 0x167f },
/* Cherokee */	{ CHR('c','h','e','r'), 0x13a0, 0x13ff },
/* Cyrillic */	{ CHR('c','y','r','l'), 0x0500, 0x052f },
/* Devanagari */{ CHR('d','e','v','a'), 0x0900, 0x097f },
/* Ethiopic */	{ CHR('e','t','h','i'), 0x1300, 0x139f },
/* Georgian */	{ CHR('g','e','o','r'), 0x1080, 0x10ff },
/* Greek */	{ CHR('g','r','e','k'), 0x0370, 0x03ff, 0x1f00, 0x1fff },
/* Gujarati */	{ CHR('g','u','j','r'), 0x0a80, 0x0aff },
/* Gurmukhi */	{ CHR('g','u','r','u'), 0x0a00, 0x0a7f },
/* Hangul */	{ CHR('h','a','n','g'), 0xac00, 0xd7af },
/* CJKIdeogra */{ CHR('h','a','n','i'), 0x3300, 0x9fff, 0xf900, 0xfaff, 0x020000, 0x02ffff },
/* Hebrew */	{ CHR('h','e','b','r'), 0x0590, 0x05ff, 0xfb1e, 0xfb4ff },
/* Hiragana */	{ CHR('h','i','r','a'), 0x3040, 0x309f },
/* Hangul Jamo*/{ CHR('j','a','m','o'), 0x1100, 0x11ff, 0x3130, 0x319f, 0xffa0, 0xffdf },
/* Katakana */	{ CHR('k','a','n','a'), 0x30a0, 0x30ff, 0xff60, 0xff9f },
/* Khmer */	{ CHR('k','h','m','r'), 0x1780, 0x17ff },
/* Kannada */	{ CHR('k','n','d','a'), 0x0c80, 0x0cff },
/* Latin */	{ CHR('l','a','t','n'), 0x0000, 0x02af, 0x1d00, 0x1eff, 0xfb00, 0xfb0f, 0xff00, 0xff5f, 0, 0 },
/* Lao */	{ CHR('l','a','o',' '), 0x0e80, 0x0eff },
/* Malayalam */	{ CHR('m','l','y','m'), 0x0d00, 0x0d7f },
/* Mongolian */	{ CHR('m','o','n','g'), 0x1800, 0x18af },
/* Myanmar */	{ CHR('m','y','m','r'), 0x1000, 0x107f },
/* Ogham */	{ CHR('o','g','a','m'), 0x1680, 0x169f },
/* Oriya */	{ CHR('o','r','y','a'), 0x0b00, 0x0b7f },
/* Runic */	{ CHR('r','u','n','r'), 0x16a0, 0x16ff },
/* Sinhala */	{ CHR('s','i','n','h'), 0x0d80, 0x0dff },
/* Syriac */	{ CHR('s','y','r','c'), 0x0700, 0x074f },
/* Tamil */	{ CHR('t','a','m','l'), 0x0b80, 0x0bff },
/* Telugu */	{ CHR('t','e','l','u'), 0x0c00, 0x0c7f },
/* Thaana */	{ CHR('t','h','a','a'), 0x0780, 0x07bf },
/* Thai */	{ CHR('t','h','a','i'), 0x0e00, 0x0e7f },
/* Tibetan */	{ CHR('t','i','b','t'), 0x0f00, 0x0fff },
/* Yi */	{ CHR('y','i',' ',' '), 0xa000, 0xa73f },
		{ -1 }
};
/* I'll bundle all left to right scripts into one lookup, and hebrew/arabic into*/
/*  another (the only reason I distinguish is because I need to specify direction) */
static int *FeatureScriptMask(SplineFont *sf,int iskern, int mask[]) {
    SplineFont *sub;
    int k,i,u, s;

    for ( s=0; scripts[s][0]!=-1; ++s )
	mask[s>>5] = 0;
    k = 0;
    do {
	sub = ( sf->subfontcnt==0 ) ? sf : sf->subfonts[k];
	for ( i=0; i<sub->charcnt; ++i )
		if ( sub->chars[i]!=NULL &&
			((iskern && sub->chars[i]->kerns!=NULL) ||
			 (!iskern && sub->chars[i]->ligofme!=NULL)) &&
			(u=sub->chars[i]->unicodeenc)!=-1 ) {
	    for ( s=0; scripts[s][0]!=-1; ++s ) {
		for ( k=1; scripts[s][k+1]!=0; k += 2 )
		    if ( u>=scripts[s][k] && u<=scripts[s][k+1] )
		break;
		if ( scripts[s][k+1]!=0 )
	    break;
	    }
	    if ( scripts[s][0]==-1 ) {
		/* Pick a default... */
		for ( s=0; scripts[s][1]!=0; ++s );	/* Pick latin */
	    }
	    if ( !iskern && s==script_jamo )	/* a ligature of jamo is a hangul sylable */
		s = script_hangul;
	    mask[s>>5] |= (1<<(s&0x1f));
	}
	++k;
    } while ( k<sf->subfontcnt );
return( mask );
}

struct simplesubs { uint16 orig, replacement; SplineChar *origsc; };

static struct simplesubs *VerticalRotationGlyphs(SplineFont *sf) {
    int cnt, i, j, k;
    struct simplesubs *subs = NULL;
    SplineFont *_sf = sf;
    SplineChar *sc, *scbase;

    for ( k=0; k<2; ++k ) {
	cnt = 0;
	j = 0;
	do {
	    sf = _sf->subfontcnt==0 ? _sf : _sf->subfonts[j];
	    for ( i=0; i<sf->charcnt; ++i ) if ( SCWorthOutputting(sc=sf->chars[i])) {
		if ( sf->cidmaster!=NULL && strncmp(sc->name,"vertcid_",8)==0 ) {
		    char *end;
		    int cid = strtol(sc->name+8,&end,10), j;
		    if ( *end!='\0' || (j=SFHasCID(sf,cid))==-1)
	    continue;
		    scbase = sf->cidmaster->subfonts[j]->chars[cid];
		} else if ( strncmp(sc->name,"vertuni",7)==0 && strlen(sc->name)==11 ) {
		    char *end;
		    int uni = strtol(sc->name+7,&end,16), index;
		    if ( *end!='\0' || (index = SFCIDFindExistingChar(sf,uni,NULL))==-1 )
	    continue;
		    if ( sf->cidmaster==NULL )
			scbase = sf->chars[index];
		    else
			scbase = sf->cidmaster->subfonts[SFHasCID(sf,index)]->chars[index];
		} else
	    continue;
		if ( !SCWorthOutputting(scbase))
	    continue;
		if ( subs!=NULL ) {
		    subs[cnt].origsc = scbase;
		    subs[cnt].orig = scbase->ttf_glyph;
		    subs[cnt].replacement = sc->ttf_glyph;
		}
		++cnt;
	    }
	    ++j;
	} while ( j<_sf->subfontcnt );
	if ( cnt==0 )
return( NULL );
	else if ( subs!=NULL )
return( subs );
	subs = gcalloc(cnt+1,sizeof(struct simplesubs));
    }
return( NULL );
}

static SplineChar *SFFindSC(SplineFont *sf,int uni) {
    int i = SFFindExistingChar(sf,uni,NULL);
    if ( i==-1 )
return( NULL );
return( sf->chars[i] );
}

static struct simplesubs *longs(SplineFont *sf) {
    struct simplesubs *subs = NULL;
    SplineChar *sc, *scbase;
    /* Convert s->longs initially and medially */

    scbase = SFFindSC(sf,'s');
    sc = SFFindSC(sf,0x17f);
    if ( SCWorthOutputting(sc) && SCWorthOutputting(scbase)) {
	subs = gcalloc(2,sizeof(struct simplesubs));
	subs[0].origsc = scbase;
	subs[0].orig = scbase->ttf_glyph;
	subs[0].replacement = sc->ttf_glyph;
    }
return( subs );
}

static struct simplesubs *arabic_forms(SplineFont *sf, int form) {
    /* data for arabic form conversion */
    int cnt, j, k;
    struct simplesubs *subs = NULL;
    SplineChar *sc, *scbase;

    for ( k=0; k<2; ++k ) {
	cnt = 0;
	for ( j = 0x0600; j<0x0700; ++j ) {
	    if ( (&(ArabicForms[j-0x600].initial))[form]!=0 &&
		    (&(ArabicForms[j-0x600].initial))[form]!=j &&
		    (scbase = SFFindSC(sf,j))!=NULL &&
		    (sc = SFFindSC(sf,(&(ArabicForms[j-0x600].initial))[form]))!=NULL &&
		    SCWorthOutputting(scbase) &&
		    SCWorthOutputting(sc)) {
		if ( subs!=NULL ) {
		    subs[cnt].origsc = scbase;
		    subs[cnt].orig = scbase->ttf_glyph;
		    subs[cnt].replacement = sc->ttf_glyph;
		}
		++cnt;
	    }
	}
	if ( cnt==0 )
return( NULL );
	else if ( subs!=NULL )
return( subs );
	subs = gcalloc(cnt+1,sizeof(struct simplesubs));
    }
return( NULL );
}

static SplineChar **generateGlyphList(SplineFont *sf, int iskern, int isr2l) {
    int cnt;
    SplineFont *sub;
    SplineChar *sc;
    int k,i,j,u,r2l;
    KernPair *kp;
    SplineChar **glyphs=NULL;

    for ( j=0; j<2; ++j ) {
	k = 0;
	cnt = 0;
	do {
	    sub = ( sf->subfontcnt==0 ) ? sf : sf->subfonts[k];
	    for ( i=0; i<sub->charcnt; ++i )
		    if ( (sc=sub->chars[i])!=NULL &&
			    ((iskern && sc->kerns!=NULL ) ||
			     (!iskern && sc->ligofme!=NULL )) ) {
		u=sc->unicodeenc;
		r2l = 0;
		if ( u==-1 ) {
		    /* If it's not something we know about try to guess its */
		    /*  directionality by seeing whether it kerns with r2l or */
		    /*  l2r letters */
		    for ( kp = sc->kerns; kp!=NULL; kp=kp->next )
			if ( kp->sc->unicodeenc!=-1 ) {
			    if ( isrighttoleft(kp->sc->unicodeenc))
				r2l = true;
		    break;
			}
		}
		if ( r2l==isr2l ) {
		    if ( glyphs!=NULL ) glyphs[cnt] = sc;
		    ++cnt;
		}
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
	if ( glyphs==NULL ) {
	    if ( cnt==0 )
return( NULL );
	    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
	}
    }
    glyphs[cnt] = NULL;
return( glyphs );
}

static void dumpcoveragetable(FILE *gpos,SplineChar **glyphs) {
    int i, last = -2, range_cnt=0, start;
    /* the glyph list should already be sorted */
    /* figure out whether it is better (smaller) to use an array of glyph ids */
    /*  or a set of glyph id ranges */

    for ( i=0; glyphs[i]!=NULL; ++i ) {
	if ( glyphs[i]->ttf_glyph<=last )
	    GDrawIError("Glyphs must be ordered when creating coverage table");
	if ( glyphs[i]->ttf_glyph!=last+1 )
	    ++range_cnt;
	last = glyphs[i]->ttf_glyph;
    }
    if ( i<=3*range_cnt ) {
	/* We use less space with a list of glyphs than with a set of ranges */
	putshort(gpos,1);		/* Coverage format=1 => glyph list */
	putshort(gpos,i);		/* count of glyphs */
	for ( i=0; glyphs[i]!=NULL; ++i )
	    putshort(gpos,glyphs[i]->ttf_glyph);	/* array of glyph IDs */
    } else {
	putshort(gpos,2);		/* Coverage format=2 => range list */
	putshort(gpos,range_cnt);	/* count of ranges */
	last = -2; start = -2;
	for ( i=0; glyphs[i]!=NULL; ++i ) {
	    if ( glyphs[i]->ttf_glyph!=last+1 ) {
		if ( last!=-2 ) {
		    putshort(gpos,glyphs[start]->ttf_glyph);	/* start glyph ID */
		    putshort(gpos,last);			/* end glyph ID */
		    putshort(gpos,start);			/* coverage index of start glyph */
		}
		start = i;
	    }
	    last = glyphs[i]->ttf_glyph;
	}
    }
}

static void dumpgposkerndata(FILE *gpos,SplineFont *sf,int isr2l,struct alltabs *at) {
    int32 coverage_pos, next_val_pos, here;
    int cnt, i, pcnt, max=100, j,k;
    int *seconds = galloc(max*sizeof(int));
    int *changes = galloc(max*sizeof(int));
    int16 *offsets=NULL;
    SplineChar **glyphs;
    KernPair *kp;

    glyphs = generateGlyphList(sf,true,isr2l);
    cnt=0;
    if ( glyphs!=NULL ) {
	for ( ; glyphs[cnt]!=NULL; ++cnt );
	at->os2.maxContext = 2;
    }

    putshort(gpos,1);		/* format 1 of the pair adjustment subtable */
    coverage_pos = ftell(gpos);
    putshort(gpos,0);		/* offset to coverage table */
    putshort(gpos,0x0004);	/* Alter XAdvance of first character */
    putshort(gpos,0x0000);	/* leave second char alone */
    putshort(gpos,cnt);
    next_val_pos = ftell(gpos);
    if ( glyphs!=NULL )
	offsets = galloc(cnt*sizeof(int16));
    for ( i=0; i<cnt; ++i )
	putshort(gpos,0);
    for ( i=0; i<cnt; ++i ) {
	offsets[i] = ftell(gpos)-coverage_pos+2;
	for ( pcnt = 0, kp = glyphs[i]->kerns; kp!=NULL; kp=kp->next ) ++pcnt;
	putshort(gpos,pcnt);
	if ( pcnt>=max ) {
	    max = pcnt+100;
	    seconds = grealloc(seconds,max*sizeof(int));
	    changes = grealloc(changes,max*sizeof(int));
	}
	for ( pcnt = 0, kp = glyphs[i]->kerns; kp!=NULL; kp=kp->next ) {
	    seconds[pcnt] = kp->sc->ttf_glyph;
	    changes[pcnt++] = kp->off;
	}
	for ( j=0; j<pcnt-1; ++j ) for ( k=j+1; k<pcnt; ++k ) {
	    if ( seconds[k]<seconds[j] ) {
		int temp = seconds[k];
		seconds[k] = seconds[j];
		seconds[j] = temp;
		temp = changes[k];
		changes[k] = changes[j];
		changes[j] = temp;
	    }
	}
	for ( j=0; j<pcnt; ++j ) {
	    putshort(gpos,seconds[j]);
	    putshort(gpos,changes[j]);
	}
    }
    free(seconds);
    free(changes);
    if ( glyphs!=NULL ) {
	here = ftell(gpos);
	fseek(gpos,coverage_pos,SEEK_SET);
	putshort(gpos,here-coverage_pos+2);
	fseek(gpos,next_val_pos,SEEK_SET);
	for ( i=0; i<cnt; ++i )
	    putshort(gpos,offsets[i]);
	fseek(gpos,here,SEEK_SET);
	dumpcoveragetable(gpos,glyphs);
	free(glyphs);
	free(offsets);
    }
}

static void dumpgsubligdata(FILE *gsub,SplineFont *sf,int isr2l,struct alltabs *at) {
    int32 coverage_pos, next_val_pos, here, lig_list_start;
    int cnt, i, pcnt, lcnt, max=100;
    uint16 *offsets=NULL, *ligoffsets=galloc(max*sizeof(uint16));
    SplineChar **glyphs;
    LigList *ll;
    struct splinecharlist *scl;

    glyphs = generateGlyphList(sf,false,isr2l);
    cnt=0;
    if ( glyphs!=NULL ) for ( ; glyphs[cnt]!=NULL; ++cnt );

    putshort(gsub,1);		/* only one format for ligatures */
    coverage_pos = ftell(gsub);
    putshort(gsub,0);		/* offset to coverage table */
    putshort(gsub,cnt);
    next_val_pos = ftell(gsub);
    if ( glyphs!=NULL )
	offsets = galloc(cnt*sizeof(int16));
    for ( i=0; i<cnt; ++i )
	putshort(gsub,0);
    for ( i=0; i<cnt; ++i ) {
	offsets[i] = ftell(gsub)-coverage_pos+2;
	for ( pcnt = 0, ll = glyphs[i]->ligofme; ll!=NULL; ll=ll->next ) ++pcnt;
	putshort(gsub,pcnt);
	if ( pcnt>=max ) {
	    max = pcnt+100;
	    ligoffsets = grealloc(ligoffsets,max*sizeof(int));
	}
	lig_list_start = ftell(gsub);
	for ( ll = glyphs[i]->ligofme; ll!=NULL; ll=ll->next )
	    putshort(gsub,0);			/* Place holders */
	for ( pcnt=0, ll = glyphs[i]->ligofme; ll!=NULL; ll=ll->next, ++pcnt ) {
	    ligoffsets[pcnt] = ftell(gsub)-lig_list_start+2;
	    putshort(gsub,ll->lig->lig->ttf_glyph);
	    for ( lcnt=0, scl=ll->components; scl!=NULL; scl=scl->next ) ++lcnt;
	    putshort(gsub, lcnt+1);
	    if ( lcnt+1>at->os2.maxContext )
		at->os2.maxContext = lcnt+1;
	    for ( scl=ll->components; scl!=NULL; scl=scl->next )
		putshort(gsub, scl->sc->ttf_glyph );
	}
	fseek(gsub,lig_list_start,SEEK_SET);
	for ( pcnt=0, ll = glyphs[i]->ligofme; ll!=NULL; ll=ll->next, ++pcnt )
	    putshort(gsub,ligoffsets[pcnt]);
	fseek(gsub,0,SEEK_END);
    }
    free(ligoffsets);
    if ( glyphs!=NULL ) {
	here = ftell(gsub);
	fseek(gsub,coverage_pos,SEEK_SET);
	putshort(gsub,here-coverage_pos+2);
	fseek(gsub,next_val_pos,SEEK_SET);
	for ( i=0; i<cnt; ++i )
	    putshort(gsub,offsets[i]);
	fseek(gsub,here,SEEK_SET);
	dumpcoveragetable(gsub,glyphs);
	free(glyphs);
	free(offsets);
    }
}

static void dumpGSUBvrt2(FILE *gsub,SplineFont *sf,struct simplesubs *subs,struct alltabs *at) {
    SplineChar **glyphs;
    int cnt;
    int32 coverage_pos, end;

    for ( cnt = 0; subs[cnt].orig!=0; ++cnt );
    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
    for ( cnt = 0; subs[cnt].orig!=0; ++cnt )
	glyphs[cnt] = subs[cnt].origsc;
    glyphs[cnt] = NULL;

    putshort(gsub,2);		/* glyph list format */
    coverage_pos = ftell(gsub);
    putshort(gsub,0);		/* offset to coverage table */
    putshort(gsub,cnt);
    for ( cnt = 0; subs[cnt].orig!=0; ++cnt )
	putshort(gsub,subs[cnt].replacement);
    end = ftell(gsub);
    fseek(gsub,coverage_pos,SEEK_SET);
    putshort(gsub,end-coverage_pos+2);
    fseek(gsub,end,SEEK_SET);
    dumpcoveragetable(gsub,glyphs);
    free(subs);
    free(glyphs);
}

static void dumpg___info(struct alltabs *at, SplineFont *sf,int is_gpos) {
    /* Dump out either a gpos or a gsub table. gpos handles kerns, gsub ligs */
    int mask[3];
    struct simplesubs *subs, *ls, *arabini, *arabmed, *arabfin, *arabiso;
    struct simplesubs *sublist[8];
    int i,j,script_cnt=0, lookup_cnt=0, l2r=0, r2l=0, han=0, hansimple=0;
    int latinsimple=0, arabsimple=0;
    int script_size;
    int32 r2l_pos;
    int g___len;
    FILE *g___;
    void (*dumpg___data)(FILE *,SplineFont *, int,struct alltabs *) =
	    is_gpos?dumpgposkerndata:dumpgsubligdata;
    int tag = is_gpos?CHR('k','e','r','n'):CHR('l','i','g','a');
    int tags[10], r2lflag[10];

    memset(mask,0,sizeof(mask));
    FeatureScriptMask(sf,is_gpos, mask);
    if ( is_gpos ) {
	subs = ls = arabini = arabmed = arabfin = arabiso = NULL;
    } else {
	subs = VerticalRotationGlyphs(sf);
	ls = longs(sf);
	arabini = arabic_forms(sf,0);
	arabmed = arabic_forms(sf,1);
	arabfin = arabic_forms(sf,2);
	arabiso = arabic_forms(sf,3);
	j=0;
	if ( subs!=NULL ) sublist[j++] = subs;
	if ( ls!=NULL ) { sublist[j++] = ls; sublist[j++] = longs(sf); }
	if ( arabini!=NULL ) sublist[j++] = arabini;
	if ( arabmed!=NULL ) sublist[j++] = arabmed;
	if ( arabfin!=NULL ) sublist[j++] = arabfin;
	if ( arabiso!=NULL ) sublist[j++] = arabiso;
    }
    if ( mask[0]==0 && mask[1]==0 && mask[2]==0 &&	/* no recognizable */
	    subs==NULL && ls==NULL &&			/*  entries */
	    arabini==NULL && arabmed==NULL && arabfin==NULL && arabiso==NULL )
return;
    if ( mask[script_cjk>>5] & (1<<(script_cjk&0x1f)) )
	hansimple=1;
    if ( mask[script_arabic>>5] & (1<<(script_arabic&0x1f)) )
	arabsimple=1;
    if ( mask[script_latin>>5] & (1<<(script_latin&0x1f)) )
	latinsimple=1;

    for ( i=0; scripts[i][0]!=-1; ++i ) {
	if ( mask[i>>5]&(1<<(i&0x1f))) {
	    if ( i==script_arabic || i==script_hebrew )
		r2l = true;
	    else
		l2r = true;
	}
    }

    if ( subs!=NULL )
	mask[script_cjk>>5] |= 1<<(script_cjk&0x1f);
    if ( ls!=NULL )
	mask[script_latin>>5] |= 1<<(script_latin&0x1f);
    if ( arabini!=NULL || arabmed!=NULL || arabfin!=NULL || arabiso!=NULL )
	mask[script_arabic>>5] |= 1<<(script_arabic&0x1f);
    for ( i=0; scripts[i][0]!=-1; ++i )
	if ( mask[i>>5]&(1<<(i&0x1f)))
	    ++script_cnt;

    if ( subs!=NULL )
	han = 1;
    lookup_cnt = l2r+r2l+han;
    tags[0] = tag; tags[1] = tag;
    j=0;
    if ( l2r ) r2lflag[j++] = 0;
    if ( r2l ) r2lflag[j++] = 1;
    if ( han ) {
	tags[l2r+r2l]= CHR('v','r','t','2');
	r2lflag[j++] = 0;
    }
    if ( ls!=NULL ) { tags[lookup_cnt++] = CHR('i','n','i','t'); tags[lookup_cnt++] = CHR('m','e','d','i'); r2lflag[j++] = 0; r2lflag[j++] = 0; }
    if ( arabini!=NULL ) { tags[lookup_cnt++] = CHR('i','n','i','t'); r2lflag[j++] = true; }
    if ( arabmed!=NULL ) { tags[lookup_cnt++] = CHR('m','e','d','i'); r2lflag[j++] = true; }
    if ( arabfin!=NULL ) { tags[lookup_cnt++] = CHR('f','i','n','a'); r2lflag[j++] = true; }
    if ( arabiso!=NULL ) { tags[lookup_cnt++] = CHR('i','s','o','l'); r2lflag[j++] = true; }

    g___ = tmpfile();
    if ( is_gpos ) at->gpos = g___; else at->gsub = g___;
    putlong(g___,0x10000);		/* version number */
    putshort(g___,10);		/* offset to script table */
    script_size = 10+2+script_cnt*18;
    if ( han && hansimple )
	script_size += 2;
    if ( mask[script_latin>>5]&(1<<(script_latin&0x1f)) )
	script_size += 4*(ls!=NULL) + 2*latinsimple -2;
    if ( mask[script_arabic>>5]&(1<<(script_arabic&0x1f)) )
	script_size += 2*((arabini!=NULL) + (arabmed!=NULL) + (arabfin!=NULL) +
		(arabiso!=NULL) + arabsimple -1);
    putshort(g___,script_size);	/* offset to features table */
    putshort(g___,script_size);	/* We'll fix this up later */
	    /* offset to features table */

/* Now the scripts, first the list */
    putshort(g___,script_cnt);
    j = 2+6*script_cnt;
    for ( i=0; scripts[i][0]!=-1; ++i ) if ( mask[i>>5]&(1<<(i&0x1f))) {
	putlong(g___,scripts[i][0]);
	putshort(g___,j);
	j+=12;
	if ( i==script_cjk && han && hansimple )
	    j+=2;
	else if ( i==script_latin && ls!=NULL )
	    j+= 4+2*latinsimple-2;
	else if ( i==script_arabic )
	    j += 2*((arabini!=NULL) + (arabmed!=NULL) + (arabfin!=NULL) +
		    (arabiso!=NULL) + arabsimple -1);
    }
/* Then each script's default language */
    for ( i=0; scripts[i][0]!=-1; ++i ) if ( mask[i>>5]&(1<<(i&0x1f))) {
	putshort(g___,4);			/* Offset from here to start of default language */
	putshort(g___,0);			/* no other languages */
	putshort(g___,0);			/* Offset to something not yet defined */
	putshort(g___,0xffff);			/* No required features */
	if ( i==script_cjk && han && hansimple ) {
	    putshort(g___,2);			/* two features */
	    putshort(g___,0);				/* left to right features */
	    putshort(g___,l2r+r2l);			/* virtical feature */
	} else if ( i==script_cjk && han ) {
	    putshort(g___,1);				/* one feature */
	    putshort(g___,l2r+r2l);			/* virtical feature */
	} else if ( i==script_latin && ls!=NULL && latinsimple ) {
	    putshort(g___,3);			/* three features */
	    putshort(g___,0);				/* left to right features */
	    putshort(g___,l2r+r2l+han);			/* longs initial */
	    putshort(g___,l2r+r2l+han+1);		/* longs medial */
	} else if ( i==script_latin && ls!=NULL ) {
	    putshort(g___,2);				/* one feature */
	    putshort(g___,l2r+r2l+han);			/* longs initial */
	    putshort(g___,l2r+r2l+han+1);		/* longs medial */
	} else if ( i==script_arabic ) {
	    putshort(g___,(arabini!=NULL) + (arabmed!=NULL) + (arabfin!=NULL) +
		    (arabiso!=NULL) + arabsimple);
	    if ( arabsimple )
		putshort(g___,l2r);
	    j=l2r+r2l+han+2*(ls!=NULL);
	    if ( arabini!=NULL )
		putshort(g___,j++);
	    if ( arabmed!=NULL )
		putshort(g___,j++);
	    if ( arabfin!=NULL )
		putshort(g___,j++);
	    if ( arabiso!=NULL )
		putshort(g___,j++);
	} else {
	    putshort(g___,1);				/* one feature */
	    putshort(g___,(i!=script_arabic && i!=script_hebrew)?0:l2r );
	}
    }

/* Now the features */
    putshort(g___,lookup_cnt);			/* Number of features */
    for ( i=0; i<lookup_cnt; ++i ) {
	putlong(g___,tags[i]);			/* Feature type */
	putshort(g___,2+6*lookup_cnt+i*6);	/* offset to feature */
    }
    for ( i=0; i<lookup_cnt; ++i ) {
	putshort(g___,0);			/* No feature params */
	putshort(g___,1);			/* only one lookup */
	putshort(g___,i);			/* And it is lookup: i */
    }

/* Now the lookups */
    { int32 here = ftell(g___);
	fseek(g___,8,SEEK_SET);
	putshort(g___,here);
	fseek(g___,here,SEEK_SET);
    }
    putshort(g___,lookup_cnt);
    putshort(g___,lookup_cnt*2+2);
    r2l_pos = ftell(g___);
    for ( i=1; i<lookup_cnt; ++i )
	putshort(g___,0);		/* Don't know, have to fill in later */
    j=0;
    for ( i=0; i<lookup_cnt; ++i ) {
	putshort(g___,is_gpos?2:i<l2r+r2l?4:1);/* subtable type: Pair adjustment/Ligature/Simple replacement */
	putshort(g___,r2lflag[i]);		/* flag (right2left flag) */
	putshort(g___,1);			/* One subtable */
	putshort(g___,8);
	if ( i<l2r+r2l )
	    dumpg___data(g___,sf,r2lflag[i],at);
	else
	    dumpGSUBvrt2(g___,sf,sublist[j++],at);
	if ( i+1!=lookup_cnt ) {
	    int32 here = ftell(g___);
	    fseek(g___,r2l_pos+i*2,SEEK_SET);
	    putshort(g___,here-r2l_pos+4);
	    fseek(g___,here,SEEK_SET);
	}
    }
    g___len = ftell(g___);
    if ( g___len&2 )
	putshort(g___,0);		/* pad it */
    if ( is_gpos ) at->gposlen = g___len; else at->gsublen = g___len;
}

static void dumpgposkerns(struct alltabs *at, SplineFont *sf) {
    /* Open Type, bless its annoying little heart, doesn't store kern info */
    /*  in the kern table. Of course not, how silly of me to think it might */
    /*  be consistent. It stores it in the awful gpos table */
    dumpg___info(at, sf,true);
}

static void dumpgsub(struct alltabs *at, SplineFont *sf) {
    /* Ligatures and cjk vertical rotation replacement */
    SFLigaturePrepare(sf);
    dumpg___info(at, sf, false);
    SFLigatureCleanup(sf);
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
    putshort(at->kern,0);		/* version */
    putshort(at->kern,1);		/* number of subtables */
    putshort(at->kern,0);		/* subtable version */
    putshort(at->kern,(7+3*cnt)*sizeof(uint16)); /* subtable length */
    putshort(at->kern,1);		/* coverage, flags&format */
    putshort(at->kern,cnt);
    for ( i=1,j=0; i<=cnt; i<<=1, ++j );
    i>>=1; --j;
    putshort(at->kern,i*6);		/* binary search headers */
    putshort(at->kern,j);
    putshort(at->kern,6*(i-cnt));

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
	    putshort(at->kern,sf->chars[i]->ttf_glyph);
	    putshort(at->kern,glnum[j]);
	    putshort(at->kern,offsets[j]);
	}
    }
    at->kernlen = ftell(at->kern);
    if ( at->kernlen&2 )
	putshort(at->kern,0);		/* pad it */
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

    if ( dummy->names[ttf_copyright]==NULL || *dummy->names[ttf_copyright]=='\0' )
	dummy->names[ttf_copyright] = uc_copy(sf->copyright);
    if ( dummy->names[ttf_family]==NULL || *dummy->names[ttf_family]=='\0' )
	dummy->names[ttf_family] = uc_copy(sf->familyname);
    if ( dummy->names[ttf_subfamily]==NULL || *dummy->names[ttf_subfamily]=='\0' )
	dummy->names[ttf_subfamily] = uc_copy(SFGetModifiers(sf));
    if ( dummy->names[ttf_uniqueid]==NULL || *dummy->names[ttf_uniqueid]=='\0' ) {
	time(&now);
	tm = localtime(&now);
	sprintf( buffer, "%s : %s : %d-%d-%d", BDFFoundry?BDFFoundry:"PfaEdit 1.0",
		sf->fullname!=NULL?sf->fullname:sf->fontname,
		tm->tm_mday, tm->tm_mon, tm->tm_year+1970 );
	dummy->names[ttf_uniqueid] = uc_copy(buffer);
    }
    if ( dummy->names[ttf_fullname]==NULL || *dummy->names[ttf_fullname]=='\0' )
	dummy->names[ttf_fullname] = uc_copy(sf->fullname);
    if ( dummy->names[ttf_version]==NULL || *dummy->names[ttf_version]=='\0' )
	dummy->names[ttf_version] = uc_copy(sf->version);
    if ( dummy->names[ttf_postscriptname]==NULL || *dummy->names[ttf_postscriptname]=='\0' )
	dummy->names[ttf_postscriptname] = uc_copy(sf->fontname);
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
    putshort(at->name,0);	/* format */
    putshort(at->name,strcnt);	/* numrec */
    putshort(at->name,(3+strcnt*6)*sizeof(int16));	/* offset to strings */
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL ) {
	putshort(at->name,1);	/* apple */
	putshort(at->name,0);	/*  */
	putshort(at->name,0);	/* Roman alphabet */
	putshort(at->name,i);
	putshort(at->name,u_strlen(dummy.names[i]));
	putshort(at->name,pos);
	pos += u_strlen(dummy.names[i])+1;
    }
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL ) {
	putshort(at->name,0);	/* apple unicode */
	putshort(at->name,3);	/* 3 => Unicode 2.0 semantics */ /* 0 ("default") is also a reasonable value */
	putshort(at->name,0);	/*  */
	putshort(at->name,i);
	putshort(at->name,2*u_strlen(dummy.names[i]));
	putshort(at->name,pos);
	posses[i] = pos;
	pos += 2*u_strlen(dummy.names[i])+2;
    }
    for ( i=0; i<ttf_namemax; ++i ) if ( dummy.names[i]!=NULL ) {
	putshort(at->name,3);	/* MS platform */
	putshort(at->name,1);	/* not symbol */
	putshort(at->name,0x0409);	/* american english language */
	putshort(at->name,i);
	putshort(at->name,2*u_strlen(dummy.names[i]));
	putshort(at->name,posses[i]);
    }

    for ( cur=sf->names; cur!=NULL; cur=cur->next ) if ( cur->lang!=0x409 ) {
	for ( i=0; i<ttf_namemax; ++i ) if ( cur->names[i]!=NULL ) {
	    putshort(at->name,3);	/* MS platform */
	    putshort(at->name,1);	/* not symbol */
	    putshort(at->name,cur->lang);/* american english language */
	    putshort(at->name,i);
	    putshort(at->name,2*u_strlen(cur->names[i]));
	    putshort(at->name,pos);
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

    putlong(at->post,format!=ff_otf && format!=ff_otfcid?0x00020000:0x00030000);	/* formattype */
    putfixed(at->post,sf->italicangle);
    putshort(at->post,sf->upos);
    putshort(at->post,sf->uwidth);
    putlong(at->post,at->isfixed);
    putlong(at->post,0);		/* no idea about memory */
    putlong(at->post,0);		/* no idea about memory */
    putlong(at->post,0);		/* no idea about memory */
    putlong(at->post,0);		/* no idea about memory */
    if ( format!=ff_otf && format!=ff_otfcid ) {
	putshort(at->post,at->maxp.numGlyphs);

	putshort(at->post,0);		/* glyph 0 is named .notdef */
	putshort(at->post,1);		/* glyphs 1&2 are tab and cr */
	putshort(at->post,2);		/* or something */
	i=1;
	if ( sf->chars[0]!=NULL && strcmp(sf->chars[0]->name,".notdef")!=0 )
	    i=0;
	for ( pos=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 ) {
	    if ( sf->chars[i]->unicodeenc<128 && sf->chars[i]->unicodeenc!=-1 )
		putshort(at->post,sf->chars[i]->unicodeenc-32+3);
	    else if ( strcmp(sf->chars[i]->name,".notdef")==0 )
		putshort(at->post,0);
	    else {
		for ( j=127-32+3; j<258; ++j )
		    if ( strcmp(sf->chars[i]->name,ttfstandardnames[j])==0 )
		break;
		if ( j!=258 )
		    putshort(at->post,j);
		else {
		    putshort(at->post,pos+258);
		    ++pos;
		}
	    }
	}
	if ( pos!=0 ) {
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 ) {
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

static void SetTo(uint32 *avail,uint16 *sfind,int to,int from) {
    avail[to] = avail[from];
    if ( sfind!=NULL ) sfind[to] = sfind[from];
}

static int Needs816Enc(struct alltabs *at, SplineFont *sf) {
    int i, j, complained, pos, k, len, subheadindex, jj;
    uint16 table[256];
    struct subhead subheads[128];
    uint16 *glyphs;
    uint16 tempglyphs[256];
    uint32 macpos;
    int enccnt;
    /* This only works for big5, johab */
    int base, lbase, basebound, subheadcnt, planesize, plane0size;
    int base2, base2bound;

    if ( sf->cidmaster!=NULL || sf->subfontcnt!=0 )
return( false );
    base2 = -1; base2bound = -2;
    if ( sf->encoding_name==em_big5 ) {
	base = 0xa1;
	basebound = 0xf9;	/* wcl-02.ttf's cmap claims to go up to fc, but everything after f9 is invalid (according to what I know of big5, f9 should be the end) */
	lbase = 0x40;
	subheadcnt = basebound-base+1;
	planesize = 191;
    } else if ( sf->encoding_name==em_wansung ) {
	base = 0xa1;
	basebound = 0xfd;
	lbase = 0xa1;
	subheadcnt = basebound-base+1;
	planesize = 0xfe - lbase +1;
    } else if ( sf->encoding_name==em_johab ) {
	base = 0x84;
	basebound = 0xf9;
	lbase = 0x31;
	subheadcnt = basebound-base+1;
	planesize = 0xfe -0x31+1;	/* Stupid gcc bug, thinks 0xfe- is ambiguous (exponant) */
    } else if ( sf->encoding_name==em_sjis ) {
	base = 129;
	basebound = 159;
	lbase = 64;
	planesize = 252 - lbase +1;
	base2 = 0xe0;
	/* SJIS supports "user defined characters" between 0xf040 and 0xfcfc */
	/*  there probably won't be any, but allow space for them if there are*/
	for ( base2bound=0xfc00; base2bound>0xefff; --base2bound )
	    if ( base2bound<sf->charcnt && SCWorthOutputting(sf->chars[base2bound]))
	break;
	base2bound >>= 8;
	subheadcnt = basebound-base + 1 + base2bound-base2 + 1;
    } else {
	fprintf( stderr, "Unsupported 8/16 encoding %d\n", sf->encoding_name );
return( false );
    }
    plane0size = base2==-1? base : base2;
    i=0;
    if ( base2!=-1 ) {
	for ( i=basebound; i<base2 && i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i]))
	break;
	if ( i==base2 || i==sf->charcnt )
	    i = 0;
    }
    if ( i==0 )
	for ( i=0; i<base && i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i]))
	break;
    if ( i==base || i==sf->charcnt )
return( false );		/* Doesn't have the single byte entries */
	/* Can use the normal 16 bit encoding scheme */

    if ( base2!=-1 ) {
	for ( i=base; i<=basebound && i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i])) {
		GWidgetErrorR(_STR_BadEncoding,_STR_ExtraneousSingleByte,i);
	break;
	    }
	if ( i==basebound+1 )
	    for ( i=base2; i<256 && i<sf->charcnt; ++i )
		if ( SCWorthOutputting(sf->chars[i])) {
		    GWidgetErrorR(_STR_BadEncoding,_STR_ExtraneousSingleByte,i);
	    break;
		}
    } else {
	for ( i=base; i<256 && i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i])) {
		GWidgetErrorR(_STR_BadEncoding,_STR_ExtraneousSingleByte,i);
	break;
	    }
    }
    for ( i=256; i<(base<<8) && i<sf->charcnt; ++i )
	if ( SCWorthOutputting(sf->chars[i])) {
	    GWidgetErrorR(_STR_BadEncoding,_STR_OutOfEncoding,i);
    break;
	}
    if ( i==(base<<8) && base2==-1 )
	for ( i=((basebound+1)<<8); i<0x10000 && i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i])) {
		GWidgetErrorR(_STR_BadEncoding,_STR_OutOfEncoding,i);
	break;
	    }

    memset(table,'\0',sizeof(table));
    for ( i=base; i<=basebound; ++i )
	table[i] = 8*(i-base+1);
    for ( i=base2; i<=base2bound; ++i )
	table[i] = 8*(i-base2+basebound-base+1+1);
    memset(subheads,'\0',sizeof(subheads));
    subheads[0].first = 0; subheads[0].cnt = plane0size;
    for ( i=1; i<subheadcnt+1; ++i ) {
	subheads[i].first = lbase;
	subheads[i].cnt = planesize;
    }
    glyphs = gcalloc(subheadcnt*planesize+plane0size,sizeof(uint16));
    subheads[0].rangeoff = 0;
    for ( i=0; i<plane0size && i<sf->charcnt; ++i )
	if ( sf->chars[i]!=NULL && SCDuplicate(sf->chars[i])->ttf_glyph!=-1)
	    glyphs[i] = SCDuplicate(sf->chars[i])->ttf_glyph;
    
    pos = 1;

    complained = false;
    subheadindex = 1;
    for ( jj=0; jj<2 || (base2==-1 && jj<1); ++jj )
	    for ( j=((jj==0?base:base2)<<8); j<=((jj==0?basebound:base2bound)<<8); j+= 0x100 ) {
	for ( i=0; i<lbase; ++i )
	    if ( !complained && SCWorthOutputting(sf->chars[i+j])) {
		GWidgetErrorR(_STR_BadEncoding,_STR_NotNormallyEncoded,i+j);
		complained = true;
	    }
	if ( sf->encoding_name==em_big5 ) {
	    /* big5 has a gap here. Does johab? */
	    for ( i=0x7f; i<0xa1; ++i )
		if ( !complained && SCWorthOutputting(sf->chars[i+j])) {
		    GWidgetErrorR(_STR_BadEncoding,_STR_NotNormallyEncoded,i+j);
		    complained = true;
		}
	}
	memset(tempglyphs,0,sizeof(tempglyphs));
	for ( i=0; i<planesize; ++i )
	    if ( sf->chars[j+lbase+i]!=NULL && SCDuplicate(sf->chars[j+lbase+i])->ttf_glyph!=-1 )
		tempglyphs[i] = SCDuplicate(sf->chars[j+lbase+i])->ttf_glyph;
	for ( i=1; i<pos; ++i ) {
	    int delta = 0;
	    for ( k=0; k<planesize; ++k )
		if ( tempglyphs[k]==0 && glyphs[plane0size+(i-1)*planesize+k]==0 )
		    /* Still matches */;
		else if ( delta==0 )
		    delta = (uint16) (tempglyphs[k]-glyphs[plane0size+(i-1)*planesize+k]);
		else if ( tempglyphs[k]==(uint16) (glyphs[plane0size+(i-1)*planesize+k]+delta) )
		    /* Still matches */;
		else
	    break;
	    if ( k==planesize ) {
		subheads[subheadindex].delta = delta;
		subheads[subheadindex].rangeoff = plane0size+(i-1)*planesize;
	break;
	    }
	}
	if ( subheads[subheadindex].rangeoff==0 ) {
	    memcpy(glyphs+(pos-1)*planesize+plane0size,tempglyphs,planesize*sizeof(uint16));
	    subheads[subheadindex].rangeoff = plane0size+(pos++-1)*planesize ;
	}
	++subheadindex;
    }

    /* fixup offsets */
    /* my rangeoffsets are indexes into the glyph array. That's nice and */
    /*  simple. Unfortunately ttf says they are offsets from the current */
    /*  location in the file (sort of) so we now fix them up. */
    for ( i=0; i<subheadcnt+1; ++i )
	subheads[i].rangeoff = subheads[i].rangeoff*sizeof(uint16) +
		(subheadcnt-i)*sizeof(struct subhead) + sizeof(uint16);

    /* Two/Three encoding table pointers, one for ms, one for mac, one for mac roman */
    enccnt = 3;
    if ( sf->encoding_name==em_johab )
	enccnt = 2;

    len = 3*sizeof(uint16) + 256*sizeof(uint16) + (subheadcnt+1)*sizeof(struct subhead) +
	    ((pos-1)*planesize+plane0size)*sizeof(uint16);
    macpos = 2*sizeof(uint16)+enccnt*(2*sizeof(uint16)+sizeof(uint32))+len;

    /* Encodings are supposed to be ordered by platform, then by specific */
    putshort(at->cmap,0);		/* version */
    putshort(at->cmap,enccnt);		/* num tables */

    putshort(at->cmap,1);		/* mac platform */
    putshort(at->cmap,0);		/* plat specific enc, script=roman */
    putlong(at->cmap,macpos);		/* offset from tab start to sub tab start */
    
    if ( enccnt==3 ) {
	/* big mac table, just a copy of the ms table */
	putshort(at->cmap,1);
	putshort(at->cmap,
	    sf->encoding_name==em_sjis?   1 :	/* Japanese */
	    sf->encoding_name==em_wansung? 3 :	/* Korean */
	    /*sf->encoding_name==em_big5?*/ 2 );/* Chinese, Traditional */
	putlong(at->cmap,2*sizeof(uint16)+enccnt*(2*sizeof(uint16)+sizeof(uint32)));
    }

    putshort(at->cmap,3);		/* ms platform */
    putshort(at->cmap,
	sf->encoding_name==em_sjis? 2 :		/* SJIS */
	sf->encoding_name==em_big5? 4 :		/* Big5 */
	sf->encoding_name==em_wansung? 5 :	/* Wansung */
	 6 );					/* Johab */
    putlong(at->cmap,2*sizeof(uint16)+enccnt*(2*sizeof(uint16)+sizeof(uint32)));
					/* offset from tab start to sub tab start */

    putshort(at->cmap,2);		/* 8/16 format */
    putshort(at->cmap,len);		/* Subtable length */
    putshort(at->cmap,0);		/* version/language, not meaningful in ms systems */
    for ( i=0; i<256; ++i )
	putshort(at->cmap,table[i]);
    for ( i=0; i<subheadcnt+1; ++i ) {
	putshort(at->cmap,subheads[i].first);
	putshort(at->cmap,subheads[i].cnt);
	putshort(at->cmap,subheads[i].delta);
	putshort(at->cmap,subheads[i].rangeoff);
    }
    for ( i=0; i<(pos-1)*planesize+plane0size; ++i )
	putshort(at->cmap,glyphs[i]);
    free(glyphs);

    /* Mac table just the first 256 entries */
    if ( ftell(at->cmap)!=macpos )
	GDrawIError("Mac cmap sub-table not at right place is %d should be %d", ftell(at->cmap), macpos );
    memset(table,0,sizeof(table));
    for ( i=0; i<256 && i<sf->charcnt; ++i )
	if ( sf->chars[i]!=NULL && SCDuplicate(sf->chars[i])->ttf_glyph!=-1 &&
		SCDuplicate(sf->chars[i])->ttf_glyph<256 )
	    table[i] = SCDuplicate(sf->chars[i])->ttf_glyph;
return( true );
}

static FILE *NeedsUCS4Table(SplineFont *sf,int *ucs4len) {
    int i=0,j,group;
    FILE *format12;
    SplineChar *sc;
    
    if ( sf->encoding_name==em_unicode4 ) {
	for ( i=0x10000; i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i]))
	break;
    } else if ( sf->encoding_name>=em_unicodeplanes && sf->encoding_name<=em_unicodeplanesmax ) {
	for ( i=0; i<sf->charcnt; ++i )
	    if ( SCWorthOutputting(sf->chars[i]))
	break;
    } else
return( NULL );
	if ( i>=sf->charcnt )
return(NULL);

    format12 = tmpfile();
    if ( format12==NULL )
return( NULL );

    putshort(format12,12);		/* Subtable format */
    putshort(format12,0);		/* padding */
    putlong(format12,0);		/* Length, we'll come back to this */
    putlong(format12,0);		/* language */
    putlong(format12,0);		/* Number of groups, we'll come back to this */

    group = 0;
    for ( i=0; i<sf->charcnt; ++i ) if ( SCWorthOutputting(sf->chars[i]) && sf->chars[i]->unicodeenc!=-1 ) {
	sc = SCDuplicate(sf->chars[i]);
	for ( j=i+1; j<sf->charcnt && SCWorthOutputting(sf->chars[j]) &&
		sf->chars[j]->unicodeenc!=-1 &&
		SCDuplicate(sf->chars[j])->ttf_glyph==sc->ttf_glyph+j-i; ++j );
	--j;
	putlong(format12,i);		/* start char code */
	putlong(format12,j);		/* end char code */
	putlong(format12,sc->ttf_glyph);
	++group;
    }
    *ucs4len = ftell(format12);
    fseek(format12,4,SEEK_SET);
    putlong(format12,*ucs4len);		/* Length, I said we'd come back to it */
    putlong(format12,0);		/* language */
    putlong(format12,group);		/* Number of groups */
return( format12 );
}

static int figureencoding(SplineFont *sf,int i) {
    switch ( sf->encoding_name ) {
      default:				/* Unicode */
	  if ( sf->chars[i]->unicodeenc>=65536 )	/* format 4 doesn't support 4byte encodings, we have an additional format 12 table for that */
return( -1 );

return( sf->chars[i]->unicodeenc );
      case em_big5:			/* Taiwan, Hong Kong */
      case em_johab:			/* Korea */
      case em_wansung:			/* Korea */
      case em_sjis:			/* Japan */
return( i );
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
    uint16 *sfind = NULL;
    int i,j,k,l,charcnt,enccnt;
    int segcnt, cnt=0, delta, rpos;
    struct cmapseg { uint16 start, end; uint16 delta; uint16 rangeoff; } *cmapseg;
    uint16 *ranges;
    char table[256];
    SplineFont *sf = _sf;
    SplineChar *sc, notdef, nonmarkingreturn;
    extern int greekfixup;
    int alreadyprivate = false;

    if ( sf->subfontcnt==0 && sf->chars[0]==NULL ) {	/* Encode the default notdef char at 0 */
	memset(&notdef,0,sizeof(notdef));
	notdef.unicodeenc = -1;
	notdef.name = ".notdef";
	notdef.parent = sf;
	sf->chars[0] = &notdef;
    }
    if ( sf->subfontcnt==0 && sf->chars[13]==NULL ) {	/* Encode the default notdef char at 0 */
	memset(&nonmarkingreturn,0,sizeof(notdef));
	nonmarkingreturn.unicodeenc = 13;
	nonmarkingreturn.name = "nonmarkingreturn";
	nonmarkingreturn.parent = sf;
	nonmarkingreturn.ttf_glyph = 2;
	sf->chars[13] = &nonmarkingreturn;
    }

    at->cmap = tmpfile();

    /* MacRoman encoding table */ /* Not going to bother with making this work for cid fonts */
    memset(table,'\0',sizeof(table));
    for ( i=0; i<sf->charcnt && i<256; ++i ) {
	sc = SCDuplicate(SFFindExistingCharMac(sf,unicode_from_mac[i]));
	if ( sc!=NULL && sc->ttf_glyph!=-1 && sc->ttf_glyph<256 )
	    table[i] = sc->ttf_glyph;
    }
    table[0] = table[8] = table[13] = table[29] = 1;
    table[9] = table[32]==0 ? 1 : table[32];

    if ( format==ff_ttfsym || sf->encoding_name==em_symbol ) {
	int acnt=0, pcnt=0;
	int space = table[9];
	for ( i=0; i<sf->charcnt; ++i ) {
	    if ( sf->chars[i]!=&notdef && sf->chars[i]!=&nonmarkingreturn &&
		    sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph!=-1 &&
		    !SCIsNotdef(sf->chars[i],-1) && i<=0xffff ) {
		if ( i>=0xf000 && i<=0xf0ff )
		    ++acnt;
		else if ( i>=0x20 && i<=0xff )
		    ++pcnt;
	    }
	}
	alreadyprivate = acnt>pcnt;
	memset(table,'\0',sizeof(table));
	if ( !alreadyprivate ) {
	    for ( i=0; i<sf->charcnt && i<256; ++i ) {
		sc = SCDuplicate(sf->chars[i]);
		if ( sc!=NULL && sc->ttf_glyph!=-1 && sc->ttf_glyph<256 )
		    table[sf->chars[i]->enc] = sc->ttf_glyph;
	    }
	} else {
	    for ( i=0xf000; i<=0xf0ff && i<sf->charcnt; ++i ) {
		sc = SCDuplicate(sf->chars[i]);
		if ( sc!=NULL && sc->ttf_glyph!=-1 &&
			sc->ttf_glyph<256 )
		    table[sf->chars[i]->enc-0xf000] = sc->ttf_glyph;
	    }
	}
	table[0] = table[8] = table[13] = table[29] = 1;
	table[9] = space;
	/* if the user has read in a ttf symbol file then it will already have */
	/*  the right private use encoding, and we don't want to mess it up. */
	/*  The alreadyprivate flag should detect this case */
	if ( !alreadyprivate ) {
	    for ( i=0; i<sf->charcnt && i<256; ++i ) {
		if ( sf->chars[i]!=&notdef && sf->chars[i]!=&nonmarkingreturn &&
			sf->chars[i]!=NULL && !SCIsNotdef(sf->chars[i],-1)) {
		    sf->chars[i]->enc = sf->chars[i]->unicodeenc;
		    sf->chars[i]->unicodeenc = 0xf000 + i;
		}
	    }
	    for ( ; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
		sf->chars[i]->enc = sf->chars[i]->unicodeenc;
		sf->chars[i]->unicodeenc = -1;
	    }
	}
    }
    if ( format!=ff_ttfsym &&
		   (sf->encoding_name==em_big5 ||
		    sf->encoding_name==em_johab ||
		    sf->encoding_name==em_sjis ||
		    sf->encoding_name==em_wansung ) && Needs816Enc(at,sf)) {
	/* All Done */;
    } else {
	uint32 *avail = galloc(65536*sizeof(uint32));
	int ucs4len=0;
	FILE *format12 = NeedsUCS4Table(sf,&ucs4len);
	int hasmac, cjkenc;
	long ucs4pos, mspos;
	memset(avail,0xff,65536*sizeof(uint32));
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
			sf->chars[i]!=NULL && SCDuplicate(sf->chars[i])->ttf_glyph!=-1 &&
			(l = figureencoding(sf,i))!=-1 ) {
		    avail[l] = i;
		    if ( sfind!=NULL ) sfind[l] = k;
		    ++cnt;
	    break;
		}
		++k;
	    } while ( k<_sf->subfontcnt );
	}
	if ( _sf->encoding_name!=em_jis208 && _sf->encoding_name!=em_ksc5601 &&
		_sf->encoding_name!=em_sjis && _sf->encoding_name!=em_wansung &&
		_sf->encoding_name!=em_big5 && _sf->encoding_name!=em_johab &&
		!greekfixup ) {
	    /* Duplicate glyphs for greek */	/* Only meaningful if unicode */
	    if ( avail[0xb5]==0xffffffff && avail[0x3bc]!=0xffffffff )
		SetTo(avail,sfind,0xb5,0x3bc);
	    else if ( avail[0x3bc]==0xffffffff && avail[0xb5]!=0xffffffff )
		SetTo(avail,sfind,0x3bc,0xb5);
	    if ( avail[0x394]==0xffffffff && avail[0x2206]!=0xffffffff )
		SetTo(avail,sfind,0x394,0x2206);
	    else if ( avail[0x2206]==0xffffffff && avail[0x394]!=0xffffffff )
		SetTo(avail,sfind,0x2206,0x394);
	    if ( avail[0x3a9]==0xffffffff && avail[0x2126]!=0xffffffff )
		SetTo(avail,sfind,0x3a9,0x2126);
	    else if ( avail[0x2126]==0xffffffff && avail[0x3a9]!=0xffffffff )
		SetTo(avail,sfind,0x2126,0x3a9);
	}

	j = -1;
	for ( i=segcnt=0; i<65536; ++i ) {
	    if ( avail[i]!=0xffffffff && j==-1 ) {
		j=i;
		++segcnt;
	    } else if ( j!=-1 && avail[i]==0xffffffff )
		j = -1;
	}
	cmapseg = gcalloc(segcnt+1,sizeof(struct cmapseg));
	ranges = galloc(cnt*sizeof(int16));
	j = -1;
	for ( i=segcnt=0; i<65536; ++i ) {
	    if ( avail[i]!=0xffffffff && j==-1 ) {
		j=i;
		cmapseg[segcnt].start = j;
		++segcnt;
	    } else if ( j!=-1 && avail[i]==0xffffffff ) {
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
		sc = SCDuplicate(sc);
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
		    sc = SCDuplicate(sc);
		    ranges[rpos++] = sc->ttf_glyph;
		}
	    }
	}
	free(avail);
	if ( _sf->subfontcnt!=0 ) free(sfind);

	/* Two/Three/Four encoding table pointers, one for ms, one for mac */
	/*  usually one for mac big, just a copy of ms */
	/* plus we may have a format12 encoding for ucs4, mac doesn't support */
	hasmac = 1;
	enccnt = 3;
	if ( sf->encoding_name==em_johab ) {
	    enccnt = 2;
	    hasmac = 0;		/* Don't know what johab looks like on mac */
	} else if ( format12!=NULL )
	    enccnt = 4;
	else if ( format==ff_ttfsym || sf->encoding_name==em_symbol ) {
	    enccnt = 2;
	    hasmac = 0;
	}
	putshort(at->cmap,0);		/* version */
	putshort(at->cmap,enccnt);	/* num tables */

	mspos = 2*sizeof(uint16)+enccnt*(2*sizeof(uint16)+sizeof(uint32));
	ucs4pos = mspos+(8+4*segcnt+rpos)*sizeof(int16);
	cjkenc = 
		sf->encoding_name==em_ksc5601 ||/* Wansung, korean */
		sf->encoding_name==em_wansung ||/* Wansung, korean */
		sf->encoding_name==em_jis208 ||	/* SJIS */
		sf->encoding_name==em_sjis ||	/* SJIS */
		sf->encoding_name==em_big5 ||	/* Big5, traditional Chinese */
		sf->encoding_name==em_johab;	/* Korean */
	if ( !cjkenc && hasmac ) {
	    /* big mac table, just a copy of the ms table */
	    putshort(at->cmap,0);	/* mac unicode platform */
	    putshort(at->cmap,3);	/* Unicode 2.0 */
	    putlong(at->cmap,mspos);
	}
	putshort(at->cmap,1);		/* mac platform */
	putshort(at->cmap,0);		/* plat specific enc, script=roman */
	    /* Even the symbol font on the mac claims a mac roman encoding */
	    /* although it actually contains a symbol encoding. There is an*/
	    /* "RSymbol" language listed for Mac (specific=8) but it isn't used*/
	putlong(at->cmap,ucs4pos+ucs4len);	/* offset from tab start to sub tab start */
	if ( cjkenc && hasmac ) {
	    /* big mac table, just a copy of the ms table */
	    putshort(at->cmap,1);	/* mac platform */
	    putshort(at->cmap,
		sf->encoding_name==em_jis208? 1 :	/* SJIS */
		sf->encoding_name==em_sjis? 1 :		/* SJIS */
		sf->encoding_name==em_ksc5601? 3 :	/* Korean */
		sf->encoding_name==em_wansung? 3 :	/* Korean */
		2 );					/* Big5 */
	    putlong(at->cmap,mspos);
	}
	putshort(at->cmap,3);		/* ms platform */
	putshort(at->cmap,		/* plat specific enc */
		format==ff_ttfsym || sf->encoding_name==em_symbol ? 0 :
		sf->encoding_name==em_ksc5601 ? 5 :	/* Wansung, korean */
		sf->encoding_name==em_wansung ? 5 :	/* Wansung, korean */
		sf->encoding_name==em_jis208 ? 2 :	/* SJIS */
		sf->encoding_name==em_sjis ? 2 :	/* SJIS */
		sf->encoding_name==em_big5 ? 4 :	/* Big5, traditional Chinese */
		sf->encoding_name==em_johab ? 6 :	/* Korean */
		 1 );					/* Unicode */
	putlong(at->cmap,mspos);		/* offset from tab start to sub tab start */
	if ( format12!=NULL ) {
	    putshort(at->cmap,3);		/* ms platform */
	    putshort(at->cmap,10);		/* plat specific enc, ucs4 */
	    putlong(at->cmap,ucs4pos);		/* offset from tab start to sub tab start */
	}

	putshort(at->cmap,4);		/* format */
	putshort(at->cmap,(8+4*segcnt+rpos)*sizeof(int16));
	putshort(at->cmap,0);		/* language/version */
	putshort(at->cmap,2*segcnt);	/* segcnt */
	for ( j=0,i=1; i<=segcnt; i<<=1, ++j );
	putshort(at->cmap,i);		/* 2*2^floor(log2(segcnt)) */
	putshort(at->cmap,j-1);
	putshort(at->cmap,2*segcnt-i);
	for ( i=0; i<segcnt; ++i )
	    putshort(at->cmap,cmapseg[i].end);
	putshort(at->cmap,0);
	for ( i=0; i<segcnt; ++i )
	    putshort(at->cmap,cmapseg[i].start);
	for ( i=0; i<segcnt; ++i )
	    putshort(at->cmap,cmapseg[i].delta);
	for ( i=0; i<segcnt; ++i )
	    putshort(at->cmap,cmapseg[i].rangeoff);
	for ( i=0; i<rpos; ++i )
	    putshort(at->cmap,ranges[i]);
	free(ranges);
	free(cmapseg);

	if ( format12!=NULL ) {
	    if ( !ttfcopyfile(at->cmap,format12,ucs4pos)) at->error = true;
	}
    }

    /* Mac table */
    putshort(at->cmap,0);		/* format */
    putshort(at->cmap,262);	/* length = 256bytes + 6 header bytes */
    putshort(at->cmap,0);		/* language = english */
    for ( i=0; i<256; ++i )
	putc(table[i],at->cmap);

    at->cmaplen = ftell(at->cmap);
    if ( (at->cmaplen&2)!=0 )
	putshort(at->cmap,0);

    if ( format==ff_ttfsym || sf->encoding_name==em_symbol ) {
	if ( !alreadyprivate ) {
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=&notdef && sf->chars[i]!=&nonmarkingreturn && sf->chars[i]!=NULL ) {
		sf->chars[i]->unicodeenc = sf->chars[i]->enc;
		sf->chars[i]->enc = i;
	    }
	}
    }
    if ( sf->subfontcnt==0 && sf->chars[0]==&notdef )
	sf->chars[0] = NULL;
    if ( sf->subfontcnt==0 && sf->chars[13]==&nonmarkingreturn )
	sf->chars[13] = NULL;
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

static void AssignTTFGlyph(SplineFont *sf,int32 *bsizes) {
    int i, tg, j;
    BDFFont *bdf;

    tg = 3;
    /* The first three glyphs are magic, glyph 0 might appear in the font */
    /*  but glyph 1,2 never do (they correspond to NUL and CR respectively) */
    /*  We generate them automagically */

    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( j=0; bsizes[j]!=0 && ((bsizes[j]&0xffff)!=bdf->pixelsize || (bsizes[j]>>16)!=BDFDepth(bdf)); ++j );
	if ( bsizes[j]==0 )
    continue;
	for ( i=0; i<bdf->charcnt; ++i ) if ( !IsntBDFChar(bdf->chars[i]) )
	    sf->chars[i]->ttf_glyph = 0;
    }
    /* If we are to use glyph 0, then it will already have the right ttf_glyph*/
    /*  (0), while if we aren't to use it, it again will be right (-1) */
    for ( i=1; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->ttf_glyph==0 )
	sf->chars[i]->ttf_glyph = tg++;
}
    
static void initTables(struct alltabs *at, SplineFont *sf,enum fontformat format,
	int32 *bsizes, enum bitmapformat bf) {
    int i, j, pos;
    BDFFont *bdf;

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	sf->chars[i]->ttf_glyph = -1;

    SFDefaultOS2Info(&sf->pfminfo,sf,sf->fontname);

    memset(at,'\0',sizeof(struct alltabs));
    at->msbitmaps = bf==bf_ttf_ms;
    if ( bf!=bf_ttf_ms && bf!=bf_ttf_apple && bf!=bf_sfnt_dfont)
	bsizes = NULL;
    if ( bsizes!=NULL ) {
	for ( i=j=0; bsizes[i]!=0; ++i ) {
	    for ( bdf=sf->bitmaps; bdf!=NULL && (bdf->pixelsize!=(bsizes[i]&0xffff) || BDFDepth(bdf)!=(bsizes[i]>>16)); bdf=bdf->next );
	    if ( bdf!=NULL )
		bsizes[j++] = bsizes[i];
	}
	bsizes[j] = 0;
	for ( i=0; bsizes[i]!=0; ++i );
	at->gi.strikecnt = i;
	if ( i==0 ) bsizes=NULL;
    }

    at->maxp.version = 0x00010000;
    if ( format==ff_otf || format==ff_otfcid || format==ff_none )
	at->maxp.version = 0x00005000;
    at->maxp.maxZones = 2;		/* 1 would probably do, don't use twilight */
    at->maxp.maxFDEFs = 1;		/* Not even 1 */
    at->maxp.maxStorage = 1;		/* Not even 1 */
    at->maxp.maxStack = 64;		/* A guess, it's probably more like 8 */
    at->gi.maxp = &at->maxp;
    if ( format==ff_otf )
	dumptype2glyphs(sf,at);
    else if ( format==ff_otfcid )
	dumpcidglyphs(sf,at);
    else if ( format==ff_none ) {
	AssignTTFGlyph(sf,bsizes);
	dumpcffhmtx(at,sf,true);
    } else
	dumpglyphs(sf,&at->gi);
    if ( bsizes!=NULL )
	ttfdumpbitmap(sf,at,bsizes);
    at->head.xmin = at->gi.xmin;
    at->head.ymin = at->gi.ymin;
    at->head.xmax = at->gi.xmax;
    at->head.ymax = at->gi.ymax;
    sethead(&at->head,sf);
    sethhead(&at->hhead,&at->vhead,at,sf);
    setvorg(&at->vorg,sf);
    setos2(&at->os2,at,sf,format);	/* should precede kern/ligature output */
    dumpnames(at,sf);
    if ( at->gi.glyph_len<0x20000 )
	at->head.locais32 = 0;
    if ( format!=ff_otf && format!=ff_otfcid && format!=ff_none )
	redoloca(at);
    redohead(at);
    redohhead(at,false);
    if ( sf->hasvmetrics ) {
	redohhead(at,true);
	redovorg(at);		/* I know, VORG is only meaningful in a otf font and I dump it out in ttf too. Well, it will help ME read the font back in, and it won't bother anyone else. So there. */
    }
    redomaxp(at,format);
    if ( format==ff_otf || format==ff_otfcid )
	dumpgposkerns(at,sf);
    else
	dumpkerns(at,sf);
    dumpgsub(at,sf);		/* ttf will probably ignore these, but doesn't hurt to dump them */
    redoos2(at);
    redocvt(at);
    dumppost(at,sf,format);
    dumpcmap(at,sf,format);

    if ( format==ff_otf || format==ff_otfcid ) {
	at->tabdir.version = CHR('O','T','T','O');
    } else if ( format==ff_none ) {
	at->tabdir.version = CHR('t','r','u','e');
    } else {
	at->tabdir.version = 0x00010000;
    }

    i = 0;
    pos = 0;

    if ( format==ff_otf || format==ff_otfcid ) {
	at->tabdir.tabs[i].tag = CHR('C','F','F',' ');
	at->tabdir.tabs[i].checksum = filecheck(at->cfff);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->cfflen;
	pos += ((at->cfflen+3)>>2)<<2;
    }

    if ( at->bdat!=NULL && at->msbitmaps ) {
	at->tabdir.tabs[i].tag = CHR('E','B','D','T');
	at->tabdir.tabs[i].checksum = filecheck(at->bdat);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->bdatlen;
	pos += ((at->bdatlen+3)>>2)<<2;
    }

    if ( at->bloc!=NULL && at->msbitmaps ) {
	at->tabdir.tabs[i].tag = CHR('E','B','L','C');
	at->tabdir.tabs[i].checksum = filecheck(at->bloc);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->bloclen;
	pos += ((at->bloclen+3)>>2)<<2;
    }

    if ( at->gpos!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('G','P','O','S');
	at->tabdir.tabs[i].checksum = filecheck(at->gpos);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->gposlen;
	pos += ((at->gposlen+3)>>2)<<2;
    }

    if ( at->gsub!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('G','S','U','B');
	at->tabdir.tabs[i].checksum = filecheck(at->gsub);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->gsublen;
	pos += ((at->gsublen+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('O','S','/','2');
    at->tabdir.tabs[i].checksum = filecheck(at->os2f);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->os2len;
    pos += ((at->os2len+3)>>2)<<2;

    if ( at->vorgf!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('V','O','R','G');
	at->tabdir.tabs[i].checksum = filecheck(at->vorgf);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->vorglen;
	pos += ((at->vorglen+3)>>2)<<2;
    }

    if ( at->bdat!=NULL && !at->msbitmaps ) {
	at->tabdir.tabs[i].tag = CHR('b','d','a','t');
	at->tabdir.tabs[i].checksum = filecheck(at->bdat);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->bdatlen;
	pos += ((at->bdatlen+3)>>2)<<2;
    }

    if ( format==ff_none ) {
	/* Bitmap only fonts get a bhed table rather than a head */
	at->tabdir.tabs[i].tag = CHR('b','h','e','d');
	at->tabdir.tabs[i].checksum = filecheck(at->headf);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->headlen;
	pos += ((at->headlen+3)>>2)<<2;
    }

    if ( at->bloc!=NULL && !at->msbitmaps ) {
	at->tabdir.tabs[i].tag = CHR('b','l','o','c');
	at->tabdir.tabs[i].checksum = filecheck(at->bloc);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->bloclen;
	pos += ((at->bloclen+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('c','m','a','p');
    at->tabdir.tabs[i].checksum = filecheck(at->cmap);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->cmaplen;
    pos += ((at->cmaplen+3)>>2)<<2;

    if ( format!=ff_otf && format!=ff_otfcid && format!=ff_none ) {
	at->tabdir.tabs[i].tag = CHR('c','v','t',' ');
	at->tabdir.tabs[i].checksum = filecheck(at->cvtf);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->cvtlen;
	pos += ((at->cvtlen+3)>>2)<<2;

	if ( at->gi.fpgmf!=NULL ) {
	    at->tabdir.tabs[i].tag = CHR('f','p','g','m');
	    at->tabdir.tabs[i].checksum = filecheck(at->gi.fpgmf);
	    at->tabdir.tabs[i].offset = pos;
	    at->tabdir.tabs[i++].length = at->gi.fpgmlen;
	    pos += ((at->gi.fpgmlen+3)>>2)<<2;
	}

	at->tabdir.tabs[i].tag = CHR('g','l','y','f');
	at->tabdir.tabs[i].checksum = filecheck(at->gi.glyphs);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->gi.glyph_len;
	pos += ((at->gi.glyph_len+3)>>2)<<2;
    }

    if ( format!=ff_none ) {
	at->tabdir.tabs[i].tag = CHR('h','e','a','d');
	at->tabdir.tabs[i].checksum = filecheck(at->headf);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->headlen;
	pos += ((at->headlen+3)>>2)<<2;
    }

    at->tabdir.tabs[i].tag = CHR('h','h','e','a');
    at->tabdir.tabs[i].checksum = filecheck(at->hheadf);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->hheadlen;
    pos += ((at->hheadlen+3)>>2)<<2;

    at->tabdir.tabs[i].tag = CHR('h','m','t','x');
    at->tabdir.tabs[i].checksum = filecheck(at->gi.hmtx);
    at->tabdir.tabs[i].offset = pos;
    at->tabdir.tabs[i++].length = at->gi.hmtxlen;
    pos += ((at->gi.hmtxlen+3)>>2)<<2;

    if ( at->kern!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('k','e','r','n');
	at->tabdir.tabs[i].checksum = filecheck(at->kern);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->kernlen;
	pos += ((at->kernlen+3)>>2)<<2;
    }

    if ( format!=ff_otf && format!=ff_otfcid  && format!=ff_none ) {
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

    if ( at->vheadf!=NULL ) {
	at->tabdir.tabs[i].tag = CHR('v','h','e','a');
	at->tabdir.tabs[i].checksum = filecheck(at->vheadf);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->vheadlen;
	pos += ((at->vheadlen+3)>>2)<<2;

	at->tabdir.tabs[i].tag = CHR('v','m','t','x');
	at->tabdir.tabs[i].checksum = filecheck(at->gi.vmtx);
	at->tabdir.tabs[i].offset = pos;
	at->tabdir.tabs[i++].length = at->gi.vmtxlen;
	pos += ((at->gi.vmtxlen+3)>>2)<<2;
    }
    if ( i>=sizeof(at->tabdir.tabs)/sizeof(at->tabdir.tabs[0]))
	GDrawIError("Miscalculation of number of tables needed. Up sizeof tabs array in struct tabdir" );

    at->tabdir.numtab = i;
    at->tabdir.searchRange = (i<16?8:i<32?16:32)*16;
    at->tabdir.entrySel = (i<16?3:i<32?4:5);
    at->tabdir.rangeShift = at->tabdir.numtab*16-at->tabdir.searchRange;
    for ( i=0; i<at->tabdir.numtab; ++i )
	at->tabdir.tabs[i].offset += sizeof(int32)+4*sizeof(int16) + at->tabdir.numtab*4*sizeof(int32);
}

static void dumpttf(FILE *ttf,struct alltabs *at, enum fontformat format) {
    int32 checksum;
    int i, head_index;
    /* I can't use fwrite because I (may) have to byte swap everything */

    putlong(ttf,at->tabdir.version);
    putshort(ttf,at->tabdir.numtab);
    putshort(ttf,at->tabdir.searchRange);
    putshort(ttf,at->tabdir.entrySel);
    putshort(ttf,at->tabdir.rangeShift);
    for ( i=0; i<at->tabdir.numtab; ++i ) {
	putlong(ttf,at->tabdir.tabs[i].tag);
	putlong(ttf,at->tabdir.tabs[i].checksum);
	putlong(ttf,at->tabdir.tabs[i].offset);
	putlong(ttf,at->tabdir.tabs[i].length);
    }

    i=0;
    if ( format==ff_otf || format==ff_otfcid)
	if ( !ttfcopyfile(ttf,at->cfff,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->bdat!=NULL && at->msbitmaps ) {
	if ( !ttfcopyfile(ttf,at->bdat,at->tabdir.tabs[i++].offset)) at->error = true;
	if ( !ttfcopyfile(ttf,at->bloc,at->tabdir.tabs[i++].offset)) at->error = true;
    }
    if ( at->gpos!=NULL )
	if ( !ttfcopyfile(ttf,at->gpos,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->gsub!=NULL )
	if ( !ttfcopyfile(ttf,at->gsub,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( !ttfcopyfile(ttf,at->os2f,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->vorgf!=NULL )
	if ( !ttfcopyfile(ttf,at->vorgf,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->bdat!=NULL && !at->msbitmaps ) {
	if ( !ttfcopyfile(ttf,at->bdat,at->tabdir.tabs[i++].offset)) at->error = true;
	if ( format==ff_none ) {
	    head_index = i;
	    if ( !ttfcopyfile(ttf,at->headf,at->tabdir.tabs[i++].offset)) at->error = true;
	}
	if ( !ttfcopyfile(ttf,at->bloc,at->tabdir.tabs[i++].offset)) at->error = true;
    }
    if ( !ttfcopyfile(ttf,at->cmap,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( format!=ff_otf && format!= ff_otfcid && format!=ff_none ) {
	if ( !ttfcopyfile(ttf,at->cvtf,at->tabdir.tabs[i++].offset)) at->error = true;
	if ( at->gi.fpgmf!=NULL ) {
	    if ( !ttfcopyfile(ttf,at->gi.fpgmf,at->tabdir.tabs[i++].offset)) at->error = true;
	}
	if ( !ttfcopyfile(ttf,at->gi.glyphs,at->tabdir.tabs[i++].offset)) at->error = true;
    }
    if ( format!=ff_none ) {
	head_index = i;
	if ( !ttfcopyfile(ttf,at->headf,at->tabdir.tabs[i++].offset)) at->error = true;
    }
    if ( !ttfcopyfile(ttf,at->hheadf,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( !ttfcopyfile(ttf,at->gi.hmtx,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->kern!=NULL )
	if ( !ttfcopyfile(ttf,at->kern,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( format!=ff_otf && format!=ff_otfcid && format!=ff_none )
	if ( !ttfcopyfile(ttf,at->loca,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( !ttfcopyfile(ttf,at->maxpf,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( !ttfcopyfile(ttf,at->name,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( !ttfcopyfile(ttf,at->post,at->tabdir.tabs[i++].offset)) at->error = true;
    if ( at->vheadf!=NULL ) {
	if ( !ttfcopyfile(ttf,at->vheadf,at->tabdir.tabs[i++].offset)) at->error = true;
	if ( !ttfcopyfile(ttf,at->gi.vmtx,at->tabdir.tabs[i++].offset)) at->error = true;
    }

    checksum = filecheck(ttf);
    checksum = 0xb1b0afba-checksum;
    fseek(ttf,at->tabdir.tabs[head_index].offset+2*sizeof(int32),SEEK_SET);
    putlong(ttf,checksum);

    /* ttfcopyfile closed all the files (except ttf) */
}

int _WriteTTFFont(FILE *ttf,SplineFont *sf,enum fontformat format,
	int32 *bsizes, enum bitmapformat bf) {
    struct alltabs at;
    char *oldloc;

    oldloc = setlocale(LC_NUMERIC,"C");		/* TrueType probably doesn't need this, but OpenType does for floats in dictionaries */
    if ( format==ff_otfcid ) {
	if ( sf->cidmaster ) sf = sf->cidmaster;
    } else {
	if ( sf->subfontcnt!=0 ) sf = sf->subfonts[0];
    }
    initTables(&at,sf,format,bsizes,bf);
    dumpttf(ttf,&at,format);
    setlocale(LC_NUMERIC,oldloc);
    if ( ferror(ttf))
return( 0 );

return( 1 );
}

int WriteTTFFont(char *fontname,SplineFont *sf,enum fontformat format,
	int32 *bsizes, enum bitmapformat bf) {
    FILE *ttf;
    int ret;

    if (( ttf=fopen(fontname,"w+"))==NULL )
return( 0 );
    ret = _WriteTTFFont(ttf,sf,format,bsizes,bf);
    if ( fclose(ttf)==-1 )
return( 0 );
return( ret );
}
    
/* Fontograpgher also generates: fpgm, hdmx, prep */
