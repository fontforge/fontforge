/* -*- coding: utf-8 -*- */
/* Copyright (C) 2000-2012 by George Williams */
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
#include "fontforgevw.h"
#include "cvundoes.h"
#include <math.h>
#include <ustring.h>
#include <utype.h>

#include "autowidth.h"

SplineFont *aw_old_sf=NULL;
int aw_old_spaceguess;

#define THIRDS_IN_WIDTH 0

/* The basic idea behind autowidth:
    We figure out how high serifs are (and if they exist at all)
    And we guess at the cap height, xheight and descender and we mark off
	zones where the serifs go (caph,caph-serifsize) (xh,xh-serifsize)
	(0,serifsize), (ds,ds+serifsize). In the future we will ignore
	those zones.
    We look at each character and we get a rough idea of the right and
	left edge of that character. (we create an array with about 60
	entries in it for a capital letter, and we divide the edge into
	60 regions and figure out roughly what the leftmost(rightmost)
	point of the character is in each region).
    The for every character pair we try to figure out what the visual
	spacing between the two letters is (we subtract off the lbearing,
	rbearing so that our result is the visual spacing if the two
	characters were jammed up against each other).
	Here we look at all regions the characters have in common and
	just find the seperation between.
	Then given this array of distances we use a heuristic to define what
	the visual spacing is.
    Then we find the "average visual distance" between two characters
	(deducting the right and left bearings so it is in some sense absolute)
	And foreach character on the left side we find the average visual
	distance of all characters paired with it and deduct this from the
	more general average found above. This gives us the amount by which
	this character's rbearing should differ from the general rbearing.
	The general rbearing should be 2*(spacing-average)/3, so the specific
	is 2*(spacing-average)/3 + average-local_average.
	Do the same thing to find lbearings for characters on the right side.
===================================
Autokern has similar ideas, but is simpler:
    Once we've found the average of the zone distances
    Adjust the kerning between the two characters so that
	<kern> = <dspace>-(<rb>-<lb>-<visual>)
	Again check for overlap (<dspace>/4 in any zone)
    Kerning by the left character can always propigate to dependents
    Kerning by the right char should not
	(ie. A and À kern the same with V
	 but V kerns differently with e and é-- the accent gets in the way)
    No, I think it is better not to propigate kerning.
*/


static void FigureLR(WidthInfo *wi) {
    int i;
    real sum, subsum, subsum_left_I, subsum_right_I, spacing;
    struct charone *ch;
    struct charpair *cp;

    sum = 0;
    for ( i=0; i<wi->pcnt; ++i )
	sum += wi->pairs[i]->visual;
    sum /= (wi->pcnt);

    subsum_left_I = subsum_right_I = sum;
    if ( wi->l_Ipos!=-1 ) {
	subsum_left_I = 0;
	for ( cp = wi->left[wi->l_Ipos]->asleft; cp!=NULL; cp = cp->nextasleft )
	    subsum_left_I += cp->visual;
	subsum_left_I /= wi->rcnt;
    }
    if ( wi->r_Ipos!=-1 ) {
	subsum_right_I = 0;
	for ( cp = wi->right[wi->r_Ipos]->asright; cp!=NULL; cp = cp->nextasright )
	    subsum_right_I += cp->visual;
	subsum_right_I /= wi->lcnt;
    }

    /* Normalize so that spacing between two "I"s is correct... */
    spacing = wi->spacing - (2*sum-subsum_left_I-subsum_right_I);

    for ( i=0; i<wi->real_lcnt; ++i ) {
	ch = wi->left[i];
	subsum = 0;
	for ( cp = ch->asleft; cp!=NULL; cp = cp->nextasleft )
	    subsum += cp->visual;
	subsum /= wi->rcnt;
#if THIRDS_IN_WIDTH
	ch->newr = rint( spacing*2/3.0 + sum-subsum );
#else
	ch->newr = rint( spacing/2.0 + sum-subsum );
#endif
    }
    for ( i=0; i<wi->real_rcnt; ++i ) {
	ch = wi->right[i];
	subsum = 0;
	for ( cp = ch->asright; cp!=NULL; cp = cp->nextasright )
	    subsum += cp->visual;
	subsum /= wi->lcnt;
#if THIRDS_IN_WIDTH
	ch->newl = rint( spacing/3.0+ sum-subsum );
#else
	ch->newl = rint( spacing/2.0+ sum-subsum );
#endif
    }
}

static void CheckOutOfBounds(WidthInfo *wi) {
    int i,j;
    struct charpair *cp;
    real min=NOTREACHED, temp, lr;
    real minsp = wi->spacing/3;

    for ( i=0; i<wi->real_rcnt; ++i ) {
	if ( wi->right[i]->newl<-wi->spacing || wi->right[i]->newl>wi->spacing )
	    LogError( _("AutoWidth failure on %s\n"), wi->right[i]->sc->name );
	if ( wi->right[i]->newl<-minsp )
	    wi->right[i]->newl = -rint(minsp);
    }
    for ( i=0; i<wi->real_lcnt; ++i ) {
	if ( wi->left[i]->newr<-wi->spacing-wi->seriflength ||
		wi->left[i]->newr>wi->spacing+wi->seriflength ) {
	    LogError( _("AutoWidth failure on %s\n"), wi->right[i]->sc->name );
	    if ( wi->left[i]->newr>wi->spacing )
		wi->left[i]->newr = wi->spacing;
	}
    }
    for ( i=0; i<wi->pcnt; ++i ) {
	cp = wi->pairs[i];
	if ( cp->left->newr==NOTREACHED || cp->right->newl==NOTREACHED )
    continue;
	lr = cp->left->newr + cp->right->newl;
	min = NOTREACHED;
	for ( j=0; j<=cp->top-cp->base; ++j ) if ( cp->distances[j]!=NOTREACHED ) {
	    temp = lr + cp->distances[j];
	    if ( min==NOTREACHED || temp<min )
		min = temp;
	}
	if ( min!=NOTREACHED && min<minsp )
	    cp->left->newr += rint(minsp-min);
    }
}

static void ApplyChanges(WidthInfo *wi) {
    EncMap *map = wi->fv->map;
    uint8 *rsel = calloc(map->enccount,sizeof(char));
    int i, width;
    real transform[6];
    struct charone *ch;
    DBounds bb;

    for ( i=0; i<wi->real_rcnt; ++i ) {
	int gid = map->map[wi->right[i]->sc->orig_pos];
	if ( gid!=-1 )
	    rsel[gid] = true;
    }
    transform[0] = transform[3] = 1.0;
    transform[1] = transform[2] = transform[5] = 0;

    for ( i=0; i<wi->real_rcnt; ++i ) {
	ch = wi->right[i];
	transform[4] = ch->newl-ch->lbearing;
	if ( transform[4]!=0 ) {
	    FVTrans(wi->fv,ch->sc,transform,rsel,false);
	    SCCharChangedUpdate(ch->sc,ly_none);
	}
    }
    free(rsel);

    for ( i=0; i<wi->real_lcnt; ++i ) {
	ch = wi->left[i];
	SplineCharLayerFindBounds(ch->sc,wi->layer,&bb);
	width = rint(bb.maxx + ch->newr);
	if ( width!=ch->sc->width ) {
	    SCPreserveWidth(ch->sc);
	    SCSynchronizeWidth(ch->sc,width,ch->sc->width,wi->fv);
	    SCCharChangedUpdate(ch->sc,ly_none);
	}
    }
}

void AW_AutoWidth(WidthInfo *wi) {
    FigureLR(wi);
    CheckOutOfBounds(wi);
    ApplyChanges(wi);
}

void AW_AutoKern(WidthInfo *wi) {
    struct charpair *cp;
    SplineChar *lsc, *rsc;
    int i, diff;
    KernPair *kp;

    for ( i=0; i<wi->pcnt; ++i ) {
	cp = wi->pairs[i];

	diff = rint( wi->spacing-(cp->left->sc->width-cp->left->rmax+cp->right->lbearing+cp->visual));

	if ( wi->threshold!=0 && diff>-wi->threshold && diff<wi->threshold )
	    diff = 0;
	if ( wi->onlynegkerns && diff>0 )
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
	    kp = chunkalloc(sizeof(KernPair));
	    kp->sc = rsc;
	    kp->off = diff;
	    kp->subtable = wi->subtable;
	    kp->next = lsc->kerns;
	    lsc->kerns = kp;
	    wi->sf->changed = true;
	}
    }
    MVReKernAll(wi->fv->sf);
}

static real SplineFindMinXAtY(Spline *spline,real y,real min) {
    extended t,t1,t2,tbase,val;
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
    SplineFindExtrema(&spline->splines[1], &t1, &t2 );
    tbase = 0;
    if ( t1!=-1 ) {
	t = SplineSolve(&spline->splines[1],0,t1,y);
	if ( t>=0 && t<=1 ) {
	    val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
	    if ( min==NOTREACHED || val<min )
		min = val;
	}
	tbase = t1;
    }
    if ( t2!=-1 ) {
	t = SplineSolve(&spline->splines[1],tbase,t2,y);
	if ( t>=0 && t<=1 ) {
	    val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
	    if ( min==NOTREACHED || val<min )
		min = val;
	}
	tbase = t2;
    }
    t = SplineSolve(&spline->splines[1],tbase,1.0,y);
    if ( t>=0 && t<=1 ) {
	val = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
	if ( min==NOTREACHED || val<min )
	    min = val;
    }
return( min );
}

static void PtFindEdges(real x, real y,struct charone *ch, WidthInfo *wi) {
    int i;

    i = rint(y/wi->decimation);
    if ( i>ch->top ) i=ch->top;		/* Can't happen */
    i -= ch->base;
    if ( i<0 ) i=0;			/* Can't happen */

    if ( ch->ledge[i]==NOTREACHED || x<ch->ledge[i] )
	ch->ledge[i] = x;
    if ( ch->redge[i]==NOTREACHED || x>ch->redge[i] )
	ch->redge[i] = x;
}

static void SplineFindEdges(Spline *spline,struct charone *ch, WidthInfo *wi) {
    Spline1D *xsp, *ysp;
    extended t1, t2;
    double t, toff, ymin, ymax;

    /* first try the end points */
    PtFindEdges(spline->to->me.x,spline->to->me.y,ch,wi);
    PtFindEdges(spline->from->me.x,spline->from->me.y,ch,wi);

    /* then try the extrema of the spline (assuming they are between t=(0,1) */
    xsp = &spline->splines[0]; ysp = &spline->splines[1];
    SplineFindExtrema(xsp, &t1, &t2 );
    if ( t1!=-1 )
	PtFindEdges( ((xsp->a*t1+xsp->b)*t1+xsp->c)*t1+xsp->d,
		((ysp->a*t1+ysp->b)*t1+ysp->c)*t1+ysp->d,
		ch,wi);
    if ( t2!=-1 )
	PtFindEdges( ((xsp->a*t2+xsp->b)*t2+xsp->c)*t2+xsp->d,
		((ysp->a*t2+ysp->b)*t2+ysp->c)*t2+ysp->d,
		ch,wi);

    ymin = ymax = spline->from->me.y;
    if ( spline->from->nextcp.y > ymax ) ymax = spline->from->nextcp.y;
    if ( spline->from->nextcp.y < ymin ) ymin = spline->from->nextcp.y;
    if ( spline->to->prevcp.y > ymax ) ymax = spline->to->prevcp.y;
    if ( spline->to->prevcp.y < ymin ) ymin = spline->to->prevcp.y;
    if ( spline->to->me.y > ymax ) ymax = spline->to->me.y;
    if ( spline->to->me.y < ymin ) ymin = spline->to->me.y;

    if ( ymin!=ymax ) {
	toff = wi->decimation/(2*(ymax-ymin));
	for ( t=toff; t<1; t+=toff ) {
	    PtFindEdges( ((xsp->a*t+xsp->b)*t+xsp->c)*t+xsp->d,
		    ((ysp->a*t+ysp->b)*t+ysp->c)*t+ysp->d,
		    ch,wi);
	}
    }
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

static real SSIsMinXAtYCurved(SplineSet *spl,real y,real oldmin,int *curved) {
    Spline *sp, *first;
    real min;

    while ( spl!=NULL ) {
	first = NULL;
	for ( sp = spl->first->next; sp!=NULL && sp!=first; sp = sp->to->next ) {
	    min = SplineFindMinXAtY(sp,y,oldmin);
	    if ( min!=oldmin ) {
		oldmin = min;
		*curved = !sp->knownlinear;
	    }
	    if ( first==NULL ) first = sp;
	}
	spl = spl->next;
    }
return( oldmin );
}

static void SSFindEdges(SplineSet *spl,struct charone *ch, WidthInfo *wi) {
    Spline *sp, *first;

    while ( spl!=NULL ) {
	first = NULL;
	for ( sp = spl->first->next; sp!=NULL && sp!=first; sp = sp->to->next ) {
	    SplineFindEdges(sp,ch,wi);
	    if ( first==NULL ) first = sp;
	}
	spl = spl->next;
    }
}

static real SCFindMinXAtY(SplineChar *sc,int layer, real y) {
    real min = NOTREACHED;
    RefChar *ref;

    min = SSFindMinXAtY(sc->layers[layer].splines,y,NOTREACHED);
    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	min = SSFindMinXAtY(ref->layers[0].splines,y,min);
return( min );
}

static int SCIsMinXAtYCurved(SplineChar *sc,int layer,real y) {
    real min = NOTREACHED;
    int curved = false;
    RefChar *ref;

    min = SSFindMinXAtY(sc->layers[layer].splines,y,NOTREACHED);
    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	min = SSIsMinXAtYCurved(ref->layers[0].splines,y,min,&curved);
return( curved );
}

static void SCFindEdges(struct charone *ch,WidthInfo *wi) {
    RefChar *ref;
    SplineChar *sc;
    int i;
    DBounds bb;

    SplineCharQuickConservativeBounds(ch->sc,&bb);
    ch->base = rint(bb.miny/wi->decimation);
    ch->top = rint(bb.maxy/wi->decimation);
    ch->ledge = malloc((ch->top-ch->base+1)*sizeof(short));
    ch->redge = malloc((ch->top-ch->base+1)*sizeof(short));
    for ( i=0; i<=ch->top-ch->base; ++i )
	ch->ledge[i] = ch->redge[i] = NOTREACHED;
    SSFindEdges(ch->sc->layers[wi->layer].splines,ch,wi);
    for ( ref=ch->sc->layers[wi->layer].refs; ref!=NULL; ref=ref->next )
	SSFindEdges(ref->layers[0].splines,ch,wi);
    ch->lbearing = ch->rmax = NOTREACHED;
    for ( i=0; i<=ch->top-ch->base; ++i ) {
	if ( ch->ledge[i]!=NOTREACHED )
	    if ( ch->lbearing==NOTREACHED || ch->ledge[i]<ch->lbearing )
		ch->lbearing = ch->ledge[i];
	if ( ch->redge[i]!=NOTREACHED )
	    if ( ch->rmax==NOTREACHED || ch->redge[i]>ch->rmax )
		ch->rmax = ch->redge[i];
    }

    /* In accented characters find the base letter, compute its dimensions */
    /*  then figure out its serif zones */
    sc = ch->sc;
    while ( sc->layers[wi->layer].refs!=NULL ) {
	for ( ref=ch->sc->layers[wi->layer].refs; ref!=NULL; ref=ref->next )
	    if ( ref->sc->unicodeenc!=-1 && isalpha(ref->sc->unicodeenc))
	break;
	if ( ref==NULL )
    break;
	sc = ref->sc;
    }
    SplineCharQuickBounds(ch->sc,&bb);
    if ( sc->unicodeenc=='k' ) {
	ch->baseserif = 1;
	ch->lefttops = 3;
	ch->righttops = 2;
    } else {
	ch->baseserif = ( bb.miny>=0 || -bb.miny<-wi->descent/2 )? 1 : 0;
	ch->lefttops = ch->righttops =
		( bb.maxy<=wi->xheight || bb.maxy-wi->xheight<wi->caph-bb.maxy )? 2 : 3;
    }
}

/* See the discussion at LineFindLeftDistance for the basic idea on guessing */
/*  at a visual distance. However things are a bit more complicated here */
/*  because we've actually got something on the other side which we compare */
/*  to. Consider the Fz combination. The cross bar of the F is usually slightly */
/*  lower than the top bar of the z. Which means it can slide nicely into the */
/*  middle of "z". But that looks really bad. Instead we need a fudge zone */
/*  that extends around every point on the edge */
static void PairFindDistance(struct charpair *cp,WidthInfo *wi) {
    int i,j, wasserif, wasseriff;
    real sum, cnt, min, fudge, minf, temp;
    struct charone *left=cp->left, *right=cp->right;
    int fudgerange;

    fudgerange = rint(wi->caph/(20*wi->decimation) );
    if ( wi->serifsize!=0 )		/* the serifs provide some fudging themselves */
	fudgerange = rint(wi->caph/(30*wi->decimation) );

    cp->base = (left->base>right->base?left->base : right->base) - fudgerange;
    cp->top = (left->top<right->top ? left->top : right->top) + fudgerange;
    if ( cp->top<cp->base )
	cp->distances = malloc(sizeof(short));
    else
	cp->distances = malloc((cp->top-cp->base+1)*sizeof(short));

    min = NOTREACHED; wasserif = false;
    for ( i=cp->base; i<=cp->top; ++i ) {
	cp->distances[i-cp->base] = NOTREACHED;
	if ( i>=left->base && i<=left->top &&
		left->redge[i-left->base]!=NOTREACHED ) {
	    minf = NOTREACHED; wasseriff = false;
	    for ( j=i-fudgerange ; j<=i+fudgerange; ++j ) {
		if ( j>=right->base && j<=right->top &&
			right->ledge[j-right->base]!=NOTREACHED ) {
		    temp = right->ledge[j-right->base]-right->lbearing +
			    left->rmax-left->redge[i-left->base];
		    if ( minf==NOTREACHED || temp<minf ) {
			minf = temp;
			wasseriff = ((i>=wi->serifs[left->baseserif][0] && i<=wi->serifs[left->baseserif][1]) ||
				(i>=wi->serifs[left->lefttops][0] && i<=wi->serifs[left->lefttops][1]) ||
				(j>=wi->serifs[right->baseserif][0] && j<=wi->serifs[right->baseserif][1]) ||
				(j>=wi->serifs[right->righttops][0] && j<=wi->serifs[right->righttops][1]));
		    }
		}
	    }
	    cp->distances[i-cp->base] = minf;
	    if ( minf!=NOTREACHED && ( min==NOTREACHED || min>minf )) {
		min = minf;
		wasserif = wasseriff;
	    }
	}
    }

    fudge = (wi->sf->ascent+wi->sf->descent)/100;
    if ( min==NOTREACHED )
	cp->visual=0;
    else {
	sum = cnt = 0;
	for ( i=cp->base; i<=cp->top; ++i ) {
	    if ( cp->distances[i-cp->base]!=NOTREACHED &&
		    cp->distances[i-cp->base]<=min+fudge ) {
		++cnt;
		sum += cp->distances[i-cp->base];
	    }
	}
	if ( cnt==0 )
	    cp->visual = min;		/* Can't happen */
	else
	    cp->visual = (min+sum/cnt)/2;
	if ( !wasserif )
	    cp->visual -= wi->seriflength/2;
    }
}

void AW_FindFontParameters(WidthInfo *wi) {
    DBounds bb;
    SplineFont *sf=wi->sf;
    unsigned i, j;
    int si=-1;
    real caph, ds, xh, serifsize, angle, ca, seriflength = 0;
    int cnt;
    static unichar_t caps[] = { 'A', 'Z', 0x391, 0x3a9, 0x40f, 0x418, 0x41a, 0x42f, 0 };
    static unichar_t descent[] = { 'p','q','g','y','j',
	    0x3c8, 0x3b7, 0x3b3, 0x3b2, 0x3b6, 0x3bc, 0x3be, 0x3c1, 0x3c6,
	    0x444, 0x443, 0x458, 0x434, 0 };
    static unichar_t xheight[] = { 'x','u','v','w','y','z',
	    0x3b3, 0x3b9, 0x3ba, 0x3bc, 0x3bd, 0x3c0, 0x3c4, 0x3c5, 0x3c7, 0x3c8,
	    0x432, 0x433, 0x436, 0x438, 0x43a, 0x43d, 0x43f, 0x442, 0x443, 0x445,
	    0x446, 0x447, 0x448, 0x449, 0 };
    static unichar_t easyserif[] = { 'I','B','D','E','F','H','K','L','N','P','R',
	    0x399, 0x406, 0x392, 0x393, 0x395, 0x397, 0x39a,
	    0x3a0, 0x3a1, 0x40a, 0x412, 0x413, 0x415, 0x41a, 0x41d, 0x41f,
	    0x420, 0x428, 0 };
    real stemx, testx, y, ytop, ybottom, yorig, topx, bottomx;

    caph = 0; cnt = 0;
    for ( i=0; caps[i]!='\0' && cnt<5; i+=2 )
	for ( j=caps[i]; j<=caps[i+1] && cnt<5; ++j )
	    if ( (si=SFFindExistingSlot(sf,j,NULL))!=-1 && sf->glyphs[si]!=NULL ) {
		SplineCharQuickBounds(sf->glyphs[si],&bb);
		caph += bb.maxy;
		++cnt;
	    }
    if ( cnt!=0 )
	caph /= cnt;
    else
	caph = sf->ascent;

    for ( i=0; descent[i]!='\0'; ++i )
	if ( (si=SFFindExistingSlot(sf,descent[i],NULL))!=-1 && sf->glyphs[si]!=NULL )
    break;
    if ( descent[i]!='\0' ) {
	SplineCharQuickBounds(sf->glyphs[si],&bb);
	ds = bb.miny;
    } else
	ds = -sf->descent;

    cnt = 0; xh = 0;
    for ( i=0; xheight[i]!='\0' && cnt<5; ++i )
	if ( (si=SFFindExistingSlot(sf,xheight[i],NULL))!=-1 && sf->glyphs[si]!=NULL ) {
	    SplineCharQuickBounds(sf->glyphs[si],&bb);
	    xh += bb.maxy;
	    ++cnt;
	}
    if ( cnt!=0 )
	xh /= cnt;
    else
	xh = 3*caph/4;

    for ( i=0; easyserif[i]!='\0'; ++i )
	if ( (si=SFFindExistingSlot(sf,easyserif[i],NULL))!=-1 && sf->glyphs[si]!=NULL )
    break;
    if ( si!=-1 ) {
	topx = SCFindMinXAtY(sf->glyphs[si],wi->layer,2*caph/3);
	bottomx = SCFindMinXAtY(sf->glyphs[si],wi->layer,caph/3);
	/* Some fonts don't sit on the baseline... */
	SplineCharQuickBounds(sf->glyphs[si],&bb);
	/* beware of slanted (italic, oblique) fonts */
	ytop = caph/2; ybottom=bb.miny;
	stemx = SCFindMinXAtY(sf->glyphs[si],wi->layer,ytop);
	if ( topx==bottomx ) {
	    ca = 0;
	    yorig = 0;	/* Irrelevant because we will multiply it by 0, but makes gcc happy */
	    while ( ytop-ybottom>=.5 ) {
		y = (ytop+ybottom)/2;
		testx = SCFindMinXAtY(sf->glyphs[si],wi->layer,y);
		if ( testx+1>=stemx )
		    ytop = y;
		else
		    ybottom = y;
	    }
	} else {
	    angle = atan2(caph/3,topx-bottomx);
	    ca = cos(angle);
	    yorig = ytop;
	    while ( ytop-ybottom>=.5 ) {
		y = (ytop+ybottom)/2;
		testx = SCFindMinXAtY(sf->glyphs[si],wi->layer,y)+
		    (yorig-y)*ca;
		if ( testx+4>=stemx )		/* the +4 is to counteract rounding */
		    ytop = y;
		else
		    ybottom = y;
	    }
	}
	/* If "I" has a curved stem then it's probably in a script style and */
	/*  serifs don't really make sense (or not the simplistic ones I deal with) */
	if ( ytop<=bb.miny+.5 || SCIsMinXAtYCurved(sf->glyphs[si],wi->layer,caph/2) )
	    serifsize = 0;
	else if ( ytop>caph/4 )
	    serifsize = /*.06*(sf->ascent+sf->descent)*/ 0;
	else
	    serifsize = ytop-bb.miny;

	if ( serifsize!=0 ) {
	    y = serifsize/4 + bb.miny;
	    testx = SCFindMinXAtY(sf->glyphs[si],wi->layer,y);
	    if ( testx==NOTREACHED )
		serifsize=0;
	    else {
		testx += (yorig-y)*ca;
		seriflength = stemx-testx;
		if ( seriflength < (sf->ascent+sf->descent)/200 )
		    serifsize = 0;
	    }
	}
    } else
	serifsize = .06*(sf->ascent+sf->descent);
    serifsize = rint(serifsize);
    if ( seriflength>.1*(sf->ascent+sf->descent) || serifsize<0 ) {
	seriflength = 0;		/* that's an unreasonable value, we must be wrong */
	serifsize = 0;
    }

    if ( (si=SFFindExistingSlot(sf,'n',"n"))!=-1 && sf->glyphs[si]!=NULL ) {
	SplineChar *sc = sf->glyphs[si];
	if ( sc->changedsincelasthinted && !sc->manualhints )
	    SplineCharAutoHint(sc,wi->layer,NULL);
	SplineCharQuickBounds(sc,&bb);
	if ( sc->vstem!=NULL && sc->vstem->next!=NULL ) {
	    wi->n_stem_exterior_width = sc->vstem->next->start+sc->vstem->next->width-
		    sc->vstem->start;
	    wi->n_stem_interior_width = sc->vstem->next->start-
		    (sc->vstem->start+sc->vstem->width);
	}
	if ( wi->n_stem_exterior_width<bb.maxx-bb.minx-3*seriflength ||
		wi->n_stem_exterior_width>bb.maxx-bb.minx+seriflength ||
		wi->n_stem_interior_width <= 0 ) {
	    wi->n_stem_exterior_width = bb.maxx-bb.minx - 2*seriflength;
	    /* guess that the stem width is somewhere around the seriflength and */
	    /*  one quarter of the character width */
	    wi->n_stem_interior_width = wi->n_stem_exterior_width - seriflength -
		    wi->n_stem_exterior_width/4;
	}
    }
    if ( ((si=SFFindExistingSlot(sf,'I',"I"))!=-1 && sf->glyphs[si]!=NULL ) ||
	    ((si=SFFindExistingSlot(sf,0x399,"Iota"))!=-1 && sf->glyphs[si]!=NULL ) ||
	    ((si=SFFindExistingSlot(sf,0x406,"afii10055"))!=-1 && sf->glyphs[si]!=NULL ) ) {
	SplineChar *sc = sf->glyphs[si];
	SplineCharQuickBounds(sc,&bb);
	wi->current_I_spacing = sc->width - (bb.maxx-bb.minx);
    }

    wi->caph = caph;
    wi->descent = ds;
    wi->xheight = xh;
    wi->serifsize = serifsize;
    wi->seriflength = seriflength;
    wi->decimation = caph<=1?10:caph/60;

    if ( serifsize==0 ) {
	wi->serifs[0][0] = wi->serifs[0][1] = wi->serifs[1][0] = wi->serifs[1][1] = NOTREACHED;
	wi->serifs[2][0] = wi->serifs[2][1] = wi->serifs[3][0] = wi->serifs[3][1] = NOTREACHED;
    } else {
	wi->serifs[0][0] = rint(ds/wi->decimation);
	wi->serifs[0][1] = rint((ds+serifsize)/wi->decimation);
	wi->serifs[1][0] = 0;
	wi->serifs[1][1] = rint(serifsize/wi->decimation);
	wi->serifs[2][0] = rint((xh-serifsize)/wi->decimation);
	wi->serifs[2][1] = rint(xh/wi->decimation);
	wi->serifs[3][0] = rint((caph-serifsize)/wi->decimation);
	wi->serifs[3][1] = rint(caph/wi->decimation);
    }

    if ( wi->sf==aw_old_sf )
	wi->space_guess = aw_old_spaceguess;
    else if ( wi->autokern && wi->current_I_spacing )
	wi->space_guess = rint(wi->current_I_spacing);
    else if ( wi->n_stem_interior_width>0 )
	wi->space_guess = rint(wi->n_stem_interior_width);
    else if ( caph!=sf->ascent && ds!=-sf->descent )
	wi->space_guess = rint(.205*(caph-ds));
    else
	wi->space_guess = rint(.184*(sf->ascent+sf->descent));
}

real SFGuessItalicAngle(SplineFont *sf) {
    static const char *easyserif = "IBDEFHKLNPR";
    int i,si;
    real as, topx, bottomx;
    DBounds bb;
    double angle;

    for ( i=0; easyserif[i]!='\0'; ++i )
	if ( (si=SFFindExistingSlot(sf,easyserif[i],NULL))!=-1 && sf->glyphs[si]!=NULL )
    break;
    if ( easyserif[i]=='\0' )		/* can't guess */
return( 0 );

    SplineCharFindBounds(sf->glyphs[si],&bb);
    as = bb.maxy-bb.miny;

    topx = SCFindMinXAtY(sf->glyphs[si],ly_fore,2*as/3+bb.miny);
    bottomx = SCFindMinXAtY(sf->glyphs[si],ly_fore,as/3+bb.miny);
    if ( topx==bottomx )
return( 0 );

    angle = atan2(as/3,topx-bottomx)*180/3.1415926535897932-90;
    if ( angle<1 && angle>-1 ) angle = 0;
return( angle );
}

void AW_InitCharPairs(WidthInfo *wi) {
    int i, j;
    struct charpair *cp;

    wi->pcnt = wi->lcnt*wi->rcnt;
    wi->pairs = malloc(wi->pcnt*sizeof(struct charpair *));
    for ( i=0; i<wi->lcnt; ++i ) for ( j=0; j<wi->rcnt; ++j ) {
	wi->pairs[i*wi->rcnt+j] = cp = calloc(1,sizeof(struct charpair));
	cp->left = wi->left[i];
	cp->right = wi->right[j];
	cp->nextasleft = cp->left->asleft;
	cp->left->asleft = cp;
	cp->nextasright = cp->right->asright;
	cp->right->asright = cp;
    }
    wi->tcnt = wi->lcnt+wi->rcnt;
}

void AW_BuildCharPairs(WidthInfo *wi) {
    int i;

    /* FindFontParameters(wi); */		/* Moved earlier */

    for ( i=0; i<wi->lcnt; ++i )
	SCFindEdges(wi->left[i],wi);
    for ( i=0; i<wi->rcnt; ++i )
	SCFindEdges(wi->right[i],wi);

    for ( i=0; i<wi->pcnt; ++i )
	PairFindDistance(wi->pairs[i],wi);
}

void AW_FreeCharList(struct charone **list) {
    int i;

    if ( list==NULL )
return;
    for ( i=0; list[i]!=NULL; ++i ) {
	free( list[i]->ledge );
	free( list[i]->redge );
	free( list[i] );
    }
    free(list);
}

void AW_FreeCharPairs(struct charpair **list, int cnt) {
    int i;

    if ( list==NULL )
return;
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
    totals = calloc(high+1,sizeof(int));
    tot=0;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( kp = sf->glyphs[i]->kerns; kp!=NULL; kp = kp->next ) {
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

void AW_KernRemoveBelowThreshold(SplineFont *sf,int threshold) {
    int i;
    KernPair *kp, *prev, *next;

    if ( threshold==0 )
return;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	prev = NULL;
	for ( kp = sf->glyphs[i]->kerns; kp!=NULL; kp = next ) {
	    next = kp->next;
	    if ( kp->off>=threshold || kp->off<=-threshold )
		prev = kp;
	    else {
		if ( prev==NULL )
		    sf->glyphs[i]->kerns = next;
		else
		    prev->next = next;
		chunkfree(kp,sizeof(KernPair));
	    }
	}
    }
    MVReKernAll(sf);
}

struct charone *AW_MakeCharOne(SplineChar *sc) {
    struct charone *ch = calloc(1,sizeof(struct charone));

    ch->sc = sc;
    ch->newr = ch->newl = NOTREACHED;
return( ch );
}

struct kernsets {
    unichar_t *ch1;
    int max, cur;
    unichar_t **ch2s;		/* first level array is same dim as ch1 */
};

static unichar_t *ugetstr(FILE *file,int format,unichar_t *buffer,int len) {
    int ch, ch2;
    unichar_t *upt = buffer;

    if ( format==0 ) {
	while ( (ch=getc(file))!='\n' && ch!='\r' && ch!=EOF ) {
	    if ( upt<buffer+len-1 )
		*upt++ = ch;
	}
	if ( ch=='\r' ) {
	    ch = getc(file);
	    if ( ch!='\n' )
		ungetc(ch,file);
	}
    } else {
	for (;;) {
	    ch = getc(file);
	    ch2 = getc(file);
	    if ( format==1 )
		ch = (ch<<8)|ch2;
	    else
		ch = (ch2<<8)|ch;
	    if ( ch2==EOF ) {
		ch = EOF;
	break;
	    } else if ( ch=='\n' || ch=='\r' )
	break;
	    if ( upt<buffer+len-1 )
		*upt++ = ch;
	}
	if ( ch=='\r' ) {
	    ch = getc(file);
	    ch2 = getc(file);
	    if ( ch2!=EOF ) {
		if ( format==1 )
		    ch = (ch<<8)|ch2;
		else
		    ch = (ch2<<8)|ch;
		if ( ch!='\n' )
		    fseek(file,-2,SEEK_CUR);
	    }
	}
    }

    if ( ch==EOF && upt==buffer )
return( NULL );
    *upt = '\0';

    for ( upt=buffer; *upt; ++upt ) {
	if ( (*upt=='U' || *upt=='u') && upt[1]=='+' && ishexdigit(upt[2]) &&
		ishexdigit(upt[3]) && ishexdigit(upt[4]) && ishexdigit(upt[5]) ) {
	    ch = isdigit(upt[2]) ? upt[2]-'0' : islower(upt[2]) ? upt[2]-'a'+10 : upt[2]-'A'+10;
	    ch = (ch<<4) + (isdigit(upt[3]) ? upt[3]-'0' : islower(upt[3]) ? upt[3]-'a'+10 : upt[3]-'A'+10);
	    ch = (ch<<4) + (isdigit(upt[4]) ? upt[4]-'0' : islower(upt[4]) ? upt[4]-'a'+10 : upt[4]-'A'+10);
	    ch = (ch<<4) + (isdigit(upt[5]) ? upt[5]-'0' : islower(upt[5]) ? upt[5]-'a'+10 : upt[5]-'A'+10);
	    *upt = ch;
	    u_strcpy(upt+1,upt+6);
	}
    }
return( buffer );
}

static void parsekernstr(unichar_t *buffer,struct kernsets *ks) {
    int i,j,k;

    /* Any line not parseable as a kern pair is ignored */
    if ( u_strlen(buffer)!=2 )
return;

    for ( i=0 ; i<ks->cur && buffer[0]>ks->ch1[i]; ++i );
    if ( i>=ks->cur || buffer[0]!=ks->ch1[i] ) {
	if ( ks->cur+1>=ks->max ) {
	    ks->max += 100;
	    if ( ks->cur==0 ) {
		ks->ch1 = malloc(ks->max*sizeof(unichar_t));
		ks->ch2s = malloc(ks->max*sizeof(unichar_t *));
	    } else {
		ks->ch1 = realloc(ks->ch1,ks->max*sizeof(unichar_t));
		ks->ch2s = realloc(ks->ch2s,ks->max*sizeof(unichar_t *));
	    }
	}
	for ( j=ks->cur; j>i; --j ) {
	    ks->ch1[j] = ks->ch1[j-1];
	    ks->ch2s[j] = ks->ch2s[j-1];
	}
	ks->ch1[i] = buffer[0];
	ks->ch2s[i] = malloc(50*sizeof(unichar_t));
	ks->ch2s[i][0] = '\0';
	++ks->cur;
    }
    if ( (u_strlen(ks->ch2s[i])+1)%50 == 0 )
	ks->ch2s[i] = realloc(ks->ch2s[i],(u_strlen(ks->ch2s[i])+50)*sizeof(unichar_t));
    for ( j=0 ; ks->ch2s[i][j]!=0 && buffer[1]>ks->ch2s[i][j]; ++j );
    if ( ks->ch2s[i][j]!=buffer[1] ) {
	for ( k=u_strlen(ks->ch2s[i])+1; k>j; --k )
	    ks->ch2s[i][k] = ks->ch2s[i][k-1];
	ks->ch2s[i][j] = buffer[1];
    }
}

void AW_ScriptSerifChecker(WidthInfo *wi) {
    /* If not LGC (latin, greek, cyrillic) then ignore serif checks */
    /*  What about letterlike-symbols? */
    if (( wi->left[0]->sc->unicodeenc>='A' && wi->left[0]->sc->unicodeenc<0x530) ||
	     ( wi->left[0]->sc->unicodeenc>=0x1d00 && wi->left[0]->sc->unicodeenc<0x2000)) {
	 /* They are working with letters where serif checks are reasonable */
    } else {
	wi->serifsize = wi->seriflength = 0;
	wi->serifs[0][0] = wi->serifs[0][1] = NOTREACHED;
	wi->serifs[1][0] = wi->serifs[1][1] = NOTREACHED;
	wi->serifs[2][0] = wi->serifs[2][1] = NOTREACHED;
	wi->serifs[3][0] = wi->serifs[3][1] = NOTREACHED;
    }
}

static int figurekernsets(WidthInfo *wi,struct kernsets *ks) {
    int i,j,k;
    unsigned cnt,lcnt,max;
    unichar_t *ch2s;
    unichar_t *cpt, *upt;
    struct charpair *cp;
    SplineFont *sf = wi->sf;

    if ( ks->cur==0 )
return( false );

    wi->left = malloc((ks->cur+1)*sizeof(struct charone *));
    for ( i=cnt=0; i<ks->cur; ++i ) {
	j = SFFindExistingSlot(sf,ks->ch1[i],NULL);
	if ( j!=-1 && sf->glyphs[j]!=NULL &&
		(sf->glyphs[j]->layers[wi->layer].splines!=NULL || sf->glyphs[j]->layers[wi->layer].refs!=NULL ))
	    wi->left[cnt++] = AW_MakeCharOne(sf->glyphs[j]);
	else
	    ks->ch1[i] = '\0';
    }
    wi->lcnt = cnt;
    wi->left[cnt] = NULL;
    if ( cnt==0 ) {
	free(wi->left); wi->left = NULL;
return( false );
    }

    for ( i=max=0; i<ks->cur; ++i )
	if ( ks->ch1[i]!='\0' )
	    max += u_strlen(ks->ch2s[i]);
    ch2s = malloc((max+1)*sizeof(unichar_t));
    for ( i=0; i<ks->cur && ks->ch1[i]=='\0'; ++i );
    u_strcpy(ch2s,ks->ch2s[i]);
    for ( ++i; i<ks->cur; ++i ) if ( ks->ch1[i]!='\0' ) {
	for ( upt=ks->ch2s[i]; *upt!='\0'; ++upt ) {
	    for ( cpt = ch2s; *cpt!='\0' && *upt<*cpt; ++cpt );
	    if ( *cpt==*upt )	/* already listed */
	continue;
	    for ( k=u_strlen(cpt)+1; k>0; --k )
		cpt[k] = cpt[k-1];
	    *cpt = *upt;
	}
    }

    wi->right = malloc((u_strlen(ch2s)+1)*sizeof(struct charone *));
    for ( cnt=0,cpt=ch2s; *cpt ; ++cpt ) {
	j = SFFindExistingSlot(sf,*cpt,NULL);
	if ( j!=-1 && sf->glyphs[j]!=NULL &&
		(sf->glyphs[j]->layers[wi->layer].splines!=NULL || sf->glyphs[j]->layers[wi->layer].refs!=NULL ))
	    wi->right[cnt++] = AW_MakeCharOne(sf->glyphs[j]);
    }
    wi->rcnt = cnt;
    wi->right[cnt] = NULL;
    free( ch2s );
    if ( cnt==0 ) {
	free(wi->left); wi->left = NULL;
	free(wi->right); wi->right = NULL;
return( false );
    }
    AW_ScriptSerifChecker(wi);

    wi->pairs = malloc(max*sizeof(struct charpair *));
    for ( i=lcnt=cnt=0; i<ks->cur; ++i ) if ( ks->ch1[i]!='\0' ) {
	for ( cpt=ks->ch2s[i]; *cpt; ++cpt ) {
	    for ( j=0; j<wi->rcnt && (unichar_t)wi->right[j]->sc->unicodeenc!=*cpt; ++j );
	    if ( j<wi->rcnt ) {
		wi->pairs[cnt++] = cp = calloc(1,sizeof(struct charpair));
		cp->left = wi->left[lcnt];
		cp->right = wi->right[j];
		cp->nextasleft = cp->left->asleft;
		cp->left->asleft = cp;
		cp->nextasright = cp->right->asright;
		cp->right->asright = cp;
	    }
	}
	++lcnt;
    }
    wi->pcnt = cnt;
return( true );
}

static void kernsetsfree(struct kernsets *ks) {
    int i;

    for ( i=0; i<ks->cur; ++i )
	free(ks->ch2s[i]);
    free(ks->ch2s);
    free(ks->ch1);
}

int AW_ReadKernPairFile(char *fn,WidthInfo *wi) {
    char *filename;
    FILE *file;
    int ch, format=0;
    unichar_t buffer[300];
    struct kernsets ks;

    filename = utf82def_copy(fn);
    file = fopen(filename,"r");
    free( filename );
    if ( file==NULL ) {
	ff_post_error(_("Couldn't open file"), _("Couldn't open file %.200s"), fn );
	free(fn);
return( false );
    }

    ch = getc(file);
    if ( ch==0xff || ch==0xfe ) {
	int ch2 = getc(file);
	if ( ch==0xfe && ch2==0xff )
	    format = 1;		/* normal ucs2 */
	else if ( ch==0xff && ch2==0xfe )
	    format = 2;		/* byte-swapped ucs2 */
	else
	    rewind(file);
    } else
	ungetc(ch,file);

    memset(&ks,0,sizeof(ks));
    while ( ugetstr(file,format,buffer,sizeof(buffer)/sizeof(buffer[0]))!=NULL )
	parsekernstr(buffer,&ks);

    fclose(file);
    if ( !figurekernsets(wi,&ks)) {
	ff_post_error(_("No Kern Pairs"), _("No kerning pairs found in %.200s"), fn );
	kernsetsfree(&ks);
return( false );
    }
    kernsetsfree(&ks);
    free( fn );
return( true );
}

void FVRemoveKerns(FontViewBase *fv) {
    int changed = false;
    SplineFont *sf = fv->sf;
    OTLookup *otl, *notl;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    for ( otl=sf->gpos_lookups; otl!=NULL; otl = notl ) {
	notl = otl->next;
	if ( otl->lookup_type==gpos_pair &&
		FeatureTagInFeatureScriptList(CHR('k','e','r','n'),otl->features)) {
	    SFRemoveLookup(sf,otl,0);
	    changed = true;
	}
    }
    if ( changed ) {
	sf->changed = true;
	MVReKernAll(fv->sf);
    }
}

void FVRemoveVKerns(FontViewBase *fv) {
    int changed = false;
    SplineFont *sf = fv->sf;
    OTLookup *otl, *notl;

    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    for ( otl=sf->gpos_lookups; otl!=NULL; otl = notl ) {
	notl = otl->next;
	if ( otl->lookup_type==gpos_pair &&
		FeatureTagInFeatureScriptList(CHR('v','k','r','n'),otl->features)) {
	    SFRemoveLookup(sf,otl,0);
	    changed = true;
	}
    }
    if ( changed ) {
	fv->sf->changed = true;
	MVReKernAll(fv->sf);
    }
}

static SplineChar *SCHasVertVariant(SplineChar *sc) {
    PST *pst;

    if ( sc==NULL )
return( NULL );

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type==pst_substitution &&
		(FeatureTagInFeatureScriptList(CHR('v','e','r','t'),pst->subtable->lookup->features) ||
		 FeatureTagInFeatureScriptList(CHR('v','r','t','2'),pst->subtable->lookup->features)) ) {
return( SFGetChar(sc->parent,-1,pst->u.subs.variant));
	}
    }
return( NULL );
}

static SplineChar **CharNamesToVertSC(SplineFont *sf,char *names ) {
    char *pt, *end, ch;
    int cnt;
    SplineChar **list;

    if ( names==NULL || *names=='\0' )
return( NULL );
    cnt=1;
    for ( pt=names; (pt=strchr(pt,' '))!=NULL; ++pt )
	++cnt;
    list = calloc(cnt+1,sizeof(SplineChar *));

    cnt = 0;
    for ( pt=names ; *pt ; pt = end ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	end = strchr(pt,' ');
	if ( end==NULL ) end = pt+strlen(pt);
	ch = *end; *end = '\0';
	list[cnt] = SCHasVertVariant( SFGetChar(sf,-1,pt));
	*end = ch;
	if ( list[cnt]!=NULL )
	    ++cnt;
    }
    if ( cnt==0 ) {
	free(list);
	list = NULL;
    }
return( list );
}

static char *SCListToName(SplineChar **sclist) {
    int i, len;
    char *names, *pt;

    for ( i=len=0; sclist[i]!=NULL; ++i )
	len += strlen(sclist[i]->name)+1;
    names = pt = malloc(len+1);
    *pt = '\0';
    for ( i=0; sclist[i]!=NULL; ++i ) {
	strcat(pt,sclist[i]->name);
	strcat(pt," ");
	pt += strlen(pt);
    }
    if ( pt>names ) pt[-1] = '\0';
return( names );
}

struct otlmap { OTLookup *from, *to; };
struct submap { struct lookup_subtable *from, *to; };

struct lookupmap {
    int lc, sc;
    struct otlmap *lmap;
    struct submap *smap;
    SplineFont *sf;
};

static struct lookup_subtable *VSubtableFromH(struct lookupmap *lookupmap,struct lookup_subtable *sub) {
    int i, lc, sc;
    OTLookup *otl;
    struct lookup_subtable *nsub, *prev, *test, *ls;
    FeatureScriptLangList *fl;

    for ( i=0 ; i<lookupmap->sc; ++i )
	if ( lookupmap->smap[i].from == sub )
return( lookupmap->smap[i].to );

    if ( lookupmap->lmap==NULL ) {
	for ( otl = lookupmap->sf->gpos_lookups, lc=sc=0; otl!=NULL; otl=otl->next ) {
	    if ( otl->lookup_type==gpos_pair ) {
		++lc;
		for ( ls=otl->subtables; ls!=NULL; ls=ls->next )
		    ++sc;
	    }
	}
	lookupmap->lmap = malloc(lc*sizeof(struct otlmap));
	lookupmap->smap = malloc(sc*sizeof(struct submap));
    }

    for ( i=0 ; i<lookupmap->lc; ++i )
	if ( lookupmap->lmap[i].from == sub->lookup )
    break;

    if ( i==lookupmap->lc ) {
	++lookupmap->lc;
	lookupmap->lmap[i].from = sub->lookup;
	lookupmap->lmap[i].to = otl = chunkalloc(sizeof(OTLookup));
	otl->lookup_type = gpos_pair;
	otl->features = FeatureListCopy(sub->lookup->features);
	for ( fl=otl->features; fl!=NULL; fl=fl->next )
	    if ( fl->featuretag == CHR('k','e','r','n') )
		fl->featuretag = CHR('v','k','r','n');
	otl->lookup_name = strconcat("V",sub->lookup->lookup_name);
	otl->next = sub->lookup->next;
	sub->lookup->next = otl;
    } else
	otl = lookupmap->lmap[i].to;

    sc = lookupmap->sc++;
    lookupmap->smap[sc].from = sub;
    lookupmap->smap[sc].to = nsub = chunkalloc(sizeof(struct lookup_subtable));
    nsub->subtable_name = strconcat("V",sub->subtable_name);
    nsub->per_glyph_pst_or_kern = sub->per_glyph_pst_or_kern;
    nsub->vertical_kerning = true;
    nsub->lookup = otl;

    /* Order the subtables of the new lookup the same way they are ordered */
    /*  in the old. However there may be holes (subtables which don't get */
    /*  converted) */
    prev = NULL;
    for ( test=sub->lookup->subtables; test!=NULL && test!=sub; test=test->next ) {
	for ( i=0 ; i<lookupmap->sc; ++i )
	    if ( lookupmap->smap[i].from == test ) {
		prev = lookupmap->smap[i].to;
	break;
	    }
    }
    if ( prev!=NULL ) {
	nsub->next = prev->next;
	prev->next = nsub;
    } else {
	nsub->next = otl->subtables;
	otl->subtables = nsub;
    }
return( nsub );
}

void FVVKernFromHKern(FontViewBase *fv) {
    int i,j;
    KernPair *kp, *vkp;
    SplineChar *sc1, *sc2;
    KernClass *kc, *vkc;
    SplineChar ***firsts, ***seconds;
    int any1, any2;
    SplineFont *sf = fv->sf;
    int *map1, *map2;
    struct lookupmap lookupmap;

    FVRemoveVKerns(fv);
    if ( sf->cidmaster ) sf = sf->cidmaster;
    if ( !sf->hasvmetrics )
return;
    memset(&lookupmap,0,sizeof(lookupmap));
    lookupmap.sf = sf;

    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( (sc1 = SCHasVertVariant(sf->glyphs[i]))!=NULL ) {
	    for ( kp = sf->glyphs[i]->kerns; kp!=NULL; kp=kp->next ) {
		if ( (sc2 = SCHasVertVariant(kp->sc))!=NULL ) {
		    vkp = chunkalloc(sizeof(KernPair));
		    *vkp = *kp;
		    vkp->subtable = VSubtableFromH(&lookupmap,kp->subtable);
		    vkp->adjust = DeviceTableCopy(vkp->adjust);
		    vkp->sc = sc2;
		    vkp->next = sc1->vkerns;
		    sc1->vkerns = vkp;
		}
	    }
	}
    }

    for ( kc = sf->kerns; kc!=NULL; kc=kc->next ) {
	firsts = malloc(kc->first_cnt*sizeof(SplineChar *));
	map1 = calloc(kc->first_cnt,sizeof(int));
	seconds = malloc(kc->second_cnt*sizeof(SplineChar *));
	map2 = calloc(kc->second_cnt,sizeof(int));
	any1=0;
	for ( i=1; i<kc->first_cnt; ++i ) {
	    if ( (firsts[i] = CharNamesToVertSC(sf,kc->firsts[i]))!=NULL )
		map1[i] = ++any1;
	}
	any2 = 0;
	for ( i=1; i<kc->second_cnt; ++i ) {
	    if ((seconds[i] = CharNamesToVertSC(sf,kc->seconds[i]))!=NULL )
		map2[i] = ++any2;
	}
	if ( any1 && any2 ) {
	    vkc = chunkalloc(sizeof(KernClass));
	    *vkc = *kc;
	    vkc->subtable = VSubtableFromH(&lookupmap,kc->subtable);
	    vkc->subtable->kc = vkc;
	    vkc->next = sf->vkerns;
	    sf->vkerns = vkc;
	    vkc->first_cnt = any1+1;
	    vkc->second_cnt = any2+1;
	    vkc->firsts = calloc(any1+1,sizeof(char *));
	    for ( i=0; i<kc->first_cnt; ++i ) if ( map1[i]!=0 )
		vkc->firsts[map1[i]] = SCListToName(firsts[i]);
	    vkc->seconds = calloc(any2+1,sizeof(char *));
	    for ( i=0; i<kc->second_cnt; ++i ) if ( map2[i]!=0 )
		vkc->seconds[map2[i]] = SCListToName(seconds[i]);
	    vkc->offsets = calloc((any1+1)*(any2+1),sizeof(int16));
	    vkc->adjusts = calloc((any1+1)*(any2+1),sizeof(DeviceTable));
	    for ( i=0; i<kc->first_cnt; ++i ) if ( map1[i]!=0 ) {
		for ( j=0; j<kc->second_cnt; ++j ) if ( map2[j]!=0 ) {
		    int n=map1[i]*vkc->second_cnt+map2[j], o = i*kc->second_cnt+j;
		    vkc->offsets[n] = kc->offsets[o];
		    if ( kc->adjusts[o].corrections!=NULL ) {
			int len = kc->adjusts[o].last_pixel_size - kc->adjusts[o].first_pixel_size + 1;
			vkc->adjusts[n] = kc->adjusts[o];
			vkc->adjusts[n].corrections = malloc(len);
			memcpy(vkc->adjusts[n].corrections,kc->adjusts[o].corrections,len);
		    }
		}
	    }
	}
	free(map1);
	free(map2);
	for ( i=1; i<kc->first_cnt; ++i )
	    free(firsts[i]);
	for ( i=1; i<kc->second_cnt; ++i )
	    free(seconds[i]);
	free(firsts);
	free(seconds);
    }
    free( lookupmap.lmap );
    free( lookupmap.smap );
 }

/* Scripting hooks */

static struct charone **autowidthBuildCharList(FontViewBase *fv, SplineFont *sf,
	int *tot, int *rtot, int *ipos, int iswidth) {
    int i, cnt, doit, s;
    struct charone **ret=NULL;
    EncMap *map = fv->map;
    int gid;

    for ( doit=0; doit<2; ++doit ) {
      for ( i=cnt=0; i<map->enccount && cnt<300; ++i ) {
	if ( fv->selected[i] && (gid=map->map[i])!=-1 && SCWorthOutputting(sf->glyphs[gid])) {
	  if ( doit )
	    ret[cnt++] = AW_MakeCharOne(sf->glyphs[gid]);
	  else
	    ++cnt;
	}
      }
      
      if ( !doit )
	ret = malloc((cnt+2)*sizeof(struct charone *));
      else {
	*rtot = cnt;
	if ( iswidth &&		/* I always want 'I' in the character list when doing widths */
				    /*  or at least when doing widths of LGC alphabets where */
				    /*  concepts like serifs make sense */
		(( ret[0]->sc->unicodeenc>='A' && ret[0]->sc->unicodeenc<0x530) ||
		 ( ret[0]->sc->unicodeenc>=0x1d00 && ret[0]->sc->unicodeenc<0x2000)) ) {
	    for ( s=0; s<cnt; ++s )
		if ( ret[s]->sc->unicodeenc=='I' )
	    break;
	    if ( s==cnt ) {
		i = SFFindExistingSlot(sf,'I',NULL);
		if ( i!=-1 )
		    ret[cnt++] = AW_MakeCharOne(sf->glyphs[i]);
		else
		    s = -1;
	    }
	    *ipos = s;
	}
	ret[cnt] = NULL;
      }
    }
    *tot = cnt;
    return( ret );
}

int AutoWidthScript(FontViewBase *fv,int spacing) {
    WidthInfo wi;
    SplineFont *sf = fv->sf;

    memset(&wi,'\0',sizeof(wi));
    wi.autokern = 0;
    wi.sf = sf;
    wi.fv = fv;
    AW_FindFontParameters(&wi);
    if ( spacing>-(sf->ascent+sf->descent) )
	wi.spacing = spacing;

    wi.left = autowidthBuildCharList(wi.fv, wi.sf, &wi.lcnt, &wi.real_lcnt, &wi.l_Ipos, true );
    wi.right = autowidthBuildCharList(wi.fv, wi.sf, &wi.rcnt, &wi.real_rcnt, &wi.r_Ipos, true );
    if ( wi.real_lcnt==0 || wi.real_rcnt==0 ) {
	AW_FreeCharList(wi.left);
	AW_FreeCharList(wi.right);
return( 0 );
    }
    AW_ScriptSerifChecker(&wi);
    wi.done = true;
    AW_InitCharPairs(&wi);
    AW_BuildCharPairs(&wi);
    AW_AutoWidth(&wi);
    AW_FreeCharList(wi.left);
    AW_FreeCharList(wi.right);
    AW_FreeCharPairs(wi.pairs,wi.lcnt*wi.rcnt);
return( true );
}

int AutoKernScript(FontViewBase *fv,int spacing, int threshold,
	struct lookup_subtable *sub, char *kernfile) {
    WidthInfo wi;
    SplineFont *sf = fv->sf;

    memset(&wi,'\0',sizeof(wi));
    wi.autokern = 1;
    wi.sf = sf;
    wi.fv = fv;
    AW_FindFontParameters(&wi);
    if ( spacing>-(sf->ascent+sf->descent) )
	wi.spacing = spacing;    
    wi.threshold = threshold;
    wi.subtable = sub;

    if ( kernfile==NULL ) {
	wi.left = autowidthBuildCharList(wi.fv, wi.sf, &wi.lcnt, &wi.real_lcnt, &wi.l_Ipos, false );
	wi.right = autowidthBuildCharList(wi.fv, wi.sf, &wi.rcnt, &wi.real_rcnt, &wi.r_Ipos, false );
	if ( wi.lcnt==0 || wi.rcnt==0 ) {
	    AW_FreeCharList(wi.left);
	    AW_FreeCharList(wi.right);
return( false );
	}
	AW_ScriptSerifChecker(&wi);
	AW_InitCharPairs(&wi);
    } else {
	if ( !AW_ReadKernPairFile(copy(kernfile),&wi))
return( false );
    }
    wi.done = true;
    AW_BuildCharPairs(&wi);
    AW_AutoKern(&wi);
    AW_KernRemoveBelowThreshold(wi.sf,KernThreshold(wi.sf,0));
    AW_FreeCharList(wi.left);
    AW_FreeCharList(wi.right);
    AW_FreeCharPairs(wi.pairs,wi.lcnt*wi.rcnt);
return( true );
}
