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
#include "edgelist.h"
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
	    if ( isalnum(enc) && enc>=32 && enc<128 ) {
		any = true;
		SplineCharFindBounds(sf->chars[i],&b);
		if ( b.miny==0 && b.maxy==0 )
	continue;
		if ( enc=='g' || enc=='j' || enc=='p' || enc=='q' || enc=='y' ) {
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
			enc == 'l' || enc == 't' ) {
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
	if ( isalnum(enc) && enc>=32 && enc<128 ) {
	    any = true;
	    SplineCharFindBounds(sf->chars[i],&b);
	    if ( b.miny==0 && b.maxy==0 )
    continue;
	    if ( enc=='g' || enc=='j' || enc=='p' || enc=='q' || enc=='y' ) {
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
		    enc == 'l' || enc == 't' ) {
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

typedef struct steminfo {
    double start;			/* location at which the stem starts */
    double width;			/* or height */
    double len;				/* How long the stem is in the direction perp. to the "width" */
		    /* Short stems (in this sense), like serifs, aren't as interesting as longer ones */
    double otherstart;			/* and where it starts in the perp direction */
    struct steminfo *next;
    unsigned int toobig:1;	/* get rid of this guy if it overlaps another */
    unsigned int haspoints:1;	/* both edges of the stem have points on them */
				/*  at the stem left and right edge */
			        /*  trivially true for horizontal/vertical lines */
			        /*  true for curves if extrema have points */
			        /*  except we aren't smart enough to detect that yet */
} StemInfo ;

static void StemInfoFree(StemInfo *stems) {
    StemInfo *next;
    while ( stems!=NULL ) {
	next = stems->next;
	free( stems );
	stems = next;
    }
}

/* If the stem is vertical here, return an approximate height over which it is */
/* if major is 1 then we check for vertical lines, if 0 then horizontal */
static double IsEdgeHorVertHere(Edge *e,int major) {
    /* It's vertical if dx/dt == 0 or if (dy/dt)/(dx/dt) is infinite */
    Spline *spline = e->spline;
    Spline1D *msp = &spline->splines[major], *osp = &spline->splines[!major];
    double dodt = (3*osp->a*e->t_cur+2*osp->b)*e->t_cur + osp->c;
    double dmdt = (3*msp->a*e->t_cur+2*msp->b)*e->t_cur + msp->c;
    double tmax, tmin, t, o;
    double dup, ddown;

    if ( dodt<0 ) dodt = -dodt;
    if ( dmdt<0 ) dmdt = -dmdt;
    if ( dodt>.01 && dmdt/dodt<10 )		/* very rough defn of 0 and infinite */
return( 0.0 );

    if ( osp->a==0 && osp->b==0 && osp->c==0 )
return( e->mmax-e->mmin );

    dup = ddown = 0;
    tmax = e->t_mmax;
    tmin = e->t_cur;
    if ( e->t_mmax<e->t_cur ) {
	tmax = e->t_mmin;
    }
    while ( tmax-tmin>.0005 ) {
	t = (tmin+tmax)/2;
	o = ((osp->a*t+osp->b)*t+osp->c)*t + osp->d;
	if ( o-e->o_cur<1 && o-e->o_cur>-1 ) {
	    dup = ((msp->a*t+msp->b)*t+msp->c)*t+msp->d-e->m_cur;
	    if ( tmax-tmin<.001 )
    break;
	    tmin = t;
	} else
	    tmax = t;
    }
    tmin = e->t_mmin;
    tmax = e->t_cur;
    if ( e->t_mmax<e->t_cur ) {
	tmin = e->t_mmax;
    }
    while ( tmax-tmin>.0005 ) {
	t = (tmin+tmax)/2;
	o = ((osp->a*t+osp->b)*t+osp->c)*t + osp->d;
	if ( o-e->o_cur<1 && o-e->o_cur>-1 ) {
	    ddown = ((msp->a*t+msp->b)*t+msp->c)*t+msp->d-e->m_cur;
	    if ( tmax-tmin<.001 )
    break;
	    tmax = t;
	} else
	    tmin = t;
    }
    if ( dup<0 ) dup= -dup;
    if ( ddown<0 ) ddown = -ddown;
return( dup+ddown );
}

static StemInfo *InStemList(StemInfo *stems, double start, double width) {
    while ( stems!=NULL ) {
	if ( DoubleNear(stems->start,start) && DoubleNear(stems->width,width))
return( stems );
	stems = stems->next;
    }
return( NULL );
}

static StemInfo *_FigureStems(EdgeList *es) {
    Edge *active=NULL, *apt, *pr, *e, *prev, *test;
    int i;
    double l1, l2;
    StemInfo *head=NULL, *last=NULL, *cur;
    StemInfo *old;

    for ( i=0; i<es->cnt; ++i ) {
	if ( es->edges[i]==NULL && !es->interesting[i] &&/* interesting things happen when we add (or remove) entries */
		(i<=0 || es->interesting[i-1]))
    continue;			/* and where we have points of inflection */
	active = ActiveEdgesRefigure(es,active,i);
	for ( apt=active; apt!=NULL; apt = e) {
	    e = ActiveEdgesFindStem(apt,&prev, i);
	    pr = prev;
	    if ( apt==pr )
	break;
	    /* Guestimate the length of the vertical segment */
	    /* If we are at a vertex then sum the vertical bits from the two adjacent splines */
	    /* This fails if another edge interveins between the two edges we */
	    /* are looking at (we will think the length too long) */
	    /* Example of this is the crossbar of the H. We will think that */
	    /*  the stem from the extreme left to the extreme right lasts the */
	    /*  entire height of the character, instead of the crossbar height*/
	    l1 = IsEdgeHorVertHere(apt,es->major);
	    test = apt->aenext;
	    if ( test!=NULL && test->up==apt->up &&
		    ( apt->before==test || apt->after==test ) &&
		    ( apt->spline==test->spline ||
			apt->spline->to->next==test->spline ||
			apt->spline->from->prev==test->spline ) &&
		    (( apt->mmax==i && test->mmin==i ) ||
		     ( apt->mmin==i && test->mmax==i )) )
		l1 += IsEdgeHorVertHere(test,es->major);
	    l2 = IsEdgeHorVertHere(pr,es->major);
	    for ( test=apt; test->aenext!=pr; test=test->aenext);
	    if ( test!=apt && test!=NULL && test->up==pr->up &&
		    ( pr->before==test || pr->after==test ) &&
		    ( pr->spline==test->spline ||
			pr->spline->to->next==test->spline ||
			pr->spline->from->prev==test->spline ) &&
		    (( pr->mmax==i && test->mmin==i ) ||
		     ( pr->mmin==i && test->mmax==i )) )
		l2 += IsEdgeHorVertHere(test,es->major);
	    if ( l1!=0 && l2!=0 ) {
		double start = rint(apt->o_cur), width = rint(pr->o_cur-apt->o_cur);
		if ( l1>l2 ) l1 = l2;
		if ( width>0 ) {
		    if ( (old = InStemList(head,start,width))==NULL ) {
			double fudge = (es->omax-es->omin)/16;
			cur = galloc(sizeof(StemInfo));
			cur->start = start;
			cur->width = width;
			cur->len = l1;
			cur->next = NULL;
			cur->toobig =  ( start<=es->omin+fudge && start+width>=es->omax-fudge );
			cur->otherstart = es->mmin+i;
			cur->haspoints = (apt->mmin==apt->mmax) && (pr->mmin==pr->mmax);
			if ( !cur->haspoints ) {
			    if ( es->major==0 ) {
				if ( ((rint(apt->spline->from->me.x)==rint(es->mmin+i)) ||
					    (rint(apt->spline->to->me.x)==rint(es->mmin+i))) &&
					((rint(pr->spline->from->me.x)==rint(es->mmin+i)) ||
					    (rint(pr->spline->to->me.x)==rint(es->mmin+i))) )
				    cur->haspoints = true;
			    } else {
				if ( ((rint(apt->spline->from->me.y)==rint(es->mmin+i)) ||
					    (rint(apt->spline->to->me.y)==rint(es->mmin+i))) &&
					((rint(pr->spline->from->me.y)==rint(es->mmin+i)) ||
					    (rint(pr->spline->to->me.y)==rint(es->mmin+i))) )
				    cur->haspoints = true;
			    }
			}
			if ( head==NULL )
			    head = last = cur;
			else {
			    last->next = cur;
			    last = cur;
			}
		    } else if ( old->otherstart+old->len<=es->mmin+i ) {
			/* if a stem is broken into bits (as the stems in H are) */
			/*  add the bits together */
			old->len += l1;
		    }
		}
	    }
	}
    }
return( head );
}

static StemInfo *FigureVStems(SplineChar *sc) {
    EdgeList es;
    DBounds b;
    StemInfo *ret;

    SplineCharFindBounds(sc,&b);
    memset(&es,'\0',sizeof(es));
    es.scale = 1.0;
    es.mmin = floor(b.miny*es.scale);
    es.mmax = ceil(b.maxy*es.scale);
    es.omin = b.minx*es.scale;
    es.omax = b.maxx*es.scale;
    es.cnt = (int) (es.mmax-es.mmin) + 1;
    es.edges = gcalloc(es.cnt,sizeof(Edge *));
    es.interesting = gcalloc(es.cnt+1,sizeof(char));
    es.sc = sc;
    es.major = 1; es.other = 0;

    FindEdgesSplineSet(sc->splines,&es);
    ret = _FigureStems(&es);
    FreeEdges(&es);
return( ret );
}

static StemInfo *FigureHStems(SplineChar *sc) {
    EdgeList es;
    DBounds b;
    StemInfo *ret;

    SplineCharFindBounds(sc,&b);
    memset(&es,'\0',sizeof(es));
    es.scale = 1.0;
    es.omin = floor(b.miny*es.scale);
    es.omax = ceil(b.maxy*es.scale);
    es.mmin = b.minx*es.scale;
    es.mmax = b.maxx*es.scale;
    es.cnt = (int) (es.mmax-es.mmin) + 1;
    es.edges = gcalloc(es.cnt,sizeof(Edge *));
    es.interesting = gcalloc(es.cnt+1,sizeof(char));
    es.sc = sc;
    es.major = 0; es.other = 1;

    FindEdgesSplineSet(sc->splines,&es);
    ret = _FigureStems(&es);
    FreeEdges(&es);
return( ret );
}

/* Look through the stem list. If there are any stems that overlap */
/*  for example a stem and its serif might both get StemInfos and they'd overlap*/
/*  then the interesting stem is the longer one */
/* Well in the case of "H" we get three vertical stems, the obvious two for the*/
/*  stems of the H, and a "stem" going from the extreme left edge to the extreme*/
/*  right edge. We get its length wrong too, so it appears bigger than the real*/
/*  stems. But I think if we have two overlapping stems, in general picking */
static StemInfo *StripNestedStems(StemInfo *stem) {
    StemInfo *head, *prev, *test, *pt, *snext, *tnext;
    int oo;

    head = stem;
    prev = NULL;
    while ( stem!=NULL ) {
	snext = stem->next;
	pt = stem;
	for ( test=snext; test!=NULL; test=tnext ) {
	    tnext = test->next;
	    /* overlap in the other dimension */
	    oo = !(stem->otherstart+stem->len<test->otherstart||
		        test->otherstart+test->len<stem->otherstart);
	    if ( stem->start+stem->width<=test->start ||
		    stem->start>=test->start+test->width )
		pt = test;	/* No overlap */
	    else if ( test->toobig || (!stem->toobig &&
		    ((!oo && stem->len>=test->len) ||
		     (oo && stem->haspoints && !test->haspoints ) ||
		     (oo && !(!stem->haspoints && test->haspoints) &&
			     stem->width<test->width )))) {
		pt->next = tnext;
		free(test);
		if ( test==snext ) snext = tnext;
	    } else {
		if ( prev==NULL )
		    head = snext;
		else
		    prev->next = snext;
		free(stem);
		stem = NULL;
	break;
	    }
	}
	if ( stem!=NULL )
	    prev = stem;
	stem = snext;
    }
return( head );
}

static StemInfo *StripTooBigs(StemInfo *head, SplineFont *sf, int hstem) {
    /* a hint that's the full height of a character isn't a good idea */
    /* instead the postscript convention is to create two little hints at the*/
    /*  top and bottom */
    if ( head==NULL )
return( NULL );
    if ( head->toobig && head->next==NULL &&
	    head->width>(sf->ascent+sf->descent)/3 ) {
	if ( hstem ) {
	    StemInfo *next;
	    next = gcalloc(1,sizeof(StemInfo));
	    next->width = -20;
	    next->start = head->width + head->start;
	    head->width = 20;
	    head->next = next;
	} else {
	    StemInfoFree(head);
	    head = NULL;
	}
    }
return( head );
}

static Hints *StemsToHints( StemInfo *stems ) {
    Hints *head=NULL, *cur, *test, *prev;
    /* Convert the stem info structures into hints, and then order then hints */

    while ( stems!=NULL ) {
	cur = gcalloc(1,sizeof(Hints));
	cur->base = stems->start;
	cur->width = stems->width;
	if ( head==NULL )
	    head = cur;
	else if ( cur->base<head->base ) {
	    cur->next = head;
	    head = cur;
	} else {
	    for ( prev=head, test=head->next; test!=NULL && test->base<cur->base;
		    prev=test, test=test->next);
	    prev->next = cur;
	    cur->next = test;
	}
	stems = stems->next;
    }
return( head );
}

static Hints *HHintFixup( Hints *hint ) {
    /* If we have multiple hstems stacked vertically, then assume that the */
    /*  last one is probably in the bluezone of the top of the character, and */
    /*  as such the interesting thing is the top of the stem, not the bottom */
    /*  so reverse it (ie. make it be a stem with a negative width) */
    Hints *first;

    if ( hint==NULL || hint->next==NULL )
return(hint);
    first = hint;
    while ( hint->next!=NULL )
	hint = hint->next;
    if ( hint->width>0 ) {
	hint->base += hint->width;
	hint->width = -hint->width;
    }
return( first );
}

static void FindStems( SplineFont *sf, double snaps[12], double cnts[12],
	StemInfo *(*Figure)(SplineChar *) ) {
    int i, j, k, cnt, smax=0, smin=1000;
    double stemwidths[1000];
    StemInfo *stems, *test;

    memset(stemwidths,'\0',sizeof(stemwidths));

    for ( i=0; i<sf->charcnt; ++i ) {
	if ( sf->chars[i]!=NULL ) {
	    stems = StripNestedStems(Figure(sf->chars[i]));
	    for ( test=stems; test!=NULL; test = test->next ) {
		if ( (j=test->width)<1000 ) {
		    stemwidths[j] += test->len;
		    if ( smax<j ) smax=j;
		    if ( smin>j ) smin=j;
		}
	    }
	    if ( Figure==FigureHStems && sf->chars[i]->changedsincelasthhinted ) {
		stems = StripTooBigs(stems,sf,true);
		HintsFree(sf->chars[i]->hstem);
		sf->chars[i]->hstem = HHintFixup(StemsToHints(stems));
		sf->chars[i]->changedsincelasthhinted = false;
	    } else if ( Figure==FigureVStems && sf->chars[i]->changedsincelastvhinted ) {
		stems = StripTooBigs(stems,sf,false);
		HintsFree(sf->chars[i]->vstem);
		sf->chars[i]->vstem = StemsToHints(stems);
		sf->chars[i]->changedsincelastvhinted = false;
	    }
	    StemInfoFree(stems);
	}
	GProgressNext();
    }
    for ( i=smin, cnt=0; i<=smax; ++i ) {
	if ( stemwidths[i]!=0 )
	    ++cnt;
    }
    if ( cnt>12 ) {
	/* Merge adjacent widths */
	for ( i=smin; i<=smax; ++i ) {
	    if ( stemwidths[i]!=0 && i<=smax-1 && stemwidths[i+1]!=0 ) {
		if ( stemwidths[i]>stemwidths[i+1] ) {
		    stemwidths[i] += stemwidths[i+1];
		    stemwidths[i+1] = 0;
		} else {
		    if ( i<=smax-2 && stemwidths[i+2] && stemwidths[i+2]<stemwidths[i+1] ) {
			stemwidths[i+1] += stemwidths[i+2];
			stemwidths[i+2] = 0;
		    }
		    stemwidths[i+1] += stemwidths[i];
		    stemwidths[i] = 0;
		    ++i;
		}
	    }
	}
	for ( i=smin, cnt=0; i<=smax; ++i ) {
	    if ( stemwidths[i]!=0 )
		++cnt;
	}
    }
    if ( cnt<=12 ) {
	for ( i=smin, cnt=0; i<=smax; ++i ) {
	    if ( stemwidths[i]!=0 ) {
		snaps[cnt] = i;
		cnts[cnt++] = stemwidths[i];
	    }
	}
    } else { double firstbiggest=0;
	for ( cnt = 0; cnt<12; ++cnt ) {
	    int biggesti=0;
	    double biggest=0;
	    for ( i=smin; i<=smax; ++i ) {
		if ( stemwidths[i]>biggest ) { biggest = stemwidths[i]; biggesti=i; }
	    }
	    /* array must be sorted */
	    if ( biggest<firstbiggest/6 )
	break;
	    for ( j=0; j<cnt; ++j )
		if ( snaps[j]>biggesti )
	    break;
	    for ( k=cnt-1; k>=j; --k ) {
		snaps[k+1] = snaps[k];
		cnts[k+1]=cnts[k];
	    }
	    snaps[j] = biggesti;
	    cnts[j] = biggest;
	    stemwidths[biggesti] = 0;
	    if ( firstbiggest==0 ) firstbiggest = biggest;
	}
    }
    for ( ; cnt<12; ++cnt ) {
	snaps[cnt] = 0;
	cnts[cnt] = 0;
    }
}

void FindHStems( SplineFont *sf, double snaps[12], double cnt[12]) {
    FindStems(sf,snaps,cnt,FigureHStems);
}

void FindVStems( SplineFont *sf, double snaps[12], double cnt[12]) {
    FindStems(sf,snaps,cnt,FigureVStems);
}

static Hints *RefHintsMerge(Hints *into, Hints *rh, double mul, double offset) {
    Hints *prev, *h, *n;
    double base, width;

    for ( ; rh!=NULL; rh=rh->next ) {
	base = rh->base*mul + offset;
	width = rh->width *mul;
	if ( width<0 ) {
	    base += width; width = -width;
	}
	for ( h=into, prev=NULL; h!=NULL; prev=h, h=h->next ) {
	    if ( base<h->base || base<h->base+h->width )
	break;
	}
	if ( h==NULL || base+width<h->base ) {
	    n = gcalloc(1,sizeof(Hints));
	    n->base = base; n->width = width;
	    n->next = h;
	    if ( prev==NULL )
		into = n;
	    else
		prev->next = n;
	}
    }
return( into );
}

static void AutoHintRefs(SplineChar *sc) {
    RefChar *ref;

    /* Add hints for base characters before accent hints => if there are any */
    /*  conflicts, the base characters win */
    for ( ref=sc->refs; ref!=NULL; ref=ref->next ) {
	if ( ref->transform[1]==0 && ref->transform[2]==0 ) {
	    if ( !ref->sc->manualhints &&
		    ( ref->sc->changedsincelasthhinted || ref->sc->changedsincelastvhinted ))
		SplineCharAutoHint(ref->sc);
	    if ( ref->sc->unicodeenc!=-1 && isalnum(ref->sc->unicodeenc) ) {
		sc->hstem = RefHintsMerge(sc->hstem,ref->sc->hstem,ref->transform[3], ref->transform[5]);
		sc->vstem = RefHintsMerge(sc->vstem,ref->sc->vstem,ref->transform[0], ref->transform[4]);
	    }
	}
    }

    for ( ref=sc->refs; ref!=NULL; ref=ref->next ) {
	if ( ref->transform[1]==0 && ref->transform[2]==0 ) {
	    sc->hstem = RefHintsMerge(sc->hstem,ref->sc->hstem,ref->transform[3], ref->transform[5]);
	    sc->vstem = RefHintsMerge(sc->vstem,ref->sc->vstem,ref->transform[0], ref->transform[4]);
	}
    }
}

void SplineCharAutoHint( SplineChar *sc) {
    StemInfo *stems;

    sc->changedsincelasthhinted = false;
    sc->changedsincelastvhinted = false;
    sc->manualhints = false;
    if ( sc->origtype1!=NULL )
	SCClearOrig(sc);

    HintsFree(sc->vstem);
    sc->vstem = NULL;
    stems = StripTooBigs(StripNestedStems(FigureVStems(sc)),sc->parent,false);
    sc->vstem = StemsToHints(stems);
    StemInfoFree(stems);

    HintsFree(sc->hstem);
    sc->hstem = NULL;
    stems = StripTooBigs(StripNestedStems(FigureHStems(sc)),sc->parent,true);
    sc->hstem = HHintFixup(StemsToHints(stems));
    StemInfoFree(stems);

    AutoHintRefs(sc);

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
