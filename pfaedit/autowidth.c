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
#include "pfaeditui.h"
#include "ustring.h"
#include "utype.h"
#include <math.h>

/* The basic idea behind autowidth:
    Every character is divided into n zones:
	serif zone: (includes descender, baseline, xheight and cap height serifs)
	ascender zone: from x-height to ascender serif
	descender zone: from baseline to descender serif
	low-x zone: 0-1/3 of way from baseline to x-height serifs
	mid-x zone: 1/3-2/3 of way ditto
	high-x zone: 2/3-1 of way ditto
    For each character pair:
	For each zone
	    Figure the max x position of the left character in that zone
		    (subtract from rmax of char)
		Figure the x position of the right char at that height
		    (subtract lbearing (==lmin))
		Store the distance between them (distance if l/r bearings==0)
	    Figure the min x position of the right char in that zone (-lbearing)
		Figure the x position of the left char at that height (rbear-)
		Store this distance.
	End for
    End for
    Average all the distances that make sense
	(ie. don't use ascender zone for "lo" combination where one char has no
	 ascender, or descender for "ei" where there is no descender)
Er...
    Now we have a set of equations one for every character pair:
	<left char's rbearing> + <right char's lbearing> + <average> ~=
		<desired spacing>
    Apply least squares and minimize (rb, lb)
	Sigma (<dspace>-(rb+lb+average))^2
Ah,
    Well it was a lovely theory, but I couldn't get it to work.
    Instead I average the averages above subtract that from each char's average
	and use that to adjust the spacing.

    Check if any character would end up less than <desired space>/<fudge> in any zone
	If so adjust its rbearing so it's that far away.
    Then for every character shift it over so that its lbearing matches the
	optimum lbearing
    (for any dependant characters, shift everything else in the character
	over by the same amount, unless the depended character is included
	in the set in which case unshift the reference to this character
	by the lbearing, since it will be adjusted assuming our character
	is where it used to be)
    No, I think that is too complex. Probably best just to warn if a char and
	its dependents are included in the same set, and always shift the rest
	of the dependant over.
    For every character shift its rbearing (=>changing width)
    Do our standard width fix up.
    Set all previous kern pairs to zero.
===================================
Autokern has similar ideas, but is simpler:
    Once we've found the average of the zone distances
    Adjust the kerning between the two characters so that
	<kern> = <dspace>-(<rb>-<lb>-<average>)
	Again check for overlap (<dspace>/4 in any zone)
    Kerning by the left character can always propigate to dependents
    Kerning by the right char should not
	(ie. A and À kern the same with V
	 but V kerns differently with e and è-- the accent gets in the way)
    No, I think it is better not to propigate kerning.
*/

enum zones { z_low, z_mid, z_high, z_ascend, z_descend,
	z_serifbase, z_serifxh, z_serifas, z_serifds, z_max };

struct charone {
    real lbearing, rmax;
    real newl, newr;
    SplineChar *sc;
    real lzones[z_max][2];	/* min x values/zone with corresponding y vals */
    real rzones[z_max][2];	/* max x values/zone with corresponding y vals */
    struct charpair *asleft;
    struct charpair *asright;
};

struct charpair {
    struct charone *left, *right;
    struct charpair *nextasleft, *nextasright;
    real zones[z_max];
    real average;
};

typedef struct widthinfo {
    real spacing;		/* desired spacing between letters */
    int zones[z_max][2];	/* low and high for each zone */
    int lcnt, rcnt;		/* count of left and right chars respectively */
    int tcnt;			/* sum of r+l cnt */
    struct charone **left, **right;
    struct charpair **pairs;
    int space_guess;
    int threshold;
    SplineFont *sf;
    int done: 1;
    int autokern: 1;
} WidthInfo;

#define NOTREACHED	-99999.0

static real LineFindLeftDistance(struct charone *right,WidthInfo *wi);
static real LineFindRightDistance(struct charone *left,WidthInfo *wi);

static void FigureLR(WidthInfo *wi) {
    int i;
    real lsum, rsum, sum, subsum;
    struct charone *ch;
    struct charpair *cp;

    lsum = rsum = 0;
    for ( i=0; i<wi->lcnt; ++i )
	lsum += LineFindRightDistance(wi->left[i],wi);
    for ( i=0; i<wi->rcnt; ++i )
	rsum += LineFindLeftDistance(wi->right[i],wi);
    lsum /= wi->lcnt; rsum /= wi->rcnt;
    /* Er... a fudge. to make things look right for Caliban */
    sum = 0;
    for ( i=0; i<wi->lcnt*wi->rcnt; ++i )
	sum += wi->pairs[i]->average;
    sum /= (wi->lcnt*wi->rcnt);
    lsum = (sum/2+lsum)/1.5; rsum = (sum/2+rsum)/1.5;

    for ( i=0; i<wi->lcnt; ++i ) {
	ch = wi->left[i];
	subsum = 0;
	for ( cp = ch->asleft; cp!=NULL; cp = cp->nextasleft )
	    subsum += cp->average;
	subsum /= wi->rcnt;
	if ( ch->sc->enc=='I' )
	    ch->sc->enc = 'I';
	ch->newr = (wi->spacing-subsum+rsum)*2/3.0;
    }
    for ( i=0; i<wi->rcnt; ++i ) {
	ch = wi->right[i];
	subsum = 0;
	for ( cp = ch->asright; cp!=NULL; cp = cp->nextasright )
	    subsum += cp->average;
	subsum /= wi->lcnt;
	ch->newl = (wi->spacing-subsum+lsum)/3.0;
    }
}

static void CheckOutOfBounds(WidthInfo *wi) {
    int i,j;
    struct charpair *cp;
    real min=NOTREACHED, temp, lr;
    real minsp = wi->spacing/3;

    for ( i=0; i<wi->rcnt; ++i ) {
	if ( wi->right[i]->newl<-wi->spacing || wi->right[i]->newl>wi->spacing )
	    fprintf( stderr, "AutoWidth failure on %s\n", wi->right[i]->sc->name );
	if ( wi->right[i]->newl<-minsp )
	    wi->right[i]->newl = -rint(minsp);
    }
    for ( i=0; i<wi->lcnt; ++i ) {
	if ( wi->left[i]->newr<-wi->spacing || wi->left[i]->newr>wi->spacing ) {
	    fprintf( stderr, "AutoWidth failure on %s\n", wi->right[i]->sc->name );
	    if ( wi->left[i]->newr>wi->spacing )
		wi->left[i]->newr = wi->spacing;
	}
    }
    for ( i=0; i<wi->lcnt*wi->rcnt; ++i ) {
	cp = wi->pairs[i];
	lr = cp->left->newr + cp->right->newl;
	min = NOTREACHED;
	for ( j=0; j<z_max; ++j ) if ( cp->zones[j]!=NOTREACHED ) {
	    temp = lr + cp->zones[j];
	    if ( min==NOTREACHED || temp<min )
		min = temp;
	}
	if ( min!=NOTREACHED && min<minsp )
	    cp->left->newr += rint(minsp-min);
    }
}

static void ApplyChanges(WidthInfo *wi) {
    char *rsel = gcalloc(wi->sf->charcnt,sizeof(char));
    int i, width;
    real transform[6];
    struct charone *ch;
    DBounds bb;

    for ( i=0; i<wi->rcnt; ++i )
	rsel[wi->right[i]->sc->enc] = true;
    transform[0] = transform[3] = 1.0;
    transform[1] = transform[2] = transform[5] = 0;

    for ( i=0; i<wi->rcnt; ++i ) {
	ch = wi->right[i];
	transform[4] = ch->newl-ch->lbearing;
	if ( transform[4]!=0 ) {
	    FVTrans(wi->sf->fv,ch->sc,transform,rsel,false);
	    SCCharChangedUpdate(ch->sc);
	}
    }
    free(rsel);

    for ( i=0; i<wi->lcnt; ++i ) {
	ch = wi->left[i];
	SplineCharFindBounds(ch->sc,&bb);
	width = rint(bb.maxx + ch->newr);
	if ( width!=ch->sc->width ) {
	    SCPreserveWidth(ch->sc);
	    SCSynchronizeWidth(ch->sc,width,ch->sc->width,wi->sf->fv);
	    SCCharChangedUpdate(ch->sc);
	}
    }
}

static void AutoWidth(WidthInfo *wi) {
    FigureLR(wi);
    CheckOutOfBounds(wi);
    ApplyChanges(wi);
}

static void AutoKern(WidthInfo *wi) {
    struct charpair *cp;
    SplineChar *lsc, *rsc;
    int i, j, diff;
    KernPair *kp;
    MetricsView *mv;
    real minsp = wi->spacing/3, min, max, temp;

    for ( i=0; i<wi->lcnt*wi->rcnt; ++i ) {
	cp = wi->pairs[i];

	diff = rint( wi->spacing-(cp->left->sc->width-cp->left->rmax+cp->right->lbearing+cp->average));
	min = NOTREACHED;
	max = NOTREACHED;
	for ( j=0; j<z_max; ++j ) if ( cp->zones[j]!=NOTREACHED ) {
	    temp = diff + cp->zones[j];
	    if ( min==NOTREACHED || temp<min )
		min = temp;
	    if ( max==NOTREACHED || temp>max )
		max = temp;
	}
	if ( max!=NOTREACHED && max>2*wi->spacing ) {
	    if ( diff>0 && diff + rint(2*wi->spacing-max)<0 )
		diff = 0;
	    else {
		diff += rint(2*wi->spacing-max);
		min += rint(2*wi->spacing-max);
	    }
	}
	if ( min!=NOTREACHED && min<minsp && diff!=0 ) {
	    if ( diff<0 && diff + rint(minsp-min)>0 )
		diff = 0;
	    else
		diff += rint(minsp-min);
	}

	if ( wi->threshold!=0 && diff>-wi->threshold && diff<wi->threshold )
	    diff = 0;
	lsc = cp->left->sc;
	rsc = cp->right->sc;
	for ( kp = lsc->kerns; kp!=NULL && kp->sc!=rsc; kp = kp->next );
	if ( kp!=NULL ) {
	    if ( kp->off!=diff ) {
		kp->off = diff;
		wi->sf->changed = true;
	    }
	} else if ( diff!=0 ) {
	    kp = gcalloc(1,sizeof(KernPair));
	    kp->sc = rsc;
	    kp->off = diff;
	    kp->next = lsc->kerns;
	    lsc->kerns = kp;
	    wi->sf->changed = true;
	}
    }
    for ( mv=wi->sf->fv->metrics; mv!=NULL; mv=mv->next )
	MVReKern(mv);
}

static real SplineFindMaxXAtY(Spline *spline,real y,real max) {
    real t,t1,t2,tbase,val;
    Spline1D *xsp;

    if ( y>spline->from->me.y && y>spline->from->nextcp.y &&
	    y>spline->to->me.y && y>spline->to->prevcp.y )
return( max );
    if ( y<spline->from->me.y && y<spline->from->nextcp.y &&
	    y<spline->to->me.y && y<spline->to->prevcp.y )
return( max );
    if ( max!=NOTREACHED ) {
	if ( max>=spline->from->me.x && max>=spline->from->nextcp.x &&
		max>=spline->to->me.x && max>=spline->to->prevcp.x )
return( max );
    }

    xsp = &spline->splines[0];
    SplineFindInflections(&spline->splines[1], &t1, &t2 );
    tbase = 0;
    if ( t1!=-1 ) {
	t = SplineSolve(&spline->splines[1],0,t1,y,.01);
	if ( t>=0 && t<=1 ) {
	    val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
	    if ( max==NOTREACHED || val>max )
		max = val;
	}
	tbase = t1;
    }
    if ( t2!=-1 ) {
	t = SplineSolve(&spline->splines[1],tbase,t2,y,.01);
	if ( t>=0 && t<=1 ) {
	    val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
	    if ( max==NOTREACHED || val>max )
		max = val;
	}
	tbase = t2;
    }
    t = SplineSolve(&spline->splines[1],tbase,1.0,y,.01);
    if ( t>=0 && t<=1 ) {
	val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
	if ( max==NOTREACHED || val>max )
	    max = val;
    }
return( max );
}

static real SplineFindMinXAtY(Spline *spline,real y,real min) {
    real t,t1,t2,tbase,val;
    Spline1D *xsp;

    if ( y>spline->from->me.y && y>spline->from->nextcp.y &&
	    y>spline->to->me.y && y>spline->to->prevcp.y )
return( min );
    if ( y<spline->from->me.y && y<spline->from->nextcp.y &&
	    y<spline->to->me.y && y<spline->to->prevcp.y )
return( min );
    if ( min!=NOTREACHED ) {
	if ( min<=spline->from->me.x && min<=spline->from->nextcp.x &&
		min<=spline->to->me.x && min<=spline->to->prevcp.x )
return( min );
    }

    xsp = &spline->splines[0];
    SplineFindInflections(&spline->splines[1], &t1, &t2 );
    tbase = 0;
    if ( t1!=-1 ) {
	t = SplineSolve(&spline->splines[1],0,t1,y,.01);
	if ( t>=0 && t<=1 ) {
	    val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
	    if ( min==NOTREACHED || val<min )
		min = val;
	}
	tbase = t1;
    }
    if ( t2!=-1 ) {
	t = SplineSolve(&spline->splines[1],tbase,t2,y,.01);
	if ( t>=0 && t<=1 ) {
	    val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
	    if ( min==NOTREACHED || val<min )
		min = val;
	}
	tbase = t2;
    }
    t = SplineSolve(&spline->splines[1],tbase,1.0,y,.01);
    if ( t>=0 && t<=1 ) {
	val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
	if ( min==NOTREACHED || val<min )
	    min = val;
    }
return( min );
}

static void PtFillZones(real x, real y,struct charone *ch, WidthInfo *wi) {
    int i;

    for ( i=0; i<z_max; ++i )
	if ( y>=wi->zones[i][0] && y<wi->zones[i][1] ) {
	    if ( ch->lzones[i][0]==NOTREACHED || x<ch->lzones[i][0] ) {
		ch->lzones[i][0] = x;
		ch->lzones[i][1] = y;
	    }
	    if ( ch->rzones[i][0]==NOTREACHED || x>ch->rzones[i][0] ) {
		ch->rzones[i][0] = x;
		ch->rzones[i][1] = y;
	    }
return;
	}
}

static void SplineFillZones(Spline *spline,struct charone *ch, WidthInfo *wi) {
    Spline1D *xsp, *ysp;
    real t1, t2, tbase, t, val;
    int i, j;

    /* first try the end points */
    PtFillZones(spline->to->me.x,spline->to->me.y,ch,wi);
    PtFillZones(spline->from->me.x,spline->from->me.y,ch,wi);

    /* then try the extrema of the spline (assuming they are between t=(0,1) */
    xsp = &spline->splines[0]; ysp = &spline->splines[1];
    SplineFindInflections(xsp, &t1, &t2 );
    if ( t1!=-1 )
	PtFillZones( ((xsp->a*t1+xsp->b)*t1+xsp->c)*t1+xsp->d,
		((ysp->a*t1+ysp->b)*t1+ysp->c)*t1+ysp->d,
		ch,wi);
    if ( t2!=-1 )
	PtFillZones( ((xsp->a*t2+xsp->b)*t2+xsp->c)*t2+xsp->d,
		((ysp->a*t2+ysp->b)*t2+ysp->c)*t2+ysp->d,
		ch,wi);

    /* then try the zone boundaries */
    SplineFindInflections(ysp, &t1, &t2 );
    for ( i=0; i<z_max; ++i ) {
	if ( wi->zones[i][0]>spline->from->me.y && wi->zones[i][0]>spline->from->nextcp.y &&
		wi->zones[i][0]>spline->to->me.y && wi->zones[i][0]>spline->to->prevcp.y )
    continue;
	if ( wi->zones[i][1]<spline->from->me.y && wi->zones[i][1]<spline->from->nextcp.y &&
		wi->zones[i][1]<spline->to->me.y && wi->zones[i][1]<spline->to->prevcp.y )
    continue;
	for ( j=0; j<2; ++j ) {
	    tbase = 0;
	    if ( t1!=-1 ) {
		t = SplineSolve(&spline->splines[1],0,t1,wi->zones[i][j],.01);
		if ( t>=0 && t<=1 ) {
		    val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
		    if ( ch->lzones[i][0]==NOTREACHED || val<ch->lzones[i][0] ) {
			ch->lzones[i][0] = val;
			ch->lzones[i][1] = wi->zones[i][j];
		    }
		    if ( ch->rzones[i][0]==NOTREACHED || val>ch->rzones[i][0] ) {
			ch->rzones[i][0] = val;
			ch->rzones[i][1] = wi->zones[i][j];
		    }
		}
		tbase = t1;
	    }
	    if ( t2!=-1 ) {
		t = SplineSolve(&spline->splines[1],tbase,t2,wi->zones[i][j],.01);
		if ( t>=0 && t<=1 ) {
		    val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
		    if ( ch->lzones[i][0]==NOTREACHED || val<ch->lzones[i][0] ) {
			ch->lzones[i][0] = val;
			ch->lzones[i][1] = wi->zones[i][j];
		    }
		    if ( ch->rzones[i][0]==NOTREACHED || val>ch->rzones[i][0] ) {
			ch->rzones[i][0] = val;
			ch->rzones[i][1] = wi->zones[i][j];
		    }
		}
		tbase = t2;
	    }
	    t = SplineSolve(&spline->splines[1],tbase,1.0,wi->zones[i][j],.01);
	    if ( t>=0 && t<=1 ) {
		val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
		if ( ch->lzones[i][0]==NOTREACHED || val<ch->lzones[i][0] ) {
		    ch->lzones[i][0] = val;
		    ch->lzones[i][1] = wi->zones[i][j];
		}
		if ( ch->rzones[i][0]==NOTREACHED || val>ch->rzones[i][0] ) {
		    ch->rzones[i][0] = val;
		    ch->rzones[i][1] = wi->zones[i][j];
		}
	    }
	}
    }
}

static real SSFindMaxXAtY(SplineSet *spl,real y,real max) {
    Spline *sp, *first;

    while ( spl!=NULL ) {
	first = NULL;
	for ( sp = spl->first->next; sp!=NULL && sp!=first; sp = sp->to->next ) {
	    max = SplineFindMaxXAtY(sp,y,max);
	    if ( first==NULL ) first = sp;
	}
	spl = spl->next;
    }
return( max );
}

static real SSFindMinXAtY(SplineSet *spl,real y,real min) {
    Spline *sp, *first;

    while ( spl!=NULL ) {
	first = NULL;
	for ( sp = spl->first->next; sp!=NULL && sp!=first; sp = sp->to->next ) {
	    min = SplineFindMinXAtY(sp,y,min);
	    if ( first==NULL ) first = sp;
	}
	spl = spl->next;
    }
return( min );
}

static void SSFillZones(SplineSet *spl,struct charone *ch, WidthInfo *wi) {
    Spline *sp, *first;

    while ( spl!=NULL ) {
	first = NULL;
	for ( sp = spl->first->next; sp!=NULL && sp!=first; sp = sp->to->next ) {
	    SplineFillZones(sp,ch,wi);
	    if ( first==NULL ) first = sp;
	}
	spl = spl->next;
    }
}

static real SCFindMaxXAtY(SplineChar *sc,real y) {
    real max = NOTREACHED;
    RefChar *ref;

    max = SSFindMaxXAtY(sc->splines,y,NOTREACHED);
    for ( ref=sc->refs; ref!=NULL; ref=ref->next )
	max = SSFindMaxXAtY(ref->splines,y,max);
return( max );
}

static real SCFindMinXAtY(SplineChar *sc,real y) {
    real min = NOTREACHED;
    RefChar *ref;

    min = SSFindMinXAtY(sc->splines,y,NOTREACHED);
    for ( ref=sc->refs; ref!=NULL; ref=ref->next )
	min = SSFindMinXAtY(ref->splines,y,min);
return( min );
}

static void SCFillZones(struct charone *ch,WidthInfo *wi) {
    RefChar *ref;
    int i;

    for ( i=0; i<z_max; ++i )
	ch->lzones[i][0] = ch->lzones[i][1] =
		ch->rzones[i][0] = ch->rzones[i][1] = NOTREACHED;
    SSFillZones(ch->sc->splines,ch,wi);
    for ( ref=ch->sc->refs; ref!=NULL; ref=ref->next )
	SSFillZones(ref->splines,ch,wi);
    ch->lbearing = ch->rmax = NOTREACHED;
    for ( i=0; i<z_max; ++i ) {
	if ( ch->lzones[i][0]!=NOTREACHED )
	    if ( ch->lbearing==NOTREACHED || ch->lzones[i][0]<ch->lbearing )
		ch->lbearing = ch->lzones[i][0];
	if ( ch->rzones[i][0]!=NOTREACHED )
	    if ( ch->rmax==NOTREACHED || ch->rzones[i][0]>ch->rmax )
		ch->rmax = ch->rzones[i][0];
    }
}

static real weights[z_max] = { 1, 1, 1, 1, 1, .25, .25, .25, .25 };
static void PairFindDistance(struct charpair *cp,WidthInfo *wi) {
    int i;
    real xr, xl;
    real sum, cnt, max, min, fudge;

    for ( i=0; i<z_max; ++i ) {
	cp->zones[i] = NOTREACHED;
	if ( cp->left->rzones[i][0]!=NOTREACHED && cp->right->lzones[i][0]!=NOTREACHED ) {
	    if ( cp->left->rzones[i][1]==cp->right->lzones[i][1]) {
		cp->zones[i] = cp->right->lzones[i][0]-cp->right->lbearing+
				    cp->left->rmax-cp->left->rzones[i][0];
	    } else {
		xr = SCFindMinXAtY(cp->right->sc,cp->left->rzones[i][1]);
		xl = SCFindMaxXAtY(cp->left->sc,cp->right->lzones[i][1]);
		if ( xr!=NOTREACHED && xl!=NOTREACHED ) {
		    xr = xr-cp->right->lbearing +
			    cp->left->rmax-cp->left->rzones[i][0];
		    xl = cp->right->lzones[i][0]-cp->right->lbearing+
			    cp->left->rmax-xl;
		    cp->zones[i] = xr<xl? xr : xl ;
		    /* I used to average them. But that gave bad results */
		    /* "TB" and "TK" were thought to be different in helvetica*/
		    /* just because they picked different heights to compare */
		    /* at */
		} else if ( xr!=NOTREACHED ) {
		    cp->zones[i] = xr-cp->right->lbearing +
			    cp->left->rmax-cp->left->rzones[i][0];
		} else if ( xl!=NOTREACHED ) {
		    cp->zones[i] = cp->right->lzones[i][0]-cp->right->lbearing+
			    cp->left->rmax-xl;
		}
	    }
	    if ( cp->zones[i]!=NOTREACHED && cp->zones[i]<0 )
		GDrawIError("AutoWidth zoning is screwed up" );
	}
    }

    max = min = NOTREACHED;
    /* Emphasize the min separation, demphasize the max sep. */
    for ( i=0; i<z_serifbase; ++i ) {
	if ( cp->zones[i]!=NOTREACHED ) {
	    if ( max==NOTREACHED || max<cp->zones[i] )
		max = cp->zones[i];
	    if ( min==NOTREACHED || min>cp->zones[i] )
		min = cp->zones[i];
	}
    }
    fudge = (wi->sf->ascent+wi->sf->descent)/100;
    if ( min==NOTREACHED )
	cp->average=0;
    else if ( min>=max-fudge )
	cp->average = min;
    else {
	sum = cnt = 0;
	for ( i=0; i<z_max; ++i ) {
	    if ( cp->zones[i]!=NOTREACHED && cp->zones[i]<=max-fudge ) {
		cnt += weights[i];
		sum += cp->zones[i]*weights[i];
	    }
	}
	cp->average = (2*min+sum/cnt)/3;
    }
}

static real LineFindLeftDistance(struct charone *right,WidthInfo *wi) {
    int i;
    real sum, cnt, max, min, fudge;
    real zones[z_max];

    for ( i=0; i<z_max; ++i ) {
	zones[i] = NOTREACHED;
	if ( right->lzones[i][0]!=NOTREACHED )
	    zones[i] = right->lzones[i][0]-right->lbearing;
    }

    max = min = NOTREACHED;
    /* Emphasize the min separation, demphasize the max sep. */
    for ( i=0; i<z_serifbase; ++i ) {
	if ( zones[i]!=NOTREACHED ) {
	    if ( max==NOTREACHED || max<zones[i] )
		max = zones[i];
	    if ( min==NOTREACHED || min>zones[i] )
		min = zones[i];
	}
    }
    fudge = (wi->sf->ascent+wi->sf->descent)/100;
    if ( min==NOTREACHED )
return( 0 );
    else if ( min>=max-fudge )
return( min );
    else {
	sum = cnt = 0;
	for ( i=0; i<z_max; ++i ) {
	    if ( zones[i]!=NOTREACHED && zones[i]<=max-fudge ) {
		cnt += weights[i];
		sum += zones[i]*weights[i];
	    }
	}
return( (2*min+sum/cnt)/3 );
    }
}

static real LineFindRightDistance(struct charone *left,WidthInfo *wi) {
    int i;
    real sum, cnt, max, min, fudge;
    real zones[z_max];

    for ( i=0; i<z_max; ++i ) {
	zones[i] = NOTREACHED;
	if ( left->rzones[i][0]!=NOTREACHED )
	    zones[i] = left->rmax-left->rzones[i][0];
    }

    max = min = NOTREACHED;
    /* Emphasize the min separation, demphasize the max sep. */
    for ( i=0; i<z_serifbase; ++i ) {
	if ( zones[i]!=NOTREACHED ) {
	    if ( max==NOTREACHED || max<zones[i] )
		max = zones[i];
	    if ( min==NOTREACHED || min>zones[i] )
		min = zones[i];
	}
    }
    fudge = (wi->sf->ascent+wi->sf->descent)/100;
    if ( min==NOTREACHED )
return( 0 );
    else if ( min>=max-fudge )
return( min );
    else {
	sum = cnt = 0;
	for ( i=0; i<z_max; ++i ) {
	    if ( zones[i]!=NOTREACHED && zones[i]<=max-fudge ) {
		cnt += weights[i];
		sum += zones[i]*weights[i];
	    }
	}
return( (2*min+sum/cnt)/3 );
    }
}

static void FindZones(WidthInfo *wi) {
    DBounds bb;
    SplineFont *sf=wi->sf;
    int i, si=-1;
    real as, ds, xh, serifsize, angle, ca;
    static char *caps = "ABCDEFGHIJKLMNOPQRSTUVWXYZbdhkl";
    static char *descent = "pqgyj";
    static char *xheight = "xuvwyzacegmnopqrs";
    static char *easyserif = "IBDEFHIKLNPR";
    real stemx, testx, y, ytop, ybottom, yorig, topx, bottomx;

    for ( i=0; caps[i]!='\0'; ++i )
	if ( (si=SFFindExistingChar(sf,caps[i],NULL))!=-1 && sf->chars[si]!=NULL )
    break;
    if ( caps[i]!='\0' ) {
	SplineCharFindBounds(sf->chars[si],&bb);
	as = bb.maxy;
    } else
	as = sf->ascent;

    for ( i=0; descent[i]!='\0'; ++i )
	if ( (si=SFFindExistingChar(sf,descent[i],NULL))!=-1 && sf->chars[si]!=NULL )
    break;
    if ( descent[i]!='\0' ) {
	SplineCharFindBounds(sf->chars[si],&bb);
	ds = bb.miny;
    } else
	ds = -sf->descent;

    for ( i=0; xheight[i]!='\0'; ++i )
	if ( (si=SFFindExistingChar(sf,xheight[i],NULL))!=-1 && sf->chars[si]!=NULL )
    break;
    if ( xheight[i]!='\0' ) {
	SplineCharFindBounds(sf->chars[si],&bb);
	xh = bb.maxy;
    } else
	xh = 3*as/4;

    for ( i=0; easyserif[i]!='\0'; ++i )
	if ( (si=SFFindExistingChar(sf,easyserif[i],NULL))!=-1 && sf->chars[si]!=NULL )
    break;
    if ( easyserif[i]!='\0' ) {
	topx = SCFindMinXAtY(sf->chars[si],2*as/3);
	bottomx = SCFindMinXAtY(sf->chars[si],as/3);
	/* beware of slanted (italic, oblique) fonts */
	ytop = as/2; ybottom=0;
	stemx = SCFindMinXAtY(sf->chars[si],ytop);
	if ( topx==bottomx ) {
	    while ( ytop-ybottom>=.5 ) {
		y = (ytop+ybottom)/2;
		testx = SCFindMinXAtY(sf->chars[si],y);
		if ( testx+1>=stemx )
		    ytop = y;
		else
		    ybottom = y;
	    }
	} else {
	    angle = atan2(as/3,topx-bottomx);
	    ca = cos(angle);
	    yorig = ytop;
	    while ( ytop-ybottom>=.5 ) {
		y = (ytop+ybottom)/2;
		testx = SCFindMinXAtY(sf->chars[si],y)+
		    (yorig-y)*ca;
		if ( testx+4>=stemx )		/* the +4 is to counteract rounding */
		    ytop = y;
		else
		    ybottom = y;
	    }
	}
	if ( ytop>as/4 )
	    serifsize = .08*(sf->ascent+sf->descent);
	else
	    serifsize = ytop;
    } else
	serifsize = .08*(sf->ascent+sf->descent);
    serifsize = rint(serifsize);

    if ( serifsize==0 ) {
	for ( i=z_serifbase; i<z_max; ++i )
	    wi->zones[i][0] = wi->zones[i][1] = sf->ascent+sf->descent;
    } else {
	wi->zones[z_serifbase][0] = 0; wi->zones[z_serifbase][1] = serifsize;
	wi->zones[z_serifxh][0] = xh-serifsize; wi->zones[z_serifxh][1] = xh;
	wi->zones[z_serifas][0] = as-serifsize; wi->zones[z_serifas][1] = as;
	wi->zones[z_serifds][0] = ds; wi->zones[z_serifds][1] = ds+serifsize;
    }

    wi->zones[z_descend][0] = ds+serifsize; wi->zones[z_descend][1] = -serifsize/4;
    wi->zones[z_ascend][0] = xh+serifsize/4; wi->zones[z_ascend][1] = as-serifsize;
    wi->zones[z_low][0] = serifsize; wi->zones[z_low][1] = serifsize+(xh-2*serifsize)/3;
    wi->zones[z_mid][0] = serifsize+(xh-2*serifsize)/3; wi->zones[z_mid][1] = serifsize+2*(xh-2*serifsize)/3;
    wi->zones[z_high][0] = serifsize+2*(xh-2*serifsize)/3; wi->zones[z_high][1] = xh-serifsize;

    if ( as!=sf->ascent && ds!=-sf->descent )
	wi->space_guess = rint(.205*(as-ds));
    else
	wi->space_guess = rint(.184*(sf->ascent+sf->descent));
}

real SFGuessItalicAngle(SplineFont *sf) {
    static char *easyserif = "IBDEFHKLNPR";
    int i,si;
    real as, topx, bottomx;
    DBounds bb;

    for ( i=0; easyserif[i]!='\0'; ++i )
	if ( (si=SFFindExistingChar(sf,easyserif[i],NULL))!=-1 && sf->chars[si]!=NULL )
    break;
    if ( easyserif[i]=='\0' )		/* can't guess */
return( 0 );

    SplineCharFindBounds(sf->chars[si],&bb);
    as = bb.maxy-bb.miny;

    topx = SCFindMinXAtY(sf->chars[si],2*as/3+bb.miny);
    bottomx = SCFindMinXAtY(sf->chars[si],as/3+bb.miny);
    if ( topx==bottomx )
return( 0 );

return( atan2(as/3,topx-bottomx)*180/3.1415926535897932-90 );
}

void SFHasSerifs(SplineFont *sf) {
    static char *easyserif = "IBDEFHKLNPR";
    int i,si;
    real as, topx, bottomx, serifbottomx, seriftopx;
    DBounds bb;

    for ( i=0; easyserif[i]!='\0'; ++i )
	if ( (si=SFFindExistingChar(sf,easyserif[i],NULL))!=-1 && sf->chars[si]!=NULL )
    break;
    if ( easyserif[i]=='\0' )		/* can't guess */
return;
    sf->serifcheck = true;

    SplineCharFindBounds(sf->chars[si],&bb);
    as = bb.maxy-bb.miny;

    topx = SCFindMinXAtY(sf->chars[si],2*as/3+bb.miny);
    bottomx = SCFindMinXAtY(sf->chars[si],as/3+bb.miny);
    serifbottomx = SCFindMinXAtY(sf->chars[si],1+bb.miny);
    seriftopx = SCFindMinXAtY(sf->chars[si],bb.maxy-1);
    if ( RealNear(topx,bottomx) ) {
	if ( RealNear(serifbottomx,bottomx) && RealNear(seriftopx,topx))
	    sf->issans = true;
	else if ( RealNear(serifbottomx,seriftopx) && topx-seriftopx>0 )
	    sf->isserif = true;
    } else {
	/* It's Italic. I'm just going to give up.... */
    }
}

static void BuildCharPairs(WidthInfo *wi) {
    int i, j;
    struct charpair *cp;

    wi->pairs = galloc(wi->lcnt*wi->rcnt*sizeof(struct charpair *));
    for ( i=0; i<wi->lcnt; ++i ) for ( j=0; j<wi->rcnt; ++j ) {
	wi->pairs[i*wi->rcnt+j] = cp = gcalloc(1,sizeof(struct charpair));
	cp->left = wi->left[i];
	cp->right = wi->right[j];
	cp->nextasleft = cp->left->asleft;
	cp->left->asleft = cp;
	cp->nextasright = cp->right->asright;
	cp->right->asright = cp;
    }
    wi->tcnt = wi->lcnt+wi->rcnt;

    /* FindZones(wi); */		/* Moved earlier */

    for ( i=0; i<wi->lcnt; ++i )
	SCFillZones(wi->left[i],wi);
    for ( i=0; i<wi->rcnt; ++i )
	SCFillZones(wi->right[i],wi);

    for ( i=0; i<wi->lcnt*wi->rcnt; ++i )
	PairFindDistance(wi->pairs[i],wi);
}

static void FreeCharList(struct charone **list) {
    int i;

    for ( i=0; list[i]!=NULL; ++i )
	free( list[i] );
    free(list);
}

static void FreeCharPairs(struct charpair **list, int cnt) {
    int i;

    for ( i=0; i<cnt; ++i )
	free( list[i] );
    free(list);
}

int KernThreshold(SplineFont *sf, int cnt) {
    /* We want only cnt kerning pairs in the entire font. Any pair whose */
    /*  absolute offset is less than the threshold should be removed */
    int *totals, tot;
    int high, i, val;
    KernPair *kp;

    if ( cnt==0 )		/* Infinite */
return(0);

    high = sf->ascent + sf->descent;
    totals = gcalloc(high+1,sizeof(int));
    tot=0;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	for ( kp = sf->chars[i]->kerns; kp!=NULL; kp = kp->next ) {
	    val = kp->off;
	    if ( val!=0 ) {
		if ( val<0 ) val = -val;
		if ( val>high ) val = high;
		++totals[val];
		++tot;
	    }
	}
    }
    if ( tot>cnt ) {
	tot = 0;
	for ( i=high; i>0 && tot+totals[i]<cnt; --i )
	    tot += totals[i];
	free(totals);
return( i+1 );
    }
    free(totals);
return( 0 );
}

static void KernRemoveBelowThreshold(SplineFont *sf,int threshold) {
    int i;
    KernPair *kp, *prev, *next;
    MetricsView *mv;

    if ( threshold==0 )
return;

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	prev = NULL;
	for ( kp = sf->chars[i]->kerns; kp!=NULL; kp = next ) {
	    next = kp->next;
	    if ( kp->off>=threshold || kp->off<=-threshold )
		prev = kp;
	    else {
		if ( prev==NULL )
		    sf->chars[i]->kerns = next;
		else
		    prev->next = next;
		free(kp);
	    }
	}
    }
    for ( mv=sf->fv->metrics; mv!=NULL; mv=mv->next )
	MVReKern(mv);
}

#define CID_Spacing	1001
#define CID_Total	1002
#define CID_Threshold	1003
#define CID_LeftBase	1010
#define CID_RightBase	1020
#define C_All		1
#define C_Sel		2
#define C_Typed		3
#define C_Std		4
#define C_Text		4

static struct charone *MakeCharOne(SplineChar *sc) {
    struct charone *ch = gcalloc(1,sizeof(struct charone));

    ch->sc = sc;
return( ch );
}

static struct charone **BuildCharList(SplineFont *sf,GWindow gw, int base, int *tot) {
    int i, cnt, doit, s, e;
    struct charone **ret=NULL;
    int all, sel, parse=false;
    const unichar_t *str, *pt;

    all = GGadgetIsChecked(GWidgetGetControl(gw,base+C_All));
    sel = GGadgetIsChecked(GWidgetGetControl(gw,base+C_Sel));
    if ( GGadgetIsChecked(GWidgetGetControl(gw,base+C_Typed))) {
	parse = true;
	str = _GGadgetGetTitle(GWidgetGetControl(gw,base+C_Text));
    } else {
	parse = true;
	str = GStringGetResource(_STR_StdCharRange,NULL);
    }

    for ( doit=0; doit<2; ++doit ) {
	if ( all ) {
	    for ( i=cnt=0; i<sf->charcnt && cnt<300; ++i ) {
		if ( sf->chars[i]!=NULL &&
			(sf->chars[i]->splines!=NULL || sf->chars[i]->refs!=NULL )) {
		    if ( doit )
			ret[cnt++] = MakeCharOne(sf->chars[i]);
		    else
			++cnt;
		}
	    }
	} else if ( sel ) {
	    for ( i=cnt=0; i<sf->charcnt && cnt<300; ++i ) {
		if ( sf->fv->selected[i] && sf->chars[i]!=NULL &&
			(sf->chars[i]->splines!=NULL || sf->chars[i]->refs!=NULL )) {
		    if ( doit )
			ret[cnt++] = MakeCharOne(sf->chars[i]);
		    else
			++cnt;
		}
	    }
	} else {
	    for ( pt=str, cnt=0; *pt && cnt<300 ; ) {
		if ( pt[1]=='-' && pt[2]!='\0' ) {
		    s = pt[0]; e = pt[2];
		    pt += 3;
		} else {
		    s = e = pt[0];
		    ++pt;
		}
		for ( ; s<=e && cnt<300 ; ++s ) {
		    i = SFFindExistingChar(sf,s,NULL);
		    if ( i!=-1 && sf->chars[i]!=NULL &&
			    (sf->chars[i]->splines!=NULL || sf->chars[i]->refs!=NULL )) {
			if ( doit )
			    ret[cnt++] = MakeCharOne(sf->chars[i]);
			else
			    ++cnt;
		    }
		}
	    }
	}
	if ( !doit )
	    ret = galloc((cnt+1)*sizeof(struct charone *));
	else
	    ret[cnt] = NULL;
    }
    *tot = cnt;
return( ret );
}

static int AW_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	WidthInfo *wi = GDrawGetUserData(gw);
	int err = false;
	int tot;

	wi->spacing = GetRealR(gw,CID_Spacing, _STR_Spacing,&err);
	if ( wi->autokern ) {
	    wi->threshold = GetIntR(gw,CID_Threshold, _STR_Threshold, &err);
	    tot = GetIntR(gw,CID_Total, _STR_TotalKerns, &err);
	    if ( tot<0 ) tot = 0;
	}
	if ( err )
return( true );
	wi->left = BuildCharList(wi->sf,gw,CID_LeftBase, &wi->lcnt );
	wi->right = BuildCharList(wi->sf,gw,CID_RightBase, &wi->rcnt );
	if ( wi->lcnt==0 || wi->rcnt==0 ) {
	    FreeCharList(wi->left);
	    FreeCharList(wi->right);
	    GWidgetErrorR(_STR_NoCharsSelected,_STR_NoCharsSelected);
return( true );
	}
	wi->done = true;
	GDrawSetVisible(gw,false);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
	BuildCharPairs(wi);
	if ( wi->autokern ) {
	    AutoKern(wi);
	    KernRemoveBelowThreshold(wi->sf,KernThreshold(wi->sf,tot));
	} else
	    AutoWidth(wi);
	FreeCharList(wi->left);
	FreeCharList(wi->right);
	FreeCharPairs(wi->pairs,wi->lcnt*wi->rcnt);
    }
return( true );
}

static int AW_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	WidthInfo *wi = GDrawGetUserData(gw);
	wi->done = true;
    }
return( true );
}

static int AW_FocusChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged ) {
	GWindow gw = GGadgetGetWindow(g);
	int cid = (int) GGadgetGetUserData(g);
	GGadgetSetChecked(GWidgetGetControl(gw,cid),true);
    }
return( true );
}

static int AW_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	WidthInfo *wi = GDrawGetUserData(gw);
	wi->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

#define SelHeight	51
static int MakeSelGadgets(GGadgetCreateData *gcd, GTextInfo *label, int i, int base,
	int labr, int y, int pixel_width, GWindow gw, int toomany ) {

    label[i].text = (unichar_t *) labr;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 12; gcd[i].gd.pos.y = y; 
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = 6; gcd[i].gd.pos.y = GDrawPointsToPixels(gw,y+6);
    gcd[i].gd.pos.width = pixel_width-12; gcd[i].gd.pos.height = GDrawPointsToPixels(gw,SelHeight);
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[i++].creator = GGroupCreate;

    label[i].text = (unichar_t *) _STR_All;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 12; gcd[i].gd.pos.y = y+11; 
    gcd[i].gd.flags = (toomany&1) ? gg_visible : (gg_visible | gg_enabled);
    gcd[i].gd.cid = base+C_All;
    gcd[i++].creator = GRadioCreate;

    label[i].text = (unichar_t *) _STR_StdCharRange;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 75; gcd[i].gd.pos.y = y+11; 
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    gcd[i].gd.cid = base+C_Std;
    gcd[i++].creator = GRadioCreate;

    label[i].text = (unichar_t *) _STR_Selected;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 12; gcd[i].gd.pos.y = y+32; 
    gcd[i].gd.flags = (toomany&2) ? gg_visible : (gg_visible | gg_enabled);
    gcd[i].gd.cid = base+C_Sel;
    gcd[i++].creator = GRadioCreate;

    gcd[i].gd.pos.x = 75; gcd[i].gd.pos.y = y+32; 
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.cid = base+C_Typed;
    gcd[i++].creator = GRadioCreate;

    label[i].text = (unichar_t *) _STR_StdCharRange;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 98; gcd[i].gd.pos.y = y+31; 
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.cid = base+C_Text;
    gcd[i].gd.handle_controlevent = AW_FocusChange;
    gcd[i].data = (void *) (base+C_Typed);
    gcd[i++].creator = GTextFieldCreate;
return( i );
}

static int SFCount(SplineFont *sf) {
    int i, cnt;

    for ( i=cnt=0; i<sf->charcnt; ++i )
	if ( sf->chars[i]!=NULL &&
		(sf->chars[i]->splines!=NULL || sf->chars[i]->refs!=NULL ))
	    ++cnt;
return( cnt );
}

static int SFCountSel(SplineFont *sf) {
    int i, cnt;
    char *sel = sf->fv->selected;

    for ( i=cnt=0; i<sf->charcnt; ++i )
	if ( sel[i] && sf->chars[i]!=NULL &&
		(sf->chars[i]->splines!=NULL || sf->chars[i]->refs!=NULL ))
	    ++cnt;
return( cnt );
}

static void AutoWKDlg(SplineFont *sf,int autokern) {
    WidthInfo wi;
    GWindow gw;
    GWindowAttrs wattrs;
    GRect pos;
    GGadgetCreateData gcd[28];
    GTextInfo label[28];
    int i, y;
    char buffer[30], buffer2[30];
    int selcnt = SFCountSel(sf);
    int toomany = ((SFCount(sf)>=300)?1:0) | ((selcnt==0 || selcnt>=300)?2:0);

    memset(&wi,'\0',sizeof(wi));
    wi.autokern = autokern;
    wi.sf = sf;
    FindZones(&wi);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(autokern?_STR_Autokern:_STR_Autowidth,NULL);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,200);
    pos.height = GDrawPointsToPixels(NULL,autokern?273:215);
    gw = GDrawCreateTopWindow(NULL,&pos,AW_e_h,&wi,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    i = 0;

    label[i].text = (unichar_t *) _STR_EnterTwoCharRange;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i++].creator = GLabelCreate;

    label[i].text = (unichar_t *) _STR_ToBeAdjusted;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 18;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i++].creator = GLabelCreate;

    i = MakeSelGadgets(gcd, label, i, CID_LeftBase, _STR_CharsLeft, 33,
	    pos.width, gw, toomany);
    i = MakeSelGadgets(gcd, label, i, CID_RightBase, _STR_CharsRight, 33+SelHeight+9,
	    pos.width, gw, toomany);
    y = 32+2*(SelHeight+9);

    label[i].text = (unichar_t *) _STR_Spacing;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = y+7;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i++].creator = GLabelCreate;

    sprintf( buffer, "%d", wi.space_guess );
    label[i].text = (unichar_t *) buffer;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 65; gcd[i].gd.pos.y = y+3;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.cid = CID_Spacing;
    gcd[i++].creator = GTextFieldCreate;
    y += 32;

    if ( autokern ) {
	y -= 4;

	label[i].text = (unichar_t *) _STR_TotalKerns;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = y+7;
	gcd[i].gd.flags = gg_visible | gg_enabled;
	gcd[i++].creator = GLabelCreate;

	label[i].text = (unichar_t *) "2048";
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 65; gcd[i].gd.pos.y = y+3;
	gcd[i].gd.flags = gg_visible | gg_enabled;
	gcd[i].gd.cid = CID_Total;
	gcd[i++].creator = GTextFieldCreate;
	y += 28;

	label[i].text = (unichar_t *) _STR_Threshold;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = y+7;
	gcd[i].gd.flags = gg_visible | gg_enabled;
	gcd[i++].creator = GLabelCreate;

	sprintf( buffer2, "%d", (sf->ascent+sf->descent)/25 );
	label[i].text = (unichar_t *) buffer2;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 65; gcd[i].gd.pos.y = y+3;
	gcd[i].gd.flags = gg_visible | gg_enabled;
	gcd[i].gd.cid = CID_Threshold;
	gcd[i++].creator = GTextFieldCreate;
	y += 32;
    }

    gcd[i].gd.pos.x = 30-3; gcd[i].gd.pos.y = y-3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _STR_OK;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'O';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = AW_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 200-GIntGetResource(_NUM_Buttonsize)-30; gcd[i].gd.pos.y = y;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _STR_Cancel;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.handle_controlevent = AW_Cancel;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = GDrawPointsToPixels(gw,2);
    gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[i++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !wi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

void FVAutoKern(FontView *fv) {
    AutoWKDlg(fv->sf,true);
}

void FVAutoWidth(FontView *fv) {
    AutoWKDlg(fv->sf,false);
}

void FVRemoveKerns(FontView *fv) {
    int i;
    SplineChar *sc;
    int changed = false;
    MetricsView *mv;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( (sc = fv->sf->chars[i])!=NULL ) {
	if ( sc->kerns!=NULL ) {
	    changed = true;
	    KernPairsFree(sc->kerns);
	    sc->kerns = NULL;
	}
    }
    if ( changed ) {
	fv->sf->changed = true;
	for ( mv=fv->metrics; mv!=NULL; mv=mv->next )
	    MVReKern(mv);
    }
}
