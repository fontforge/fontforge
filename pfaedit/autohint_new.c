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
#include <stdio.h>
#include <math.h>
#include "splinefont.h"
#include "views.h"
#include <utype.h>

/* to create a type 1 font we must come up with the following entries for the
  private dictionary:
    BlueValues	-- an array of 2n entries where Blue[2i]<Blue[2i+1] max n=7, Blue[i]>0
    OtherBlues	-- (optional) OtherBlue[i]<0
    	(blue zones should be at least 3 units appart)
    StdHW	-- (o) array with one entry, standard hstem height
    StdVW	-- (o) ditto vstem width
    StemSnapH	-- (o) up to 12 numbers containing various hstem heights (includes StdHW), small widths first
    StemSnapV	-- (o) ditto, vstem
This file has routines to figure out at least some of these

Also other routines to guess at good per-character hints
*/

static void AddBlue(double val, double array[5], int force) {
    val = rint(val);
    if ( !force && (val<array[0]-array[1] || val >array[0]+array[1] ))
return;		/* Outside of one sd */
    if ( array[3]==0 && array[4]==0 )
	array[3] = array[4] = val;
    else if ( val>array[4] )
	array[4] = val;
    else if ( val<array[3] )
	array[3] = val;
}

static void MergeZones(double zone1[5], double zone2[5]) {
    if ( zone1[2]!=0 && zone2[2]!=0 &&
	    ((zone1[4]+3>zone2[3] && zone1[3]<=zone2[3]) ||
	     (zone2[4]+3>zone1[3] && zone2[3]<=zone1[3]) )) {
	if (( zone2[0]<zone1[0]-zone1[1] || zone2[0] >zone1[0]+zone1[1] ) &&
		( zone1[0]<zone2[0]-zone2[1] || zone1[0] >zone2[0]+zone2[1] ))
	    /* the means of the zones are too far appart, don't merge em */;
	else {
	    if ( zone1[0]<zone2[0] ) zone2[0] = zone1[0];
	    if ( zone1[1]>zone2[1] ) zone2[1] = zone1[1];
	}
	zone1[2] = 0;
    }
}

/* I can deal with latin, greek and cyrillic because the they all come from */
/*  the same set of letter shapes and have all evolved together and have */
/*  various common features (ascenders, descenders, lower case, etc.). Other */
/*  scripts don't fit */
void FindBlues( SplineFont *sf, double blues[14], double otherblues[10]) {
    double caph[5], xh[5], ascenth[5], digith[5], descenth[5], base[5];
    double otherdigits[5];
    int i, any = 0, j, k;
    DBounds b;

    /* Go through once to get some idea of the average value so we can weed */
    /*  out undesireables */
    caph[0] = caph[1] = caph[2] = xh[0] = xh[1] = xh[2] = 0;
    ascenth[0] = ascenth[1] = ascenth[2] = digith[0] = digith[1] = digith[2] = 0;
    descenth[0] = descenth[1] = descenth[2] = base[0] = base[1] = base[2] = 0;
    otherdigits[0] = otherdigits[1] = otherdigits[2] = 0;
    for ( i=0; i<sf->charcnt; ++i ) {
	if ( sf->chars[i]!=NULL && sf->chars[i]->splines!=NULL ) {
	    int enc = sf->chars[i]->unicodeenc;
	    unichar_t *upt;
	    if ( isalnum(enc) &&
		    ((enc>=32 && enc<128 ) || enc == 0xfe || enc==0xf0 || enc==0xdf ||
		     (enc>=0x391 && enc<=0x3f3 ) ||
		     (enc>=0x400 && enc<=0x4e9 ) )) {
		/* no accented characters (or ligatures) */
		if ( unicode_alternates[enc>>8]!=NULL &&
			(upt =unicode_alternates[enc>>8][enc&0xff])!=NULL &&
			upt[1]!='\0' )
    continue;
		any = true;
		SplineCharFindBounds(sf->chars[i],&b);
		if ( b.miny==0 && b.maxy==0 )
    continue;
		if ( enc=='g' || enc=='j' || enc=='p' || enc=='q' || enc=='y' ||
			enc==0xfe || enc == 0x3c6 || enc==0x3c8
			enc==0x434 || enc==0x444 || enc==0x449 || enc==0x471) {
		    descenth[0] += b.miny;
		    descenth[1] += b.miny*b.miny;
		    ++descenth[2];
		} else {
		    base[0] += b.miny;
		    base[1] += b.miny*b.miny;
		    ++base[2];
		}
		/* careful of lowercase digits, 6 and 8 should be ascenders */
		if ( enc=='6' || enc=='8' ) {
		    digith[0] += b.maxy;
		    digith[1] += b.maxy*b.maxy;
		    ++digith[2];
		} else if ( isdigit(enc) ) {
		    otherdigits[0] += b.maxy;
		    otherdigits[1] += b.maxy*b.maxy;
		    ++otherdigits[2];
		} else if ( isupper(enc)) {
		    caph[0] += b.maxy;
		    caph[1] += b.maxy*b.maxy;
		    ++caph[2];
		} else if ( enc=='b' || enc=='d' || enc=='f' || enc=='h' || enc=='k' ||
			enc == 'l' || enc==0xf0 || enc==0xfe || enc == 0xdf ||
			enc == 0x3b2 || enc==0x3b6 || enc==0x3b8 || enc==0x3bb ||
			enc == 0x3be ||
			enc == 0x444 ) {
		    ascenth[0] += b.maxy;
		    ascenth[1] += b.maxy*b.maxy;
		    ++ascenth[2];
		} else if ( enc!='i' && enc!='j' ) {
		    xh[0] += b.maxy;
		    xh[1] += b.maxy*b.maxy;
		    ++xh[2];
		}
	    }
	}
	GProgressNext();
    }
    if ( otherdigits[2]>0 && digith[2]>0 ) {
	if ( otherdigits[0]/otherdigits[2] >= .95*digith[0]/digith[2] ) {
	    /* all digits are about the same height, not lowercase */
	    digith[0] += otherdigits[0];
	    digith[1] += otherdigits[1];
	    digith[2] += otherdigits[2];
	}
    }

    if ( xh[2]>1 ) {
	xh[1] = sqrt((xh[1]-xh[0]*xh[0]/xh[2])/(xh[2]-1));
	xh[0] /= xh[2];
    }
    if ( ascenth[2]>1 ) {
	ascenth[1] = sqrt((ascenth[1]-ascenth[0]*ascenth[0]/ascenth[2])/(ascenth[2]-1));
	ascenth[0] /= ascenth[2];
    }
    if ( caph[2]>1 ) {
	caph[1] = sqrt((caph[1]-caph[0]*caph[0]/caph[2])/(caph[2]-1));
	caph[0] /= caph[2];
    }
    if ( digith[2]>1 ) {
	digith[1] = sqrt((digith[1]-digith[0]*digith[0]/digith[2])/(digith[2]-1));
	digith[0] /= digith[2];
    }
    if ( base[2]>1 ) {
	base[1] = sqrt((base[1]-base[0]*base[0]/base[2])/(base[2]-1));
	base[0] /= base[2];
    }
    if ( descenth[2]>1 ) {
	descenth[1] = sqrt((descenth[1]-descenth[0]*descenth[0]/descenth[2])/(descenth[2]-1));
	descenth[0] /= descenth[2];
    }

    /* we'll accept values between +/- 1sd of the mean */
    /* array[0] == mean, array[1] == sd, array[2] == cnt, array[3]=min, array[4]==max */
    caph[3] = caph[4] = 0;
    xh[3] = xh[4] = 0;
    ascenth[3] = ascenth[4] = 0;
    digith[3] = digith[4] = 0;
    descenth[3] = descenth[4] = 0;
    base[3] = base[4] = 0;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	int enc = sf->chars[i]->unicodeenc;
	unichar_t *upt;
	if ( isalnum(enc) &&
		((enc>=32 && enc<128 ) || enc == 0xfe || enc==0xf0 || enc==0xdf ||
		 (enc>=0x391 && enc<=0x3f3 ) ||
		 (enc>=0x400 && enc<=0x4e9 ) )) {
	    /* no accented characters (or ligatures) */
	    if ( unicode_alternates[enc>>8]!=NULL &&
		    (upt =unicode_alternates[enc>>8][enc&0xff])!=NULL &&
		    upt[1]!='\0' )
    continue;
	    any = true;
	    SplineCharFindBounds(sf->chars[i],&b);
	    if ( b.miny==0 && b.maxy==0 )
    continue;
	    if ( enc=='g' || enc=='j' || enc=='p' || enc=='q' || enc=='y' ||
		    enc==0xfe || enc == 0x3c6 || enc==0x3c8
		    enc==0x434 || enc==0x444 || enc==0x449 || enc==0x471) {
		AddBlue(b.miny,descenth,false);
	    } else {
		/* O and o get forced into the baseline blue value even if they*/
		/*  are beyond 1 sd */
		AddBlue(b.miny,base,enc=='O' || enc=='o');
	    }
	    if ( isdigit(enc)) {
		AddBlue(b.maxy,digith,false);
	    } else if ( isupper(enc)) {
		AddBlue(b.maxy,caph,enc=='O');
		} else if ( enc=='b' || enc=='d' || enc=='f' || enc=='h' || enc=='k' ||
			enc == 'l' || enc=='t' || enc==0xf0 || enc==0xfe || enc == 0xdf ||
			enc == 0x3b2 || enc==0x3b6 || enc==0x3b8 || enc==0x3bb ||
		    enc == 0x3be ||
		    enc == 0x444 ) {
		AddBlue(b.maxy,ascenth,false);
	    } else if ( enc!='i' && enc!='j' ) {
		AddBlue(b.maxy,xh,enc=='o');
	    }
	}
    }

    /* the descent blue zone merges into the base zone */
    MergeZones(descenth,base);
    MergeZones(xh,base);
    MergeZones(ascenth,caph);
    MergeZones(digith,caph);
    MergeZones(xh,caph);
    MergeZones(ascenth,digith);
    MergeZones(xh,digith);

    if ( otherblues!=NULL )
	for ( i=0; i<10; ++i )
	    otherblues[i] = 0;
    for ( i=0; i<14; ++i )
	blues[i] = 0;

    if ( otherblues!=NULL && descenth[2]!=0 ) {
	otherblues[0] = descenth[3];
	otherblues[1] = descenth[4];
    }
    i = 0;
    if ( base[2]==0 && (xh[2]!=0 || ascenth[2]!=0 || caph[2]!=0 || digith[2]!=0 )) {
	/* base line blue value must be present if any other value is */
	/*  make one up if we don't have one */
	blues[0] = -20;
	blues[1] = 0;
	i = 2;
    } else if ( base[2]!=0 ) {
	blues[0] = base[3];
	blues[1] = base[4];
	i = 2;
    }
    if ( xh[2]!=0 ) {
	blues[i++] = xh[3];
	blues[i++] = xh[4];
    }
    if ( caph[2]!=0 ) {
	blues[i++] = caph[3];
	blues[i++] = caph[4];
    }
    if ( digith[2]!=0 ) {
	blues[i++] = digith[3];
	blues[i++] = digith[4];
    }
    if ( ascenth[2]!=0 ) {
	blues[i++] = ascenth[3];
	blues[i++] = ascenth[4];
    }

    for ( j=0; j<i; j+=2 ) for ( k=j+2; k<i; k+=2 ) {
	if ( blues[j]>blues[k] ) {
	    double temp = blues[j];
	    blues[j]=blues[k];
	    blues[k] = temp;
	    temp = blues[j+1];
	    blues[j+1] = blues[k+1];
	    blues[k+1] = temp;
	}
    }
}

/* Some stems may appear, disappear, reapear several times */
/* Serif stems on I which appear at 0, disappear, reappear at top */
/* Or the major vertical stems on H which disappear at the cross bar */
typedef struct hintinstance {
    double begin;			/* location in the non-major direction*/
    double end;				/* width/height in non-major direction*/
    unsigned int closed: 1;
    struct hintinstance *next;
} HintInstance;

typedef struct steminfo {
    double start;			/* location at which the stem starts */
    double width;			/* or height */
    HintInstance *where;		/* location(s) in the other coord */
    struct steminfo *next;
    unsigned int toobig:1;	/* get rid of this guy if it overlaps another */
    unsigned int haspointleft:1;
    unsigned int haspointright:1;
    unsigned int haspoints:1;	/* both edges of the stem have points on them */
				/*  at the stem left and right edge */
			        /*  trivially true for horizontal/vertical lines */
			        /*  true for curves if extrema have points */
			        /*  except we aren't smart enough to detect that yet */
} StemInfo ;

typedef struct edgeinfo {
    /* The spline is broken up at all points of inflection. So... */
    /*  The spline between tmin and tmax is monotonic in both coordinates */
    /*  If the spline becomes vert/horizontal that will be at one of the */
    /*   end points too */
    Spline *spline;
    double tmin, tmax;
    double coordmin[2];
    double coordmax[2];
    unsigned int up: 1;
    unsigned int hv: 1;
    unsigned int hvbottom: 1;
    unsigned int hvtop: 1;
    unsigned int hor: 1;
    unsigned int vert: 1;
    unsigned int almosthor: 1;
    unsigned int almostvert: 1;
    unsigned int horattmin: 1;
    unsigned int horattmax: 1;
    unsigned int vertattmin: 1;
    unsigned int vertattmax: 1;
    unsigned hup: 1;
    unsigned vup: 1;
    double tcur;		/* Value of t for current major coord */
    double ocur;		/* Value of the other coord for current major coord */
    struct edgeinfo *next;
    sturct edgeinfo *ordered;
    sturct edgeinfo *aenext;
    SplineChar *sc;
    int major;
} EI;

typedef struct splbounds {
    SplineSet *spl;
    DBounds b;
    struct splbounds *next;
} SPLBounds;

typedef struct splcluster {
    SPLBounds *spls;
    DBounds b;
    struct splcluster *next;
    StemInfo *vstem, *hstem;
    double *vbreaks, *hbreaks;
    int vbcnt, hbcnt;
} SPLCluster;

typedef struct eilist {
    EI *edges;
    double coordmin[2];
    double coordmax[2];
    int low, high, cnt;
    EI **ordered;
    char *ends;			/* flag to say an edge ends on this line */
    SPLBounds *spls;
    SPLClusters *clusters;
} EIList;

static void EIAddEdge(Spline *spline, double tmin, double tmax, EIList *el,DBounds *b) {
    EI *new = gcalloc(1,sizeof(EI));
    double min, max;
    Spline1D *s;
    double dxdtmin, dxdtmax, dydtmin, dydtmax;

    new->spline = spline;
    new->tmin = tmin;
    new->tmax = tmax;

    s = &spline->spline[1];
    if (( dydtmin = (3*s->a*tmin + 2*s->b)*tmin + s->c ) dydtmin = -dydtmin;
    if (( dydtmax = (3*s->a*tmax + 2*s->b)*tmax + s->c ) dydtmax = -dydtmax;
    s = &spline->spline[0];
    if (( dxdtmin = (3*s->a*tmin + 2*s->b)*tmin + s->c ) dxdtmin = -dxdtmin;
    if (( dxdtmax = (3*s->a*tmax + 2*s->b)*tmax + s->c ) dxdtmax = -dxdtmax;
    
    /*s = &spline->spline[0];*/
    min = ((s->a * tmin + s->b)* tmin + s->c)* tmin + s->d;
    max = ((s->a * tmax + s->b)* tmax + s->c)* tmax + s->d;
    if ( min==max )
	new->vert = true;
    else if ( min<max )
	new->hup = true;
    else {
	temp = min; min = max; max=temp;
    }
    if ( min+1>max ) new->almostvert = true;
    if ( dxdtmin<=40*dydtmin ) new->vertattmin = true;
    if ( dxdtmax<=40*dydtmax ) new->vertattmax = true;
    new->coordmin[0] = min; new->coordmax[0] = max;
    if ( el->coordmin[0]>min )
	el->coordmin[0] = min;
    if ( el->coordmax[0]<max )
	el->coordmax[0] = max;
    if ( sb->minx>min ) sb->minx = min;
    if ( sb->maxx<max ) sb->maxx = max;

    s = &spline->spline[1];
    min = ((s->a * tmin + s->b)* tmin + s->c)* tmin + s->d;
    max = ((s->a * tmax + s->b)* tmax + s->c)* tmax + s->d;
    if ( min==max )
	new->hor = true;
    else if ( min<max )
	new->vup = true;
    else {
	temp = min; min = max; max=temp;
    }
    if ( min+1>max ) new->almosthor = true;
    if ( dydtmin<=40*dxdtmin ) new->horattmin = true;
    if ( dydtmax<=40*dxdtmax ) new->horattmax = true;
    new->coordmin[1] = min; new->coordmax[1] = max;
    if ( el->coordmin[1]>min )
	el->coordmin[1] = min;
    if ( el->coordmax[1]<max )
	el->coordmax[1] = max;
    if ( sb->miny>min ) sb->miny = min;
    if ( sb->maxy<max ) sb->maxy = max;

    new->next = el->edges;
    el->edges = new;
}

static void EIAddSpline(Spline *spline, EIList *el, DBounds *b) {
    double ts[6];
    int i, j, base;

    ts[0] = 0; ts[5] = 1.0;
    SplineFindInflections(&spline->splines[0],&ts[1],&ts[2]);
    SplineFindInflections(&spline->splines[1],&ts[3],&ts[4]);
    for ( i=0; i<4; ++i ) for ( j=i+1; j<5; ++j ) {
	if ( ts[i]>ts[j] ) {
	    temp = ts[i];
	    ts[i] = ts[j];
	    ts[j] = temp;
	}
    }
    for ( base=0; ts[base]==-1; ++base );
    for ( i=5; i>base ; --i ) {
	if ( ts[i]==ts[i-1] ) {
	    for ( j=i-1; j>base; --j )
		ts[j] = ts[j-1];
	    ts[j] = -1;
	    ++base;
	}
    }
    for ( i=base; i<5 ; ++i )
	EIAddEdge(spline,ts[i],ts[i+1],el,&b);
}

/* Divide the set of splinesets into a series of non-overlapping clusters */
/* Hints in different clusters will not overlap */
static void FindClusters(EIList *el) {
    SPLCluster *cnext=NULL, *c;
    SPLBounds *sb, *snext, *prev;
    int any;

    while ( el->spls!=NULL ) {
	c = gcalloc(1,sizeof(SPLCluster));
	c->spls = el->spls;
	el->spls = el->spls->next;
	c->spls->next = NULL;
	c->b = c->spls->b;
	if ( cnext==NULL )
	    el->clusters = c;
	else
	    cnext->next = c;
	cnext = c;

	any = true;
	while ( any ) {
	    any = false;
	    for ( prev=NULL, sb=el->spls; sb!=NULL; sb=snext ) {
		snext = sb->next;
		if ( BoundsIntersect(&c->b,&sb->b)) {
		    BoundsMerge(&c->b,&sb->b);
		    any = true;
		    if ( prev==NULL )
			el->spls = snext;
		    else
			prev->next = snext;
		    sb->next = c->spls;
		    c->spls = sb;
		} else
		    prev = sb;
	    }
	}
    }
}

static void ELFindEdges(SplineChar *sc, EIList *el) {
    Spline *spline, *first;
    SplineSet *spl;
    SPLBounds *sb, *sblast = NULL;

    el->sc = sc;
    if ( sc->splines==NULL )
return;

    el->coordmin[0] = el->coordmax[0] = sc->splines->first->me.x;
    el->coordmin[1] = el->coordmax[1] = sc->splines->first->me.y;

    for ( spl = sc->splines; spl!=NULL; spl = spl->next ) {
	sb = gcalloc( 1,sizeof(SPLBounds));
	sb->spl = spl;
	sb->b.minx = sb->b.maxx = spl->first->me.x;
	sb->b.miny = sb->b.maxy = spl->first->me.y;
	if ( sblast==NULL )
	    el->spls = sb;
	else
	    sblast->next = sb;
	sblast = sb;

	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    EIAddSpline(spline,el,&sb->b);
	    if ( first==NULL ) first = spline;
	}
    }

    FindClusters(el);
}

static int IsBiggerSlope(EI *test, EI *base, int major) {
    int other = !major;
    double tdo, tdm, bdo, bdm;

    if (( major && test->vup ) || (!major && test->hup))
	t = test->tmin;
    else
	t = test->tmax;
    tdm = (3*test->spline->splines[major].a*t + 2*test->spline->splines[major].b)*t + test->spline->splines[major].c;
    tdo = (3*test->spline->splines[other].a*t + 2*test->spline->splines[other].b)*t + test->spline->splines[other].c;

    if (( major && base->vup ) || (!major && base->hup))
	t = base->tmin;
    else
	t = base->tmax;
    bdm = (3*base->spline->splines[major].a*t + 2*base->spline->splines[major].b)*t + base->spline->splines[major].c;
    bdo = (3*base->spline->splines[other].a*t + 2*base->spline->splines[other].b)*t + base->spline->splines[other].c;

    if ( tdm==0 && bdm==0 )
return( tdo > bdo );
    if ( tdo==0 )
return( tdo>0 );
    else if ( bdo==0 )
return( bdo>0 );

return( tdo/tdm > bdo/bdm );
}

static void ELOrder(EIList *el, int major, DBounds *b ) {
    int other = !major;
    int pos;
    EI *ei;
    BasePoint *me;

    el->low = floor(major?b->miny:b->minx); el->high = ceil(major?b->maxy:b->maxx);
    el->cnt = el->high-el->low+1;
    el->ordered = galloc(cnt*sizeof(EI *));
    el->ends = galloc(cnt);

    for ( ei = el->edges; ei!=NULL ; ei=ei->next ) {
	me = &ei->spline->from->me;
	if ( me->x<b->minx || me->x>b->maxx || me->y<b->miny || b->y>b->maxy )
    continue;		/* Not in this cluster */
	if ( ei->coordmin[major]>ei->coordmax[major]-1 )
    continue;
	pos = ceil(ei->coordmax[major])-el->low;
	el->ends[pos] = true;
	pos = floor(ei->coordmin[major])-el->low;
	ei->ocur = (ei->hup == ei->vup)?ei->coordmin[other]:ei->coordmax[other];
	ei->tcur = ((major && ei->vup) || (!major && ei->hup)) ?
			ei->tmin: ei->tmax;
	if ( major ) {
	    ei->up = ei->vup;
	    ei->hv = (ei->vert || ei->almostvert);
	    ei->hvbottom = ei->vup ? ei->vertattmin : ei->vertattmax;
	    ei->hvtop    =!ei->vup ? ei->vertattmin : ei->vertattmax;
	} else {
	    ei->up = ei->hup;
	    ei->hv = (ei->hor || ei->almosthor);
	    ei->hvbottom = ei->hup ? ei->horattmin : ei->horattmax;
	    ei->hvtop    =!ei->hup ? ei->horattmin : ei->horattmax;
	}
	if ( el->ordered[pos]==NULL || ei->ocur<el->ordered[pos]->ocur ) {
	    ei->ordered = el->ordered[pos];
	    el->ordered[pos] = ei;
	} else {
	    for ( prev=el->ordered[pos], test = prev->ordered; test!=NULL;
		    prev = test, test = test->ordered ) {
		if ( test->ocur>ei->ocur ||
			(test->ocur==ei->ocur && IsBiggerSlope(test,ei,major)))
	    break;
	    }
	    ei->ordered = test;
	    prev->ordered = ei;
	}
    }
}

/* If the stem is vertical here, return an approximate height over which it is */
/* if major is 1 then we check for vertical lines, if 0 then horizontal */
static double IsEdgeHorVertHere(EI *e,int major, int i, double *up, double *down) {
    /* It's vertical if dx/dt == 0 or if (dy/dt)/(dx/dt) is infinite */
    Spline *spline = e->spline;
    Spline1D *msp = &spline->splines[major], *osp = &spline->splines[!major];
    double dodt = (3*osp->a*e->t_cur+2*osp->b)*e->t_cur + osp->c;
    double dmdt = (3*msp->a*e->t_cur+2*msp->b)*e->t_cur + msp->c;
    double tmax, tmin, t, o, ocur;
    double dup, ddown;

    if ( dodt<0 ) dodt = -dodt;
    if ( dmdt<0 ) dmdt = -dmdt;
    if ( dodt>.02 && dmdt/dodt<40 )		/* very rough defn of 0 and infinite */
return( 0.0 );

    if ( osp->a==0 && osp->b==0 && osp->c==0 ) {
	*up = e->coordmax[major]-i;
	*down = i-e->coordmin[major];
return( e->coordmax[major]-e->coordmin[major] );
    }

    dup = ddown = 0;
    tmax = e->tmax;
    tmin = e->t_cur;
    ocur = ((osp->a*tmin+osp->b)*tmin+osp->c)*tmin + osp->d;
    while ( tmax-tmin>.0005 ) {
	t = (tmin+tmax)/2;
	o = ((osp->a*t+osp->b)*t+osp->c)*t + osp->d;
	if ( o-ocur<1 && o-ocur>-1 ) {
	    dup = ((msp->a*t+msp->b)*t+msp->c)*t+msp->d-i;
	    if ( tmax-tmin<.001 )
    break;
	    tmin = t;
	} else
	    tmax = t;
    }
    tmin = e->tmin;
    tmax = e->t_cur;
    while ( tmax-tmin>.0005 ) {
	t = (tmin+tmax)/2;
	o = ((osp->a*t+osp->b)*t+osp->c)*t + osp->d;
	if ( o-ocur<1 && o-ocur>-1 ) {
	    ddown = ((msp->a*t+msp->b)*t+msp->c)*t+msp->d-i;
	    if ( tmax-tmin<.001 )
    break;
	    tmax = t;
	} else
	    tmin = t;
    }
    if ( dup<0 ) {
	*down = dup= -dup;
	*up = ddown;
    } else {
	*down = ddown = -ddown;
	*up = dup;
    }
return( dup+ddown );
}

static int IsNearHV(EI *hv,EI *test,int i,double *up, double *down, int major) {
    double u1, u2, d1, d2;
    if ( IsEdgeHorVertHere(test,major,i,&u2,&d2)==0 )
return( false );
    if ( IsEdgeHorVertHere(hv,major,i,&u1,&d1)==0 )
return( false );
    if ( u1<u2 ) u2=u1;
    if ( d1<d2 ) d2=d1;
    *up = u2;
    *down = d2;
return( true );
}

static double EIFigurePos(EI *e,int other, int *hadpt ) {
    double val = e->o_cur;
    int oup = other==0?e->hup:e->vup;

    *hadpt = false;
    if ( e->tmin==0 && ((oup && e->coordmin[other]+1>val) ||
			(!oup && e->coordmax[other]-1<val)) ) {
	*hadpt = true;
	val = oup ? e->coordmin[other] : e->coordmax[other];
    } else if ( e->tmin==1 && ((!oup && e->coordmin[other]+1>val) ||
				(oup && e->coordmax[other]-1<val)) ) {
	*hadpt = true;
	val = !oup ? e->coordmin[other] : e->coordmax[other];
    }
return( val );
}

static Stems *StemsFind(Stems *stems,EI *apt,EI *e, int major) {
    int other = !major;
    int shadpt, ehadpt;
    double start = EIFigurePos(apt,other,&shadpt);
    double end = EIFigurePos(e,other,&ehadpt);
    Stems *test;

    for ( test=stems; test!=NULL; test=test->next ) {
	if ( start-1<test->start && start+1>test->start &&
		end-1<test->start+test->width && end+1>test->start+test->width ) {
	    if ( start!=test->start && shadpt && !test->haspointleft ) {
		test->width = test->start+test->width - start;
		test->start = start;
		test->haspointleft = true;
	    }
	    if ( end!=test->start+test->width && ehadpt && !test->haspointright ) {
		test->width = end-test->start;
		test->haspointright = true;
	    }
return( test );
	}
    }

    test = gcalloc(1,sizeof(Stems));
    test->start = start;
    test->width = end-start;
    test->haspointleft = shadpt;
    test->haspointright = ehadpt;
return( test );
}

static Stems *StemAddBrief(Stems *stems,EI *apt,EI *e,
	double mstart, double mend, int major) {
    Stems *new = StemsFind(stems,apt,e);
    HintInstance *hi;

    if ( new->where==NULL )
	stems = StemInsert(stems,new);
    hi = gcalloc(1,sizeof(HintInstance));
    hi->begin = mstart;
    hi->end = mend;
    hi->next = new->where;
    hi->closed = true;
    new->where = hi;
return( stems );
}

static Stems *StemAddUpdate(Stems *stems,EI *apt,EI *e, int i, int major) {
    Stems *new = StemsFind(stems,apt,e);

    if ( new->where==NULL )
	stems = StemInsert(stems,new);
    else if ( new->where->end>=i )
return;
    if ( new->where==NULL || new->where->closed ) {
	hi = gcalloc(1,sizeof(HintInstance));
	hi->begin = i;
	hi->end = i;
	hi->next = new->where;
	new->where = hi;
    } else
	hi->end = i;
return( stems );
}

static void StemCloseUntouched(Stems *stems,int i ) {
    Stems *test;

    for ( test=stems; test!=NULL; test=test->next )
	if ( test->where->end<i )
	    test->where->closed = true;
}


double EITOfNextMajor(EI *e, EIList *el, double sought_m ) {
    /* We want to find t so that Mspline(t) = sought_m */
    /*  the curve is monotonic */
    Spline *sp = e->spline;
    Spline1D *msp = &e->spline->splines[el->major];
    double new_t, old_t, newer_t;
    double found_m;
    double t_mmax, t_mmin;
    double mmax, mmin;

    if ( msp->a==0 && msp->b==0 ) {
	new_t = e->t_cur + (sought_m-e->m_cur)/(msp->c);
return( new_t );
    }

    old_t = e->t_cur;
    mmax = e->coordmax[el->major];
    t_mmax = e->up?e->tmax:e->tmin;
    mmin = e->m_cur;
    t_mmin = old_t;
    sought_m += el->low;

    while ( 1 ) {
	new_t = (t_mmin+t_mmax)/2;
	found_m = ( ((msp->a*new_t+msp->b)*new_t+msp->c)*new_t + msp->d );
	if ( found_m>sought_m-.001 && found_m<sought_m+.001 )
return( new_t );
	if ( found_m > sought_m ) {
	    mmax = found_m;
	    t_mmax = new_t;
	} else {
	    mmin = found_m;
	    t_mmin = new_t;
	}
	if ( t_mmax==t_mmin ) {
	    GDrawIError("EITOfNextMajor failed! on %s", el->sc!=NULL?es->sc->name:"Unknown" );
return( new_t );
	}
    }
}

Edge *EIActiveEdgesRefigure(EIList *el, EI *active,double i,int major, int *_change) {
    Edge *apt, *pr;
    int any;
    int change = false;
    int other = !major;

    /* first remove any entry which doesn't intersect the new scan line */
    /*  (ie. stopped on last line) */
    for ( pr=NULL, apt=active; apt!=NULL; apt = apt->aenext ) {
	if ( apt->coordmax[major]<i ) {
	    if ( pr==NULL )
		active = apt->aenext;
	    else
		pr->aenext = apt->aenext;
	    change = true;
	} else
	    pr = apt;
    }
    /* then move the active list to the next line */
    for ( apt=active; apt!=NULL; apt = apt->aenext ) {
	Spline1D *osp = &apt->spline->splines[other];
	apt->tcur = TOfNextMajor(apt,el,i);
	apt->ocur = ( ((osp->a*apt->tcur+osp->b)*apt->tcur+osp->c)*apt->tcur + osp->d );
    }
    /* reorder list */
    if ( active!=NULL ) {
	any = true;
	while ( any ) {
	    any = false;
	    for ( pr=NULL, apt=active; apt->aenext!=NULL; ) {
		if ( apt->o_cur <= apt->aenext->o_cur ) {
		    /* still ordered */;
		    pr = apt;
		    apt = apt->aenext;
		} else if ( pr==NULL ) {
		    active = apt->aenext;
		    apt->aenext = apt->aenext->aenext;
		    active->aenext = apt;
		    change = true;
		    /* don't need to set any, since this reorder can't disorder the list */
		    pr = active;
		} else {
		    pr->aenext = apt->aenext;
		    apt->aenext = apt->aenext->aenext;
		    pr->aenext->aenext = apt;
		    any = change = true;
		    pr = pr->aenext;
		}
	    }
	}
    }

    /* Insert new nodes */
    if ( el->ordered[(int) i]!=NULL ) change = true;
    for ( pr=NULL, apt=active, npt=el->ordered[(int) i]; apt!=NULL && npt!=NULL; ) {
	if ( npt->o_cur<apt->o_cur ) {
	    npt->aenext = apt;
	    if ( pr==NULL )
		active = npt;
	    else
		pr->aenext = npt;
	    pr = npt;
	    npt = npt->ordered;
	} else {
	    pr = apt;
	    apt = apt->aenext;
	}
    }
    while ( npt!=NULL ) {
	npt->aenext = NULL;
	if ( pr==NULL )
	    active = npt;
	else
	    pr->aenext = npt;
	pr = npt;
	npt = npt->esnext;
    }
    *_change = change;
return( active );
}

static int EISameLine(EI *e, EI *n) {
return( n!=NULL && n->up==e->up &&
	    (ceil(e->coordmin[major])==i || floor(e->coordmax[major])==i) &&
	    (ceil(n->coordmin[major])==i || floor(n->coordmax[major])==i) &&
	    ((n->spline->to->next==e->spline && n->tmax==1 && e->tmin==0 &&
		    n->tcur>.8 && e->tcur<.2) ||
	     (n->spline->from->prev==e->spline && n->tmin==0 && e->tmax==1 &&
		    n->tcur<.2 && e->tcur>.8) ||
	     (n->spline==e->spline && n->tmin==e->tmax &&
		    n->tcur<n->tmin+.2 && e->tcur>e->tmax-.2 ) ||
	     (n->spline==e->spline && n->tmax==e->tmin &&
		    n->tcur>n->tmax-.2 && e->tcur<e->tmin+.2 )));
}

static EI *EIActiveEdgesFindStem(EI *apt, double i, int major) {
    int cnt=apt->up?1:-1;
    Edge *pr, *e, *n;

    pr=apt; e=apt->aenext;
	/* If we're at an intersection point and the next spline continues */
	/*  in about the same direction then this doesn't count as two lines */
	/*  but as one */
    if ( e->aenext!=NULL && EISameLine(pr,e))
	e = e->aenext;

    for ( ; e!=NULL && cnt!=0; pr=e, e=e->aenext ) {
	if ( EISameLine(e,e->aenext))
	    e = e->aenext;
	cnt += (e->up?1:-1);
    }
return( pr );
}

static Stems *ELFindStems(EIList *el, int major) {
    EI *active=NULL, *apt, *pr, *e, *prev, *test;
    int i, change, ahv, ehv, waschange;
    StemInfo *stems=NULL;
    double len;

    waschange = false;
    for ( i=0; i<el->cnt; ++i ) {
	active = EIActiveEdgesRefigure(el,active,i,&change);
	/* Change means something started, ended, crossed */
	/* I'd like to know when a crossing is going to happen the pixel before*/
	/*  it does. but that's too hard to compute */
	if ( !( waschange || change ||
		(i!=el->cnt-1 && (el->ends[i+1] || el->ordered[i+1]!=NULL))) )
	    /* It's in the middle of everything. Nothing will have changed */
    continue;
	waschange = change;
	for ( apt=active; apt!=NULL; apt = e) {
	    e = EIActiveEdgesFindStem(apt, i);
	    ahv = ( apt->hv ||
		    (i==floor(apt->coordmin[major]) && apt->hvbottom) ||
		    (i==ceil(apt->coordmin[major])-1 && apt->hvtop) );
	    ehv = ( apt->hv ||
		    (i==floor(apt->coordmin[major]) && apt->hvbottom) ||
		    (i==ceil(apt->coordmin[major])-1 && apt->hvtop) );
	    if ( ahv && ehv )
		stems = StemAddUpdate(stems,apt,e);
	    else if ( ahv && !apt->hv && IsNearHV(apt,e,i,&up,&down,major))
		stems = StemAddBrief(apt,e,i-down,i+up,major);
	    else if ( ehv && !ept->hv && IsNearHV(e,apt,i,&up,&down,major))
		stems = StemAddBrief(apt,e,i-down,i+up,major);
	}
	StemCloseUntouched(stems,i);
    }
return( stems );
}

static Hints *StemRemoveSerifOverlaps(Stems *stems) {
    /* I don't think the rasterizer will be able to do much useful stuff with*/
    /*  with a serif vstem. What we want is to make sure the distance between*/
    /*  the nested (main) stem and the serif is the same on both sides but */
    /*  there is no mechanism for that */
    /* So I think they are useless. But they provide overlaps, which means */
    /*  we need to invoke hint substitution for something useless. So let's */
    /*  just get rid of them */

    /* The stems list is ordered by stem start. Look for any overlaps where: */
    /*  the nested stem has about the same distance to the right as to the left */
    /*  the nested stem's height is large compared to that of the serif (containing) */
    /*  the containing stem only happens at the top and bottom of the nested */

    Stems *serif, *main, *prev, *next;
    HintInstance *hi;

    prev = NULL;
    for ( serif=stems; serif!=NULL; serif=next ) {
	next = serif->next;
	for ( main=serif->next; main!=NULL && main->start<serif->start+serif->width;
		main = main->next ) {
	    double left, right, mh, sh, top, bottom;
	    left = main->start-serif->start;
	    right = serif->start+serif->width - (main->start+main->width);
	    if ( left-right<-5 || left-right>5 )
	continue;
	    /* In "H" the main stem is broken in two */
	    mh = 0;
	    bottom = main->where->start; top = main->where->end;
	    for ( hi = main->where; hi!=NULL; hi=hi->next ) {
		mh += hi->end-hi->start;
		if ( bottom>hi->start ) bottom = hi->start;
		if ( top < hi->end ) top = hi->end;
	    }
	    sh = 0;
	    for ( hi = serif->where; hi!=NULL; hi=hi->next ) {
		sh += hi->end-hi->start;
		if ( hi->end<top && hi->start>bottom )
	    break;	/* serif in middle => not serif */
	    }
	    if ( hi!=NULL )
	continue;	/* serif in middle => not serif */
	    if ( 4*sh>mh )
	continue;
	    if ( serif->where!=NULL && serif->where->next!=NULL && serif->where->next->next!=NULL )
	continue;	/* No more that two serifs, top & bottom */
	    /* If we get here, then we've got a serif and a main stem */
	break;
	}
	if ( main!=NULL && main->start<serif->start+serif->width ) {
	    /* If we get here, then we've got a serif and a main stem */
	    if ( prev==NULL )
		stems = next;
	    else
		prev->next = next;
	    StemFree(serif);
	} else
	    prev = serif;
    }
}

static Hints *ClusterFindStems(EIList *el, int major, DBounds *b) {
    StemInfo *stems;

    el->major = major;
    ELOrder(el,major,b);
    stems = ELFindStems(el,major);
    if ( major==1 && removeSerifOverlaps )
	stems = StemRemoveSerifOverlaps(stems);
    free(el->ordered);
return( stems );
}

static double *ClusterOverlapStems(StemInfo *stems,int *_bcnt) {
    StemInfo *base, *over, *next, *test;
    double max, *breaks;
    double *(ranges[2]);
    int cnt;

    ranges = NULL;
    while ( 1 ) {
	for ( base=stems; base!=NULL; base=next ) {
	    next = base->next;
	    max = base->start+base->width;
	    for ( over=next; over!=NULL && over->start<max; over=over->next ) {
		if ( over->start+over->width>max )
		    max = over->start+over->width;
	    }
	    if ( over==base->next )
	continue;		/* Nothing overlaps this stem */
	    /* All stems between [base,over) overlap somewhat */
	    for ( test=base->next; test!=NULL && test!=over; test=test->next ) {
		for ( bhi = base->where, thi=test->where; thi!=NULL && bhi!=NULL ; ) {
		    if ( thi->end<bhi->begin ) {
			while ( thi->next!=NULL && thi->next->end<bhi->begin )
			    thi = thi->next;
			if ( ranges!=NULL ) {
			    ranges[cnt][0] = thi->end;
			    ranges[cnt][1] = bhi->begin;
			}
			++cnt;
			thi = thi->next;
		    } else {
			while ( bhi->next!=NULL && bhi->next->end<=thi->begin )
			    bhi = bhi->next;
			if ( ranges!=NULL ) {
			    ranges[cnt][0] = bhi->end;
			    ranges[cnt][1] = thi->begin;
			}
			++cnt;
			bhi = bhi->next;
		    }
		}
	    }
	}
	if ( ranges!=NULL )
    break;
	ranges = gcalloc(cnt*2,sizeof(double));
    }

    /* Now merge things */
    for ( i=0; i<cnt; ++i ) {
	for ( j=i+1; j<cnt; ) {
	    if ( ranges[i][1]<ranges[j][0] || ranges[i][0]>ranges[j][1] )
		++j;		/* No overlap */
	    else {
		/* merge the two ranges to give the intersection */
		if ( ranges[i][0]<ranges[j][0] )
		    ranges[i][0] = ranges[j][0];
		if ( ranges[i][1]>ranges[j][1] )
		    ranges[i][1] = ranges[j][1];
		for ( k=j+1; k<cnt; ++k ) {
		    ranges[k-1][0] = ranges[k][0];
		    ranges[k-1][1] = ranges[k][1];
		}
		--cnt;
	    }
	}
    }

    breaks = galloc(cnt,sizeof(double));
    for ( i=0; i<cnt; ++i )
	breaks[i] = (ranges[i][0]+ranges[i][1])/2;
    *_bcnt = cnt;
    free(ranges);
}

void SplineCharAutoHint( SplineChar *sc) {
    EIList el;
    SPLClusters *c;

    memset(&el,'\0',sizeof(el));
    ELFindEdges(sc, &el);

    sc->changedsincelasthhinted = false;
    sc->changedsincelastvhinted = false;
    sc->manualhints = false;
    if ( sc->origtype1!=NULL )
	SCClearOrig(sc);

    for ( c=el.clusters; c!=NULL; c=c->next ) {
	c->vstems = ClusterFindStems(&el,1,&c->b);
	c->hstems = ClusterFindStems(&el,0,&c->b);
	c->vbreaks = ClusterOverlapStems(c->vstems,&c->vbcnt);
	c->hbreaks = ClusterOverlapStems(c->hstems,&c->hbcnt);
    }

    ElFreeEI(&el);
    SCOutOfDateBackground(sc);
    SCUpdateAll(sc);
    sc->parent->changed = true;
}
    
void SplineFontAutoHint( SplineFont *sf) {
    int i;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	if ( (sf->chars[i]->changedsincelasthhinted || sf->chars[i]->changedsincelastvhinted ) &&
		!sf->chars[i]->manualhints && sf->chars[i]->origtype1==NULL )
	    SplineCharAutoHint(sf->chars[i]);
}

static int SplineCharIsFlexible(SplineChar *sc, int blueshift) {
    /* Need two splines
	outer endpoints have same x (or y) values
	inner point must be less than 20 horizontal (v) units from the outer points
	inner point must also be less than BlueShift units (defaults to 7=>6)
	    (can increase BlueShift up to 21)
	the inner point must be a local extremum
		(something incomprehensible is written about the control points)
		perhaps that they should have the same x(y) value as the center
    */
    SplineSet *spl;
    SplinePoint *sp, *np, *pp;
    int max=0, val;

    for ( spl = sc->splines; spl!=NULL; spl=spl->next ) {
	if ( spl->first->prev==NULL ) {
	    /* Mark everything on the open path as inflexible */
	    sp=spl->first;
	    while ( 1 ) {
		sp->flexx = sp->flexy = false;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } 
    continue;				/* Ignore open paths */
	}
	sp=spl->first;
	do {
	    np = sp->next->to;
	    pp = sp->prev->from;
	    if ( !pp->flexx && !pp->flexy ) {
		sp->flexy = sp->flexx = 0;
		val = 0;
		if ( DoubleNear(sp->nextcp.x,sp->me.x) &&
			DoubleNear(sp->prevcp.x,sp->me.x) &&
			DoubleNear(np->me.x,pp->me.x) &&
			!DoubleNear(np->me.x,sp->me.x) &&
			np->me.x-sp->me.x < blueshift &&
			np->me.x-sp->me.x > -blueshift ) {
		    if ( (np->me.x>sp->me.x &&
			    np->prevcp.x<=np->me.x && np->prevcp.x>=sp->me.x &&
			    pp->nextcp.x<=pp->me.x && pp->prevcp.x>=sp->me.x ) ||
			  (np->me.x<sp->me.x &&
			    np->prevcp.x>=np->me.x && np->prevcp.x<=sp->me.x &&
			    pp->nextcp.x>=pp->me.x && pp->prevcp.x<=sp->me.x )) {
			sp->flexx = true;
			val = np->me.x-sp->me.x;
		    }
		}
		if ( DoubleNear(sp->nextcp.y,sp->me.y) &&
			DoubleNear(sp->prevcp.y,sp->me.y) &&
			DoubleNear(np->me.y,pp->me.y) &&
			!DoubleNear(np->me.y,sp->me.y) &&
			np->me.y-sp->me.y < blueshift &&
			np->me.y-sp->me.y > -blueshift ) {
		    if ( (np->me.y>sp->me.y &&
			    np->prevcp.y<=np->me.y && np->prevcp.y>=sp->me.y &&
			    pp->nextcp.y<=pp->me.y && pp->nextcp.y>=sp->me.y ) ||
			  (np->me.y<sp->me.y &&
			    np->prevcp.y>=np->me.y && np->prevcp.y<=sp->me.y &&
			    pp->nextcp.y>=pp->me.y && pp->nextcp.y<=sp->me.y )) {
			sp->flexy = true;
			val = np->me.y-sp->me.y;
		    }
		}
		if ( val<0 ) val = -val;
		if ( val>max ) max = val;
	    }
	    sp = np;
	} while ( sp!=spl->first );
    }
return( max );
}

int SplineFontIsFlexible(SplineFont *sf) {
    int i;
    int max=0, val;
    char *pt;
    int blueshift;
    /* if the return value is bigger than 6 and we don't have a BlueShift */
    /*  then we must set BlueShift to ret+1 before saving private dictionary */
    /* If the first point in a spline set is flexible, then we must rotate */
    /*  the splineset */

    pt = PSDictHasEntry(sf->private,"BlueShift");
    blueshift = 21;		/* maximum posible flex, not default */
    if ( pt!=NULL ) {
	blueshift = strtol(pt,NULL,10);
	if ( blueshift>21 ) blueshift = 21;
    } else if ( PSDictHasEntry(sf->private,"BlueValues")!=NULL )
	blueshift = 7;	/* The BlueValues array may depend on BlueShift having its default value */

    for ( i=0; i<sf->charcnt; ++i )
	if ( sf->chars[i]!=NULL && sf->chars[i]->origtype1==NULL ) {
	    val = SplineCharIsFlexible(sf->chars[i],blueshift);
	    if ( val>max ) max = val;
	}
return( max );
}
