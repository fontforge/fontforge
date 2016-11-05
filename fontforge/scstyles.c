/* Copyright (C) 2007-2012 by George Williams */
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
#include "autohint.h"
#include "cvundoes.h"
#include "dumppfa.h"
#include "fontforgevw.h"
#include "fvcomposite.h"
#include "fvfonts.h"
#include "lookups.h"
#include "namelist.h"
#include "scstyles.h"
#include "splineorder2.h"
#include "splineoverlap.h"
#include "splinestroke.h"
#include "splineutil.h"
#include "splineutil2.h"
#include <ustring.h>
#include <utype.h>
#include <gkeysym.h>
#include <math.h>
#include <ttf.h>
#include <stemdb.h>

static unichar_t lc_stem_str[] = { 'l', 'l', 'l', 'm', 'f', 't', 0x438, 0x43D,
	0x43f, 0x448, 0x3b9, 0 };
static unichar_t uc_stem_str[] = { 'I', 'L', 'T', 'H', 0x3a0, 0x397, 0x399,
	0x406, 0x418, 0x41d, 0x41f, 0x422, 0x428, 0 };
static unichar_t lc_botserif_str[] = { 'i', 'k', 'l', 'm', 'f', 0x433, 0x43a,
	0x43f, 0x442, 0x3c0, 0x3ba, 0 };
static unichar_t lc_topserif_str[] = { 'k', 'l', 'm', 0x444, 0x3b9, 0 };
static unichar_t descender_str[] = { 'p', 'q', 0x3b7, 0x3c1, 0x440, 0x444, 0 };

static void SSCPValidate(SplineSet *ss) {
    SplinePoint *sp, *nsp;

    while ( ss!=NULL ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->next==NULL )
	break;
	    nsp = sp->next->to;
	    if ( !sp->nonextcp && (sp->nextcp.x!=nsp->prevcp.x || sp->nextcp.y!=nsp->prevcp.y))
		IError( "Invalid 2nd order" );
	    sp = nsp;
	    if ( sp==ss->first )
	break;
	}
	ss = ss->next;
    }
}

static void SplineSetRefigure(SplineSet *ss) {
    Spline *s, *first;

    while ( ss!=NULL ) {
	first = NULL;
	for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	    if ( first == NULL ) first = s;
	    SplineRefigure(s);
	}
	ss = ss->next;
    }
}

static SplineSet *BoldSSStroke(SplineSet *ss,StrokeInfo *si,int order2,int ro) {
    SplineSet *temp;
    Spline *s1, *s2;

    /* We don't want to use the remove overlap built into SplineSetStroke because */
    /*  only looks at one contour at a time, but we need to look at all together */
    /*  else we might get some unreal internal hints that will screw up */
    /*  counter correction */
    temp = SplineSetStroke(ss,si,order2);
    if ( ro && temp!=NULL && SplineSetIntersect(temp,&s1,&s2))
	temp = SplineSetRemoveOverlap(NULL,temp,over_remove);
return( temp );
}

/* ************************************************************************** */
/* ***************** Generic resize /stem control routines. ***************** */
/* ************* Used also for building small caps and friends. ************* */
/* ************************************************************************** */

static int IsAnglePoint(SplinePoint *sp) {
    SplinePoint *psp, *nsp;
    double PrevTangent, NextTangent;

    if (sp->next == NULL || sp->prev == NULL ||
	sp->pointtype != pt_corner || sp->ttfindex == 0xffff)
return( false );

    psp = sp->prev->from;
    nsp = sp->next->to;
    PrevTangent = atan2(sp->me.y - psp->me.y, sp->me.x - psp->me.x);
    NextTangent = atan2(nsp->me.y - sp->me.y, nsp->me.x - sp->me.x);

return fabs(PrevTangent - NextTangent) > 0.261;
}

static int IsExtremum(SplinePoint *sp,int xdir) {
    SplinePoint *psp, *nsp;
    real val = (&sp->me.x)[xdir];

    if (sp->next == NULL || sp->prev == NULL )
return( false );

    psp = sp->prev->from;
    nsp = sp->next->to;
return ((val < (&psp->me.x)[xdir] && val < (&nsp->me.x)[xdir]) ||
	(val > (&psp->me.x)[xdir] && val > (&nsp->me.x)[xdir]));
}

static double InterpolateVal( double a,double b,double a1, double b1, double val ) {
return( a1 + ( val - a ) * ( b1 - a1 )/( b - a ));
}

static int IsPointFixed( PointData *pd ) {
return (( pd->touched & tf_x && pd->touched & tf_y ) ||
	( pd->touched & tf_x && pd->touched & tf_d ) ||
	( pd->touched & tf_y && pd->touched & tf_d ));
}

static int active_cmp(const void *_s1, const void *_s2) {
    const struct segment *s1 = _s1, *s2 = _s2;
    if ( s1->start<s2->start )
return( -1 );
    else if ( s1->start>s2->start )
return( 1 );

return( 0 );
}

static int fixed_cmp(const void *_s1, const void *_s2) {
    const struct position_maps *s1 = _s1, *s2 = _s2;
    if ( s1->current<s2->current )
return( -1 );
    else if ( s1->current>s2->current )
return( 1 );

return( 0 );
}

static int ds_cmp( const void *_s1, const void *_s2 ) {
    StemData * const *s1 = _s1, * const *s2 = _s2;

    BasePoint *bp1, *bp2;
    bp1 = (*s1)->unit.y > 0 ? &(*s1)->keypts[0]->base : &(*s1)->keypts[2]->base;
    bp2 = (*s2)->unit.y > 0 ? &(*s2)->keypts[0]->base : &(*s2)->keypts[2]->base;
    if ( bp1->x < bp2->x || ( bp1->x == bp2->x && bp1->y < bp2->y ))
return( -1 );
    else if ( bp2->x < bp1->x || ( bp2->x == bp1->x && bp2->y < bp1->y ))
return( 1 );

return( 0 );
}

#define czone_top 2
#define czone_bot 1
static uint8 GetStemCounterZone( StemData *stem, DBounds *orig_b ) {
    uint8 ret = 0, by_x;
    int i;
    double min, max, middle, fudge, s, e;

    if ( stem == NULL )
return( czone_top | czone_bot );
    by_x = ( stem->unit.x > stem->unit.y );
    min = by_x ? orig_b->minx : orig_b->miny;
    max = by_x ? orig_b->maxx : orig_b->maxy;
    middle = ( max - min )/2.;
    fudge = ( max - min )/16.;

    for ( i=0; i<stem->activecnt && ret < 3; i++ ) {
	s = (&stem->left.x)[!by_x] +
	    stem->active[i].start * (&stem->unit.x)[!by_x];
	e = (&stem->left.x)[!by_x] +
	    stem->active[i].end * (&stem->unit.x)[!by_x];
	if ( s < middle - fudge || e < middle - fudge )
	    ret |= czone_bot;
	if ( e > middle + fudge || s > middle + fudge )
	    ret |= czone_top;
    }
return( ret );
}

static double GetCounterBlackSpace( GlyphData *gd, StemData **dstems, int dcnt,
    DBounds *orig_b, double cstart, double cend, double pos, uint8 czone, struct genericchange *gc, int x_dir ) {

    double ldist, rdist, lrdist, is, ie, temp, black=0;
    double scale, w;
    struct segment *inters;
    StemBundle *bundle;
    StemData *stem;
    int i, j, icnt=0;

    bundle = x_dir ? gd->vbundle : gd->hbundle;
    inters = calloc( dcnt + bundle->cnt,sizeof( struct segment ));

    for ( i=0; i<dcnt; i++ ) {
	stem = dstems[i];
	/* If a stem is more horizontal than vertical, then it should not
	 * cause vertical counters to be increased, and vice versa.
	 * However we have to check both directions if stem slope angle
	 * is close to 45 degrees, as otherwise undesired random effects
	 * can occur (e. g. in a multiply sign). So we have selected 0.75
	 * (sin(48 deg. 36'), cos(41 deg. 24 min)) as a boundary value
	 */
	if (( x_dir && fabs( stem->unit.x ) > .75 ) ||
	    ( !x_dir && fabs( stem->unit.y ) > .75 ))
    continue;
	ldist = ( pos - (&stem->left.x)[x_dir] )/(&stem->unit.x)[x_dir];
	rdist = ( pos - (&stem->right.x)[x_dir] )/(&stem->unit.x)[x_dir];
	lrdist =( stem->right.x - stem->left.x ) * stem->unit.x +
		( stem->right.y - stem->left.y ) * stem->unit.y;

	for ( j=0; j<stem->activecnt; j++ ) {
	    if (ldist >= stem->active[j].start && ldist <= stem->active[j].end &&
		rdist + lrdist >= stem->active[j].start && rdist + lrdist <= stem->active[j].end ) {

		is = (&stem->left.x)[!x_dir] + ldist * (&stem->unit.x)[!x_dir];
		ie = (&stem->right.x)[!x_dir] + rdist * (&stem->unit.x)[!x_dir];
		if ( is > ie ) {
		    temp = is; is = ie; ie = temp;
		}
		if ( is >= cstart && is < cend ) {
		    inters[icnt  ].start = is;
		    inters[icnt++].end = ( ie < cend ) ? ie : cend;
		} else if ( ie > cstart && ie <=cend ) {
		    inters[icnt  ].start = ( is > cstart ) ? is : cstart;
		    inters[icnt++].end = ie;
		}
	    }
	}
    }
    for ( i=0; i<bundle->cnt; i++ ) {
	stem = bundle->stemlist[i];
	if ( stem->bbox )
    continue;
	is = x_dir ? stem->left.x : stem->right.y;
	ie = x_dir ? stem->right.x : stem->left.y;

	if ( is >= cstart && is < cend && GetStemCounterZone( stem,orig_b ) & czone ) {
	    inters[icnt  ].start = is;
	    inters[icnt++].end = ( ie < cend ) ? ie : cend;
	} else if ( ie > cstart && ie <=cend && GetStemCounterZone( stem,orig_b ) & czone  ) {
	    inters[icnt  ].start = ( is > cstart ) ? is : cstart;
	    inters[icnt++].end = ie;
	}
    }
    qsort( inters,icnt,sizeof(struct segment),active_cmp );

    for ( i=j=0; i<icnt; i++ ) {
	if ( i == 0 ) {
	    w = ( inters[i].end - inters[i].start );
	} else {
	    while ( i<icnt && inters[i].end <= inters[j].end ) i++;
	    if ( i == icnt )
    break;
	    if ( inters[i].start < inters[i-1].end )
		w = ( inters[i].end - inters[i-1].end );
	    else
		w = ( inters[i].end - inters[i].start );
	}
	if (gc->stem_threshold > 0) {
	    scale = w > gc->stem_threshold ? gc->stem_width_scale : gc->stem_height_scale;
	} else {
	    scale = x_dir ? gc->stem_width_scale : gc->stem_height_scale;
	}
	black += w*scale;
	j = i;
    }
    free( inters );
return( black );
}

/* As with expanding/condensing, we are going to assume that LCG glyphs have
 * at most two counter zones, one near the bottom (baseline), one near the top.
 * So check if the given counter is intersected by some DStems at two positions:
 * 25% glyph height and 75% glyph height. If so, then only the "white" part
 * of the counter can be scaled by the normal counter ratio, while for the space
 * covered by DStems the stem ratio should be used instead.
 * However if at least one of the stems which form the counter doesn't extend
 * to the top/bottom part of the glyph (Latin "Y" is the most obvious example),
 * then the actual counter width in that part is larger than just the space
 * between two stems (or a stem and a glyph boundary), so that expanding just
 * that space would make the counter disproportionally wide. We are attempting
 * to compensate this by subtracting a half base stem width from the "white"
 * part of the counter
 */
static double ScaleCounter( GlyphData *gd, StemData **dstems, int dcnt,
    DBounds *orig_b, StemData *pstem, StemData *nstem, struct genericchange *gc, int x_dir ) {

    double min, max, onequarter, threequarters, cstart, cend, cntr_scale;
    double black25, black75, white25, white75, gen25, gen75, ret;
    uint8 pczone, nczone;

    min = x_dir ? orig_b->minx : orig_b->miny;
    max = x_dir ? orig_b->maxx : orig_b->maxy;
    cntr_scale = x_dir ? gc->hcounter_scale : gc->vcounter_scale;
    cstart = ( pstem != NULL ) ? x_dir ? pstem->right.x : pstem->left.y : min;
    cend = ( nstem != NULL ) ? x_dir ? nstem->left.x : nstem->right.y : max;
    if ( cend == cstart )
return( 0 );

    pczone = GetStemCounterZone( pstem,orig_b );
    nczone = GetStemCounterZone( nstem,orig_b );

    min = x_dir ? orig_b->miny : orig_b->minx;
    max = x_dir ? orig_b->maxy : orig_b->maxx;
    onequarter = min + ( max - min )*.25;
    threequarters = min + ( max - min )*.75;
    black25 = GetCounterBlackSpace( gd,dstems,dcnt,orig_b,cstart,cend,onequarter,czone_bot,gc,x_dir );
    black75 = GetCounterBlackSpace( gd,dstems,dcnt,orig_b,cstart,cend,threequarters,czone_top,gc,x_dir );
    white25 = cend - cstart - black25;
    white75 = cend - cstart - black75;

    if ( !(pczone & czone_top) && white75 > white25 + pstem->width/2 )
	white75 -= pstem->width/2;
    if ( !(nczone & czone_top) && white75 > white25 + nstem->width/2 )
	white75 -= nstem->width/2;
    if ( !(pczone & czone_bot) && white25 > white75 + pstem->width/2 )
	white25 -= pstem->width/2;
    if ( !(nczone & czone_bot) && white25 > white75 + nstem->width/2 )
	white25 -= nstem->width/2;
    gen25 = white25 * cntr_scale + black25;
    gen75 = white75 * cntr_scale + black75;
    ret = ( gen25 > gen75 ) ? gen25 : gen75;

return( ret );
}

static void StemPosDependent( StemData *stem,struct genericchange *genchange,int x_dir ) {

    int i, lbase, expanded;
    StemData *slave;
    double dist, l, r, l1, r1, sl, sr;
    double stem_scale, stem_add, stroke_add, serif_scale, width_new;

    expanded = (genchange->stem_width_add != 0 && genchange->stem_height_add !=0 &&
		genchange->stem_height_add/genchange->stem_width_add > 0 );
    l = (&stem->left.x)[!x_dir]; r = (&stem->right.x)[!x_dir];
    l1 = (&stem->newleft.x)[!x_dir]; r1 = (&stem->newright.x)[!x_dir];
    if ( x_dir ) {
	stem_scale = genchange->stem_width_scale;
	stem_add = genchange->stem_width_add;
	serif_scale = genchange->serif_width_scale;
    } else {
	stem_scale = genchange->stem_height_scale;
	stem_add = genchange->stem_height_add;
	serif_scale = genchange->serif_height_scale;
    }
    stroke_add = expanded ? stem_add : 0;

    for (i=0; i<stem->dep_cnt; i++) {
	slave = stem->dependent[i].stem;
	lbase = stem->dependent[i].lbase;
	if ( genchange->stem_threshold > 0 ) {
	    stem_scale = slave->width > genchange->stem_threshold ?
		genchange->stem_width_scale : genchange->stem_height_scale;
	    stem_add = genchange->stem_height_add;
	}
	stroke_add = expanded ? stem_add : 0;

	if ( !slave->ldone && !slave->rdone ) {
	    width_new = ( slave->width - stroke_add )*stem_scale + stem_add;
	    sl = (&slave->left.x)[!x_dir]; sr = (&slave->right.x)[!x_dir];
	    if ( !x_dir ) width_new = -width_new;
	    if (stem->dependent[i].dep_type == 'a' || stem->dependent[i].dep_type == 'm') {
		dist = lbase ? sl - l : sr - r;
		dist *= stem_scale;
		if (lbase) (&slave->newleft.x)[!x_dir] = l1 + floor( dist + .5 );
		else (&slave->newright.x)[!x_dir] = r1 + floor( dist + .5 );
	    } else if (stem->dependent[i].dep_type == 'i') {
		if (lbase)
		    (&slave->newleft.x)[!x_dir] = floor( InterpolateVal( l,r,l1,r1,sl ) + .5 );
		else
		    (&slave->newright.x)[!x_dir] = floor( InterpolateVal( l,r,l1,r1,sr ) + .5 );
	    }
	    if (lbase)
		(&slave->newright.x)[!x_dir] = (&slave->newleft.x)[!x_dir] + floor( width_new + .5 );
	    else
		(&slave->newleft.x)[!x_dir] = (&slave->newright.x)[!x_dir] - floor( width_new + .5 );
	}
	if ( slave->dep_cnt > 0 )
	    StemPosDependent( slave,genchange,x_dir );
    }

    if ( genchange->serif_control ) {
	for (i=0; i<stem->serif_cnt; i++) {
	    slave = stem->serifs[i].stem;
	    lbase = stem->serifs[i].lbase;
	    /* In the autoinstructor we usually link an edge of the serif stem to the opposite
	     * edge of the main stem. So, if 'lbase' is true, this actually means that we are
	     * interested in the right edge of the serif stem. However, here, despite
	     * the variable name, we position the left edge of the serif relatively to the
	     * left edge of the master stem and vice versa
	     */
	    dist = lbase ? (&slave->right.x)[!x_dir] - r : l - (&slave->left.x)[!x_dir];
	    dist *= serif_scale;
	    if (lbase) (&slave->newright.x)[!x_dir] = r1 + floor( dist + .5 );
	    else (&slave->newleft.x)[!x_dir] = l1 - floor( dist + .5 );
	}
    }
}

static void StemResize( SplineSet *ss,GlyphData *gd, StemData **dstems, int dcnt,
    DBounds *orig_b, DBounds *new_b, struct genericchange *genchange, int x_dir ) {

    double stem_scale, cntr_scale, stem_add, cntr_add, stroke_add, cntr_new, width_new;
    double min_coord = x_dir ? orig_b->minx : orig_b->miny;
    real *min_new = x_dir ? &new_b->minx : &new_b->miny;
    real *max_new = x_dir ? &new_b->maxx : &new_b->maxy;
    real *end, *newstart, *newend, *prevend=NULL, *newprevend=NULL;
    StemData *stem, *prev=NULL;
    StemBundle *bundle = x_dir ? gd->vbundle : gd->hbundle;
    int i, expanded;

    /* If additional amounts of em units for stem width/height have been
     * specified, but it was impossible to handle them via BoldSSStroke()
     * (e. g. because one of two values was zero) then just add them to the
     * scaled stem width. Otherwise stems have already been expanded, so
     * there is nothing to add
     */
    expanded = (genchange->stem_width_add != 0 && genchange->stem_height_add !=0 &&
		genchange->stem_height_add/genchange->stem_width_add > 0 );
    if ( x_dir ) {
	stem_scale = genchange->stem_width_scale;
	stem_add = genchange->stem_width_add;
	cntr_scale = genchange->hcounter_scale;
	cntr_add = genchange->hcounter_add;
    } else {
	stem_scale = genchange->stem_height_scale;
	stem_add = genchange->stem_height_add;
	cntr_scale = genchange->vcounter_scale;
	cntr_add = genchange->vcounter_add;
    }

    *min_new = floor( min_coord * cntr_scale + cntr_add + .5 );
    for ( i=0; i<bundle->cnt; i++ ) {
	stem = bundle->stemlist[i];
	if ( genchange->stem_threshold > 0 ) {
	    stem_scale = stem->width > genchange->stem_threshold ?
		genchange->stem_width_scale : genchange->stem_height_scale;
	    stem_add = genchange->stem_height_add;
	}
	stroke_add = expanded ? stem_add : 0;

	if ( stem->master == NULL ) {
	    newstart = x_dir ? &stem->newleft.x : &stem->newright.x;
	    newend = x_dir ? &stem->newright.x : &stem->newleft.x;

	    cntr_new = ScaleCounter( gd,dstems,dcnt,orig_b,prev,stem,genchange,x_dir );
	    if ( prev == NULL )
		newstart[!x_dir] = *min_new + floor( cntr_new + cntr_add + .5 );
	    else
		newstart[!x_dir] = newprevend[!x_dir] + floor( cntr_new + cntr_add + .5 );

	    /* Bounding box hints usually don't mark real stems, so there is */
	    /* no reason to maintain the normal stem width ratio when dealing with them */
	    if ( stem->bbox )
		width_new = ScaleCounter( gd,dstems,dcnt,orig_b,NULL,NULL,genchange,x_dir );
	    else
		width_new = ( stem->width - stroke_add )*stem_scale + stem_add;
	    newend[!x_dir] = newstart[!x_dir] + floor( width_new + .5 );
	    stem->ldone = stem->rdone = true;
	    StemPosDependent( stem,genchange,x_dir );
	    prev = stem;
	    newprevend = newend;
	}
    }

    *max_new = *min_new;
    prev = NULL;
    /* A new pass to calculate the glyph's right boundary. Here we */
    /* need to take into account also dependent stems (and that's why we */
    /* couldn't do that in the previous cycle) */
    for ( i=0; i<bundle->cnt; i++ ) {
	stem = bundle->stemlist[i];
	if ( stem->bbox )
    continue;
	end = x_dir ? &stem->right.x : &stem->left.y;
	newend = x_dir ? &stem->newright.x : &stem->newleft.y;
	if ( prev == NULL || end[!x_dir] > prevend[!x_dir] ) {
	    *max_new = floor( newend[!x_dir] + .5 );
	    prevend = end;
	    prev = stem;
	}
    }
    cntr_new = ScaleCounter( gd,dstems,dcnt,orig_b,prev,NULL,genchange,x_dir );
    *max_new += floor( cntr_new + cntr_add + .5 );
}

static void HStemResize( SplineSet *ss,GlyphData *gd,
    DBounds *orig_b, DBounds *new_b, struct genericchange *genchange ) {

    double middle, scale, stem_scale, stem_add, stroke_add, width_new;
    double top, bot, lpos, rpos, fuzz = gd->fuzz;
    StemData *stem, *test, *upper, *lower;
    int i, j, fcnt, expanded;
    struct fixed_maps *fix = &genchange->m;
    struct position_maps *pm;

    expanded = (genchange->stem_width_add != 0 && genchange->stem_height_add !=0 &&
		genchange->stem_height_add/genchange->stem_width_add > 0 );
    stem_add = genchange->stem_height_add;
    stroke_add = expanded ? stem_add : 0;
    scale = genchange->v_scale;

    new_b->miny = orig_b->miny * scale;
    new_b->maxy = orig_b->maxy * scale;

    /* Extend the scaled glyph bounding box if necessary to ensure all mapped values are inside */
    if ( fix->cnt > 0 ) {
	if ( fix->maps[0].cur_width < 0 && new_b->miny > fix->maps[0].desired + fix->maps[0].des_width )
	    new_b->miny = fix->maps[0].desired + fix->maps[0].des_width -
		genchange->v_scale * ( fix->maps[0].current + fix->maps[0].cur_width - orig_b->miny );
	else if ( fix->maps[0].cur_width > 0 && new_b->miny > fix->maps[0].desired )
	    new_b->miny = fix->maps[0].desired -
		genchange->v_scale * ( fix->maps[0].current - orig_b->miny );

	fcnt = fix->cnt - 1;
	if ( fix->maps[fcnt].cur_width > 0 && new_b->maxy < fix->maps[fcnt].desired + fix->maps[fcnt].des_width )
	    new_b->maxy = fix->maps[fcnt].desired + fix->maps[fcnt].des_width +
		genchange->v_scale * ( orig_b->maxy - ( fix->maps[fcnt].current + fix->maps[fcnt].cur_width ));
	else if ( fix->maps[fcnt].cur_width < 0 && new_b->maxy < fix->maps[fcnt].desired )
	    new_b->maxy = fix->maps[fcnt].desired +
		genchange->v_scale * ( orig_b->maxy - fix->maps[fcnt].current );
    }

    for ( i=0; i<gd->hbundle->cnt; i++ ) {
	stem = gd->hbundle->stemlist[i];
	if ( genchange->stem_threshold > 0 )
	    stem_scale = stem->width > genchange->stem_threshold ?
		genchange->stem_width_scale : genchange->stem_height_scale;
	else
	    stem_scale = genchange->stem_height_scale;

	/* The 'blue' field now actually points to a 'position_maps' structure. */
	/* So the name is wrong, but who cares... */
	if (stem->blue != -1 ) {
	    pm = &fix->maps[stem->blue];
	    width_new = ( stem->width - stroke_add )*stem_scale + stem_add;
	    lpos = stem->left.y - stroke_add/2;
	    rpos = stem->right.y + stroke_add/2;
	    if ( pm->cur_width < 0 && ( !stem->ghost || stem->width == 21 ) &&
		rpos >= pm->current + pm->cur_width - fuzz && rpos <= pm->current + fuzz ) {

		top = pm->current; bot = pm->current + pm->cur_width;
		if ( rpos >= bot && rpos <= top )
		    stem->newright.y = floor( InterpolateVal( bot,top,
			pm->desired + pm->des_width,pm->desired,rpos ) + .5 );
		else if ( rpos < bot )
		    stem->newright.y = floor( pm->desired + pm->des_width -
			( bot - rpos ) * stem_scale + .5 );
		else if ( rpos > top )
		    stem->newright.y = floor( pm->desired +
			( rpos - top ) * stem_scale + .5 );

		if ( !stem->ghost )
		    stem->newleft.y = stem->newright.y + floor( width_new + .5 );
		else
		    stem->newleft.y = stem->newright.y + 21;
		stem->ldone = stem->rdone = true;
	    } else if ( pm->cur_width > 0 && ( !stem->ghost || stem->width == 20 ) &&
		lpos >= pm->current - fuzz && lpos <= pm->current + pm->cur_width + fuzz  ) {

		top = pm->current + pm->cur_width; bot = pm->current;
		if ( lpos >= bot && lpos <= top )
		    stem->newleft.y = floor( InterpolateVal( bot,top,
			pm->desired,pm->desired + pm->des_width,lpos ) + .5 );
		else if ( lpos < bot )
		    stem->newleft.y = floor( pm->desired -
			( bot - lpos ) * stem_scale + .5 );
		else if ( lpos > top )
		    stem->newleft.y = floor( pm->desired + pm->des_width +
			( lpos - top ) * stem_scale + .5 );

		if ( !stem->ghost )
		    stem->newright.y = stem->newleft.y - floor( width_new + .5 );
		else
		    stem->newright.y = stem->newleft.y - 20;
		stem->ldone = stem->rdone = true;
	    } else
		stem->blue = -1;
	}
	if ( stem->bbox && stem->blue != -1 ) {
	    new_b->miny = stem->newright.y;
	    new_b->maxy = stem->newleft.y;
	}
    }

    for ( i=0; i<gd->hbundle->cnt; i++ ) {
	stem = gd->hbundle->stemlist[i];
	if ( genchange->stem_threshold > 0 )
	    stem_scale = stem->width > genchange->stem_threshold ?
		genchange->stem_width_scale : genchange->stem_height_scale;
	else
	    stem_scale = genchange->stem_height_scale;

	if (stem->blue == -1 && stem->master == NULL) {
	    middle = stem->right.y + stem->width/2;
	    upper = lower = NULL;
	    width_new = ( stem->width - stroke_add )*stem_scale + stem_add;
	    for ( j=0; j<gd->hbundle->cnt; j++ ) {
		test = gd->hbundle->stemlist[j];
		if ( test != stem && test->blue != -1 ) {
		    if ( test->right.y > stem->left.y &&
			( upper == NULL || test->right.y < upper->right.y ))
			upper = test;
		    else if ( test->left.y < stem->right.y &&
			( lower == NULL || test->left.y > lower->left.y ))
			lower = test;
		}
	    }
	    if ( upper != NULL && lower != NULL ) {
		middle = InterpolateVal( lower->left.y,upper->right.y,
		    lower->newleft.y,upper->newright.y,middle );
		stem->newleft.y = floor( middle + ( width_new/2 + .5 ));
		stem->newright.y = floor( middle - ( width_new/2 + .5 ));
	    } else if ( upper != NULL ) {
		stem->newright.y = floor( InterpolateVal( orig_b->miny,upper->right.y,
		    new_b->miny,upper->newright.y,stem->right.y ) + .5 );
		stem->newleft.y = stem->newright.y + floor( width_new + .5 );
	    } else if ( lower != NULL ) {
		stem->newleft.y = floor( InterpolateVal( lower->left.y,orig_b->maxy,
		    lower->newleft.y,new_b->maxy,stem->left.y ) + .5 );
		stem->newright.y = stem->newleft.y - floor( width_new + .5 );
	    } else {
		middle = InterpolateVal( orig_b->miny,orig_b->maxy,
		    new_b->miny,new_b->maxy,middle );
		stem->newleft.y = floor( middle + ( width_new/2 + .5 ));
		stem->newright.y = floor( middle - ( width_new/2 + .5 ));
	    }
	}
    }

    for ( i=0; i<gd->hbundle->cnt; i++ ) {
	stem = gd->hbundle->stemlist[i];
	if ( stem->master == NULL )
	    StemPosDependent( stem,genchange,false );
    }
    /* Final bounding box adjustment (it is possible that some of the  */
    /* calculated stem boundaries are outside of the new bounding box, */
    /* and we should fix this now */
    upper = lower = NULL;
    for ( i=0; i<gd->hbundle->cnt; i++ ) {
	stem = gd->hbundle->stemlist[i];
	if ( upper == NULL || stem->newleft.y > upper->newleft.y )
	    upper = stem;
	if ( lower == NULL || stem->newright.y < lower->newright.y )
	    lower = stem;
    }
    if ( upper != NULL )
	new_b->maxy = upper->newleft.y + ( orig_b->maxy - upper->left.y )*scale;
    if ( lower != NULL )
	new_b->miny = lower->newright.y - ( lower->right.y - orig_b->miny )*scale;
}

static void InitZoneMappings( struct fixed_maps *fix, BlueData *bd, double stem_scale ) {
    int i, j;
    double s_cur, e_cur, s_des, e_des;

    /* Sort mappings to ensure they go in the ascending order */
    qsort( fix->maps,fix->cnt,sizeof( struct position_maps ),fixed_cmp );

    /* Remove overlapping mappings */
    for ( i=j=0; i<fix->cnt; i++ ) {
	if ( i==j)
    continue;
	s_cur = ( fix->maps[i].cur_width > 0 ) ?
	    fix->maps[i].current : fix->maps[i].current + fix->maps[i].cur_width;
	e_cur = ( fix->maps[j].cur_width > 0 ) ?
	    fix->maps[j].current + fix->maps[j].cur_width : fix->maps[j].current;
	s_des = ( fix->maps[i].des_width > 0 ) ?
	    fix->maps[i].desired : fix->maps[i].desired + fix->maps[i].des_width;
	e_des = ( fix->maps[j].des_width > 0 ) ?
	    fix->maps[j].desired + fix->maps[j].des_width : fix->maps[j].desired;
	if ( s_cur > e_cur && s_des > e_des ) {
	    j++;
	    if ( j<i )
		memcpy( &fix->maps[j],&fix->maps[i],sizeof( struct position_maps ));
	}
    }
    if ( i>0 ) fix->cnt = j+1;

    /* Init a set of blues from user-specified mappings (we need this for */
    /* GlyphDataBuild(), to get stems associated with blues) */
    for ( i=0; i<fix->cnt; i++ ) {
	fix->maps[i].des_width = floor( fix->maps[i].cur_width * stem_scale + .5 );
	if ( i < 12 ) {
	    bd->blues[i][0] = fix->maps[i].cur_width < 0 ?
		fix->maps[i].current + fix->maps[i].cur_width : fix->maps[i].current;
	    bd->blues[i][1] = fix->maps[i].cur_width < 0 ?
		fix->maps[i].current : fix->maps[i].current + fix->maps[i].cur_width;
	    bd->bluecnt++;
	}
    }
}

static void PosStemPoints( GlyphData *gd, struct genericchange *genchange, int has_dstems, int x_dir ) {

    int i, j, best_is_l;
    uint8 flag = x_dir ? tf_x : tf_y;
    PointData *pd;
    StemData *tstem, *best;
    struct stem_chunk *chunk;
    double dist, bdist, stem_scale;
    BasePoint *sbase;

    for ( i=0; i<gd->pcnt; i++ ) if ( gd->points[i].sp != NULL ) {
	pd = &gd->points[i];
	best = NULL; bdist = 0;
	for ( j=0; j<pd->prevcnt; j++ ) {
	    tstem = pd->prevstems[j];
	    if ( !tstem->toobig && tstem->unit.x == !x_dir && RealNear( tstem->unit.y,x_dir )) {
		sbase = pd->prev_is_l[j] ? &tstem->left : &tstem->right;
		dist = ( pd->base.x - sbase->x )*x_dir - ( pd->base.y - sbase->y )*!x_dir;
		if ( best == NULL || fabs( dist ) < fabs( bdist )) {
		    best = tstem;
		    bdist = dist;
		    best_is_l = pd->prev_is_l[j];
		}
	    }
	}
	for ( j=0; j<pd->nextcnt; j++ ) {
	    tstem = pd->nextstems[j];
	    if ( !tstem->toobig && tstem->unit.x == !x_dir && tstem->unit.y == x_dir ) {
		sbase = pd->next_is_l[j] ? &tstem->left : &tstem->right;
		dist = ( pd->base.x - sbase->x )*x_dir - ( pd->base.y - sbase->y )*!x_dir;
		if ( best == NULL || fabs( dist ) < fabs( bdist )) {
		    best = tstem;
		    bdist = dist;
		    best_is_l = pd->next_is_l[j];
		}
	    }
	}

	if ( best != NULL ) {
	    if ( has_dstems && ( pd->x_corner == 2 || pd->y_corner == 2 )) {
		for ( j=0; j<best->chunk_cnt; j++ ) {
		    chunk = &best->chunks[j];
		    if (( chunk->l == pd || chunk->r == pd ) &&
			( chunk->stemcheat == 2 || chunk->stemcheat == 3 ))
		break;
		}
		/* Don't attempt to position inner points at diagonal intersections: */
		/* our diagonal stem processor will handle them better */
		if ( j<best->chunk_cnt )
    continue;
	    }
	    if (genchange->stem_threshold > 0)
		stem_scale = best->width > genchange->stem_threshold ?
		    genchange->stem_width_scale : genchange->stem_height_scale;
	    else
		stem_scale = x_dir ? genchange->stem_width_scale : genchange->stem_height_scale;

	    if ( !x_dir ) bdist = -bdist;
	    (&pd->newpos.x)[!x_dir] = best_is_l ?
		(&best->newleft.x)[!x_dir] + bdist * stem_scale :
		(&best->newright.x)[!x_dir] + bdist * stem_scale;
	    pd->touched |= flag;
	    pd->posdir.x = !x_dir; pd->posdir.y = x_dir;
	}
    }
}

static double InterpolateBetweenEdges( GlyphData *gd, double coord, double min, double max,
    double min_new, double max_new, int x_dir ) {

    StemData *stem;
    StemBundle *bundle = x_dir ? gd->vbundle : gd->hbundle;
    double prev_pos, next_pos, prev_new, next_new, start, end, ret;
    int i;

    prev_pos = -1e4; next_pos = 1e4;
    for ( i=0; i<bundle->cnt; i++ ) {
	stem = bundle->stemlist[i];
	start = x_dir ? stem->left.x : stem->right.y;
	end = x_dir ? stem->right.x : stem->left.y;

	if ( start >= min && start <= max ) {
	    if ( start < coord && start > prev_pos ) {
		prev_pos = start;
		prev_new = x_dir ? stem->newleft.x : stem->newright.y;
	    }
	    if ( start > coord && start < next_pos ) {
		next_pos = start;
		next_new = x_dir ? stem->newleft.x : stem->newright.y;
	    }
	}
	if ( end >= min && end <= max ) {
	    if ( end > coord && end < next_pos ) {
		next_pos = end;
		next_new = x_dir ? stem->newright.x : stem->newleft.y;
	    }
	    if ( end < coord && end > prev_pos ) {
		prev_pos = end;
		prev_new = x_dir ? stem->newright.x : stem->newleft.y;
	    }
	}
    }
    if ( prev_pos > -1e4 && next_pos < 1e4 )
	ret = InterpolateVal( prev_pos,next_pos,prev_new,next_new,coord );
    else if ( prev_pos > -1e4 )
	ret = InterpolateVal( prev_pos,max,prev_new,max_new,coord );
    else if ( next_pos < 1e4 )
	ret = InterpolateVal( min,next_pos,min_new,next_new,coord );
    else
	ret = InterpolateVal( min,max,min_new,max_new,coord );
return( ret );
}

static void InterpolateStrong( GlyphData *gd, DBounds *orig_b, DBounds *new_b, int x_dir ) {
    int i;
    uint8 mask, flag = x_dir ? tf_x : tf_y;
    double coord, new;
    double min, max, min_new, max_new;
    PointData *pd;

    mask = flag | tf_d;
    min = x_dir ? orig_b->minx : orig_b->miny;
    max = x_dir ? orig_b->maxx : orig_b->maxy;
    min_new = x_dir ? new_b->minx : new_b->miny;
    max_new = x_dir ? new_b->maxx : new_b->maxy;
    for ( i=0; i<gd->pcnt; i++ ) if ( gd->points[i].sp != NULL ) {
	pd = &gd->points[i];

	if ( !(pd->touched & mask) && ( IsExtremum(pd->sp,!x_dir) || IsAnglePoint(pd->sp))) {
	    coord = (&pd->base.x)[!x_dir];
	    new = InterpolateBetweenEdges( gd,coord,min,max,min_new,max_new,x_dir );
	    (&pd->newpos.x)[!x_dir] = new;
	    pd->touched |= flag;
	    pd->posdir.x = !x_dir; pd->posdir.y = x_dir;
	}
    }
}

static void InterpolateWeak( GlyphData *gd, DBounds *orig_b, DBounds *new_b, double scale, int x_dir ) {
    PointData *pd, *tpd, *fpd;
    int i;
    uint8 mask, flag = x_dir ? tf_x : tf_y;
    double coord, new, min, max, min_new, max_new;

    mask = flag | tf_d;
    min = x_dir ? orig_b->minx : orig_b->miny;
    max = x_dir ? orig_b->maxx : orig_b->maxy;
    min_new = x_dir ? new_b->minx : new_b->miny;
    max_new = x_dir ? new_b->maxx : new_b->maxy;

    /* Position any points which line between points already positioned */
    for ( i=0; i<gd->pcnt; i++ ) {
	pd = &gd->points[i];
	if ( pd->sp != NULL && !(pd->touched & mask) ) {
	    coord = (&pd->base.x)[!x_dir];
	    if ( pd->sp->prev != NULL && pd->sp->next != NULL ) {
		fpd = &gd->points[pd->sp->prev->from->ptindex];
		while ( !(fpd->touched & mask) && fpd != pd && fpd->sp->prev != NULL )
		    fpd = &gd->points[fpd->sp->prev->from->ptindex];

		tpd = &gd->points[pd->sp->next->to->ptindex];
		while ( !(tpd->touched & mask) && tpd != pd && tpd->sp->next != NULL )
		    tpd = &gd->points[tpd->sp->next->to->ptindex];

		if (( fpd->touched & mask ) && ( tpd->touched & mask ) &&
		    (&fpd->base.x)[!x_dir] != (&tpd->base.x)[!x_dir] && (
		    ( (&fpd->base.x)[!x_dir] <= coord && coord <= (&tpd->base.x)[!x_dir] ) ||
		    ( (&tpd->base.x)[!x_dir] <= coord && coord <= (&fpd->base.x)[!x_dir] ))) {
		    new = InterpolateVal( (&fpd->base.x)[!x_dir],(&tpd->base.x)[!x_dir],
					    (&fpd->newpos.x)[!x_dir],(&tpd->newpos.x)[!x_dir],coord );
		    (&pd->newpos.x)[!x_dir] = new;
		    pd->touched |= flag;
		    pd->posdir.x = !x_dir; pd->posdir.y = x_dir;
		}
	    }
	}
    }
    /* Any points which aren't currently positioned, just interpolate them */
    /*  between the hint zones between which they lie */
    /* I don't think this can actually happen... but do it just in case */
    for ( i=0; i<gd->pcnt; i++ ) {
	pd = &gd->points[i];
	if ( pd->sp != NULL && !(pd->touched & mask) ) {
	    coord = (&pd->base.x)[!x_dir];
	    new = InterpolateBetweenEdges( gd,coord,min,max,min_new,max_new,x_dir );
	    (&pd->newpos.x)[!x_dir] = new;
	    pd->touched |= flag;
	    pd->posdir.x = !x_dir; pd->posdir.y = x_dir;
	}
    }

    /* Interpolate the control points. More complex in order2. We want to */
    /*  preserve interpolated points, but simplified as we only have one cp */
    for ( i=0; i<gd->pcnt; i++ ) if ( gd->points[i].sp != NULL ) {
	pd = &gd->points[i];
	if ( !pd->sp->noprevcp && !gd->order2 ) {
	    coord = (&pd->sp->prevcp.x)[!x_dir];
	    fpd = &gd->points[pd->sp->prev->from->ptindex];
	    if (coord == (&pd->base.x)[!x_dir] )
		(&pd->newprev.x)[!x_dir] = (&pd->newpos.x)[!x_dir];
	    else {
		if (( coord >= (&fpd->base.x)[!x_dir] && coord <= (&pd->base.x)[!x_dir] ) ||
		    ( coord <= (&fpd->base.x)[!x_dir] && coord >= (&pd->base.x)[!x_dir] ))

		    (&pd->newprev.x)[!x_dir] = InterpolateVal((&pd->base.x)[!x_dir],
			(&fpd->base.x)[!x_dir],(&pd->newpos.x)[!x_dir],(&fpd->newpos.x)[!x_dir],coord);
		else
		    (&pd->newprev.x)[!x_dir] = (&pd->newpos.x)[!x_dir] +
			( coord - (&pd->base.x)[!x_dir] )*scale;
	    }
	}
	if ( !pd->sp->nonextcp ) {
	    coord = (&pd->sp->nextcp.x)[!x_dir];
	    tpd = &gd->points[pd->sp->next->to->ptindex];
	    if ( coord == (&pd->base.x)[!x_dir] )
		(&pd->newnext.x)[!x_dir] = (&pd->newpos.x)[!x_dir];
	    else {
		if (( coord >= (&tpd->base.x)[!x_dir] && coord <= (&pd->base.x)[!x_dir] ) ||
		    ( coord <= (&tpd->base.x)[!x_dir] && coord >= (&pd->base.x)[!x_dir] ))

		    (&pd->newnext.x)[!x_dir] = InterpolateVal((&pd->base.x)[!x_dir],
			(&tpd->base.x)[!x_dir],(&pd->newpos.x)[!x_dir],(&tpd->newpos.x)[!x_dir],coord);
		else
		    (&pd->newnext.x)[!x_dir] = (&pd->newpos.x)[!x_dir] +
			( coord - (&pd->base.x)[!x_dir] )*scale;
	    }
	    if ( gd->order2 )
	       (&tpd->newprev.x)[!x_dir] = (&pd->newnext.x)[!x_dir];
	}
    }
    if ( gd->order2 ) {
	for ( i=0; i<gd->pcnt; i++ ) if ( gd->points[i].sp != NULL ) {
	    pd = &gd->points[i];
	    if ( pd->ttfindex == 0xFFFF ) {
		(&pd->newpos.x)[!x_dir] = ((&pd->newnext.x)[!x_dir] + (&pd->newprev.x)[!x_dir])/2;
	    }
	}
    }
}

static void InterpolateAnchorPoints( GlyphData *gd,AnchorPoint *aps,
    DBounds *orig_b, DBounds *new_b,double scale,int x_dir ) {

    AnchorPoint *ap;
    double coord, new, min, max, min_new, max_new;

    min = x_dir ? orig_b->minx : orig_b->miny;
    max = x_dir ? orig_b->maxx : orig_b->maxy;
    min_new = x_dir ? new_b->minx : new_b->miny;
    max_new = x_dir ? new_b->maxx : new_b->maxy;

    for ( ap=aps; ap!=NULL; ap=ap->next ) {
	coord = (&ap->me.x)[!x_dir];
	/* Anchor points might be outside the bounding box */
	if ( coord >= min && coord <= max )
	    new = InterpolateBetweenEdges( gd,coord,min,max,min_new,max_new,x_dir );
	else if ( coord < min )
	    new = min_new - ( min - coord ) * scale;
	else
	    new = max_new + ( coord - max ) * scale;

	(&ap->me.x)[!x_dir] = new;
    }
}

static int PrepareDStemList( GlyphData *gd, StemData **dstems ) {
    double lpos, rpos, prevlsp, prevrsp, prevlep, prevrep;
    PointData *ls, *rs, *le, *re;
    struct stem_chunk *chunk;
    StemData *stem;
    int i, j, dcnt=0;

    for ( i=0; i<gd->stemcnt; i++ ) {
	stem = &gd->stems[i];
	if ( stem->toobig ||
	    ( stem->unit.y > -.05 && stem->unit.y < .05 ) ||
	    ( stem->unit.x > -.05 && stem->unit.x < .05 ) ||
	    stem->activecnt == 0 || stem->lpcnt == 0 || stem->rpcnt == 0 )
    continue;

	prevlsp = prevrsp = 1e4;
	prevlep = prevrep = -1e4;
	ls = rs = le = re = NULL;
	for ( j=0; j<stem->chunk_cnt; j++ ) {
	    chunk = &stem->chunks[j];
	    if ( chunk->l != NULL ) {
		lpos =  ( chunk->l->base.x - stem->left.x )*stem->unit.x +
			( chunk->l->base.y - stem->left.y )*stem->unit.y;
		if ( lpos < prevlsp ) {
		    ls = chunk->l; prevlsp = lpos;
		}
		if ( lpos > prevlep ) {
		    le = chunk->l; prevlep = lpos;
		}
	    }
	    if ( chunk->r != NULL ) {
		rpos =  ( chunk->r->base.x - stem->right.x )*stem->unit.x +
			( chunk->r->base.y - stem->right.y )*stem->unit.y;
		if ( rpos < prevrsp ) {
		    rs = chunk->r; prevrsp = rpos;
		}
		if ( rpos > prevrep ) {
		    re = chunk->r; prevrep = rpos;
		}
	   }
	}
	if ( ls == NULL || rs == NULL || le == NULL || re == NULL )
    continue;
	stem->keypts[0] = ls; stem->keypts[1] = le;
	stem->keypts[2] = rs; stem->keypts[3] = re;
	dstems[dcnt++] = stem;
    }
    qsort( dstems,dcnt,sizeof( StemData *),ds_cmp );
return( dcnt );
}

static void MovePointToDiag( PointData *pd, StemData *stem, int is_l ) {
    BasePoint fv, *base, *ptpos;
    double d, da;

    base = is_l ? &stem->newleft : &stem->newright;
    if ( !pd->touched ) {
	ptpos = &pd->base;
	if ( fabs( stem->unit.x ) > fabs( stem->unit.y )) {
	    fv.x = 0; fv.y = 1;
	} else {
	    fv.x = 1; fv.y = 0;
	}
    } else {
	fv.x = pd->posdir.x; fv.y = pd->posdir.y;
	ptpos = &pd->newpos;
    }

    d = stem->newunit.x * fv.y - stem->newunit.y * fv.x;
    da = ( base->x - ptpos->x )*fv.y - ( base->y - ptpos->y )*fv.x;

    if ( fabs( d ) > 0 ) {
	pd->newpos.x = base->x - ( da/d )*stem->newunit.x;
	pd->newpos.y = base->y - ( da/d )*stem->newunit.y;
	if ( pd->touched & tf_d && ( pd->posdir.x != stem->newunit.x || pd->posdir.y != stem->newunit.y )) {
	    pd->touched |= tf_x; pd->touched |= tf_y;
	} else
	    pd->touched |= tf_d;
	pd->posdir.x = stem->newunit.x; pd->posdir.y = stem->newunit.y;
    }
}

static void GetDStemBounds( GlyphData *gd, StemData *stem, real *prev, real *next, int x_dir ) {
    int i, maxact;
    double roff, dstart, dend, hvstart, hvend, temp;
    StemBundle *bundle;
    StemData *hvstem;

    roff =  ( stem->right.x - stem->left.x ) * stem->unit.x +
	    ( stem->right.y - stem->left.y ) * stem->unit.y;
    maxact = stem->activecnt - 1;

    if ( stem->unit.y > 0 ) {
	dstart = x_dir ?
	    stem->right.x + ( stem->active[0].start - roff ) * stem->unit.x :
	    stem->left.y + stem->active[0].start * stem->unit.y;
	dend = x_dir ?
	    stem->left.x + stem->active[maxact].end * stem->unit.x :
	    stem->right.y + ( stem->active[maxact].end - roff ) * stem->unit.y;
    } else {
	dstart = x_dir ?
	    stem->left.x + stem->active[0].start * stem->unit.x :
	    stem->right.y + ( stem->active[0].start - roff ) * stem->unit.y ;
	dend = x_dir ?
	    stem->right.x + ( stem->active[maxact].end - roff ) * stem->unit.x :
	    stem->left.y + stem->active[maxact].end * stem->unit.y;
    }
    if ( dstart > dend ) {
	temp = dstart; dstart = dend; dend = temp;
    }

    bundle = x_dir ? gd->vbundle : gd->hbundle;
    for ( i=0; i<bundle->cnt; i++ ) {
	hvstem = bundle->stemlist[i];
	hvstart = x_dir ? hvstem->left.x : hvstem->right.y;
	hvend = x_dir ? hvstem->right.x : hvstem->left.y;
	if ( hvend > *prev && hvend <= dstart )
	    *prev = hvend;
	else if ( hvstart < *next && hvstart >= dend )
	    *next = hvstart;
    }
}

static void AlignPointPair( StemData *stem,PointData *lpd, PointData *rpd, double hscale,double vscale ) {
    double off, newoff, dscale;

    /* If points are already horizontally or vertically aligned, */
    /* then there is nothing more to do here */
    if (( lpd->base.x == rpd->base.x && lpd->newpos.x == rpd->newpos.x ) ||
	( lpd->base.y == rpd->base.y && lpd->newpos.y == rpd->newpos.y ))
return;

    dscale =  sqrt( pow( hscale * stem->unit.x,2 ) +
		    pow( vscale * stem->unit.y,2 ));
    if ( !IsPointFixed( rpd )) {
	 off    =( rpd->base.x - lpd->base.x ) * stem->unit.x +
		 ( rpd->base.y - lpd->base.y ) * stem->unit.y;
	 newoff =( rpd->newpos.x - lpd->newpos.x ) * stem->newunit.x +
		 ( rpd->newpos.y - lpd->newpos.y ) * stem->newunit.y;
	 rpd->newpos.x += ( off*dscale - newoff )*stem->newunit.x;
	 rpd->newpos.y += ( off*dscale - newoff )*stem->newunit.y;
     } else if ( !IsPointFixed( lpd )) {
	 off    =( lpd->base.x - rpd->base.x ) * stem->unit.x +
		 ( lpd->base.y - rpd->base.y ) * stem->unit.y;
	 newoff =( lpd->newpos.x - rpd->newpos.x ) * stem->newunit.x +
		 ( lpd->newpos.y - rpd->newpos.y ) * stem->newunit.y;
	 lpd->newpos.x += ( off*dscale - newoff )*stem->newunit.x;
	 lpd->newpos.y += ( off*dscale - newoff )*stem->newunit.y;
     }
}

static int CorrectDPointPos( GlyphData *gd, PointData *pd, StemData *stem,
    double scale, int next, int is_l, int x_dir ) {

    int i, found = 0;
    uint8 flag = x_dir ? tf_x : tf_y;
    double ndot, pdot, coord_orig, coord_new, base_orig, base_new;
    Spline *ns;
    StemData *tstem;
    PointData *npd;

    if ( IsPointFixed( pd ))
return( false );
    ns = next ? pd->sp->next : pd->sp->prev;
    if ( ns == NULL )
return( false );
    npd = next ? &gd->points[ns->to->ptindex] : &gd->points[ns->from->ptindex];
    if ( IsStemAssignedToPoint( npd,stem,!next ) != -1 )
return( false );
    ndot = pd->nextunit.x * npd->nextunit.x + pd->nextunit.y * npd->nextunit.y;
    pdot = pd->prevunit.x * npd->prevunit.x + pd->prevunit.y * npd->prevunit.y;

    while ( npd != pd && ( ndot > 0 || pdot > 0 )) {
	if ( npd->touched & flag ) {
	    for ( i=0; i<npd->prevcnt && !found; i++ ) {
		tstem = npd->prevstems[i];
		if ( !tstem->toobig && tstem->unit.x == !x_dir && tstem->unit.y == x_dir )
		    found = true;
	    }
	    for ( i=0; i<npd->nextcnt && !found; i++ ) {
		tstem = npd->nextstems[i];
		if ( !tstem->toobig && tstem->unit.x == !x_dir && tstem->unit.y == x_dir )
		    found = true;
	    }
	}
	if ( found )
    break;
	ns = next ? npd->sp->next : npd->sp->prev;
	if ( ns == NULL )
    break;
	npd = next ? &gd->points[ns->to->ptindex] : &gd->points[ns->from->ptindex];
	ndot = pd->nextunit.x * npd->nextunit.x + pd->nextunit.y * npd->nextunit.y;
	pdot = pd->prevunit.x * npd->prevunit.x + pd->prevunit.y * npd->prevunit.y;
    }
    if ( !found )
return( false );

    coord_orig = (&pd->base.x)[!x_dir];
    coord_new = (&pd->newpos.x)[!x_dir];
    base_orig = (&npd->base.x)[!x_dir];
    base_new = (&npd->newpos.x)[!x_dir];
    if (( coord_orig > base_orig && coord_new <= base_new ) ||
	( coord_orig < base_orig && coord_new >= base_new )) {
	coord_new = base_new + ( coord_orig - base_orig )*scale;
	if ( x_dir ) {
	    pd->newpos.y += stem->newunit.y * (( coord_new - pd->newpos.x  )/stem->newunit.x );
	    pd->newpos.x = coord_new;
	} else {
	    pd->newpos.x += stem->newunit.x * (( coord_new - pd->newpos.y )/stem->newunit.y );
	    pd->newpos.y = coord_new;
	}
return( true );
    }
return( false );
}

static void ShiftDependent( GlyphData *gd, PointData *pd, StemData *stem,
    DBounds *orig_b, DBounds *new_b, double cscale, int next, int is_l, int x_dir ) {

    uint8 flag = x_dir ? tf_x : tf_y;
    int i, scnt, from_min;
    double ndot, pdot, off, dist;
    Spline *ns;
    PointData *npd;
    StemData *tstem, *dstem = NULL;

    /* Check if the given side of the point is controlled by another */
    /* DStem. If so, then we should check against that stem instead  */
    scnt = next ? pd->nextcnt : pd->prevcnt;
    for ( i=0; i<scnt; i++ ) {
	tstem = next ? pd->nextstems[i] : pd->prevstems[i];
	if ( tstem != stem && !tstem->toobig &&
	    ( tstem->unit.x < -.05 || tstem->unit.x > .05 ) &&
	    ( tstem->unit.y < -.05 || tstem->unit.y > .05 ))
return;
    }

    /* Is this edge closer to the initial or the final coordinate of the bounding box? */
    from_min = (( x_dir && is_l && stem->unit.y > 0 ) ||
		( x_dir && !is_l && stem->unit.y < 0 ) ||
		( !x_dir && !is_l ));
    ns = next ? pd->sp->next : pd->sp->prev;
    if ( ns == NULL )
return;
    npd = next ? &gd->points[ns->to->ptindex] : &gd->points[ns->from->ptindex];
    if ( IsStemAssignedToPoint( npd,stem,!next ) != -1 )
return;
    ndot = pd->nextunit.x * npd->nextunit.x + pd->nextunit.y * npd->nextunit.y;
    pdot = pd->prevunit.x * npd->prevunit.x + pd->prevunit.y * npd->prevunit.y;
    off =   ( npd->base.x - pd->base.x )*stem->unit.y -
	    ( npd->base.y - pd->base.y )*stem->unit.x;
    dist =  (&npd->base.x)[!x_dir] - (&pd->base.x)[!x_dir];

    while ( npd != pd && ( ndot > 0 || pdot > 0 ) && !( npd->touched & flag ) && (
	( is_l && off <= 0 ) || ( !is_l && off >= 0 ) ||
	( from_min && dist <= 0 ) || ( !from_min && dist >= 0 ))) {

	/* Can't just check if the point is touched in a diagonal direction,
	 * since not all DStems may have been done at the moment. So see if there
	 * are any valid DStems attached to the point. If so, then it is not
	 * a candidate for the strong interpolation, and thus there is no need
	 * to adjust its position here
	 */
	for ( i=0; i<npd->prevcnt && dstem == NULL; i++ ) {
	    tstem = npd->prevstems[i];
	    if ( !tstem->toobig &&
		( tstem->unit.x < -.05 || tstem->unit.x > .05 ) &&
		( tstem->unit.y < -.05 || tstem->unit.y > .05 ))
		dstem = tstem;
	}
	for ( i=0; i<npd->nextcnt && dstem == NULL; i++ ) {
	    tstem = npd->nextstems[i];
	    if ( !tstem->toobig &&
		( tstem->unit.x < -.05 || tstem->unit.x > .05 ) &&
		( tstem->unit.y < -.05 || tstem->unit.y > .05 ))
		dstem = tstem;
	}
	if ( dstem != NULL )
    break;
	if ( IsExtremum( npd->sp,!x_dir ) || IsAnglePoint( npd->sp )) {
	    (&npd->newpos.x)[!x_dir] = (&pd->newpos.x)[!x_dir] +
		((&npd->base.x)[!x_dir] - (&pd->base.x)[!x_dir]) * cscale;

	    npd->touched |= flag;
	    npd->posdir.x = !x_dir; npd->posdir.y = x_dir;
	}

	ns = next ? npd->sp->next : npd->sp->prev;
	if ( ns == NULL )
    break;
	npd = next ? &gd->points[ns->to->ptindex] : &gd->points[ns->from->ptindex];
	ndot = pd->nextunit.x * npd->nextunit.x + pd->nextunit.y * npd->nextunit.y;
	pdot = pd->prevunit.x * npd->prevunit.x + pd->prevunit.y * npd->prevunit.y;
	off =   ( npd->base.x - pd->base.x )*stem->unit.y -
		( npd->base.y - pd->base.y )*stem->unit.x;
	dist =  (&npd->base.x)[!x_dir] - (&pd->base.x)[!x_dir];
    }
}

/* If several DStems share one common edge, then use the longer one to
 * position point on that edge and ignore others. Otherwise we can go
 * far outside of our bounding box, attempting to find an intersection
 * of two almost parallel (but still different) vectors
 */
static int PreferredDStem( PointData *pd, StemData *stem, int next ) {
    int i, stemcnt = next ? pd->nextcnt : pd->prevcnt;
    StemData *tstem;

    for ( i=0; i<stemcnt; i++ ) {
	tstem = next ? pd->nextstems[i] : pd->prevstems[i];
	if ( tstem != stem && !tstem->toobig &&
	    ( tstem->unit.y < -.05 || tstem->unit.y > .05 ) &&
	    ( tstem->unit.x < -.05 || tstem->unit.x > .05 ) && tstem->clen > stem->clen )
return( false );
    }
return( true );
}

static void FixDStem( GlyphData *gd, StemData *stem,  StemData **dstems, int dcnt,
    DBounds *orig_b, DBounds *new_b, struct genericchange *genchange ) {

    int i, is_l, pref_y, nextidx, previdx;
    PointData *pd, *lfixed=NULL, *rfixed=NULL;
    double new_hyp, new_w, des_w, hscale, vscale, hscale1, vscale1, cscale;
    double coord_new, min, max, min_new, max_new;
    BasePoint l_to_r, left, right, temp;
    DBounds stem_b;
    StemData *hvstem;

    /* Find the ratio by which the stem is going to be downsized in both directions.
     * To do this, we assume each DStem crosses a counter and calculate the ratio by
     * comparing the original and resulting size of that counter. For "A" or "V"
     * such a "counter" will most probably cover the overall glyph width, while for
     * "M" or "N" it will be limited to the space between two vertical stems.
     * For the vertical direction we could just use genchange->v_scale instead, but it
     * is not guaranteed to be relevant if some blues were mapped to positions,
     * different from their default (scaled) values
     */
    stem_b.minx = orig_b->minx; stem_b.maxx = orig_b->maxx;
    GetDStemBounds( gd,stem,&stem_b.minx,&stem_b.maxx,true );
    min = orig_b->minx; max = orig_b->maxx;
    min_new = new_b->minx; max_new = new_b->maxx;
    for ( i=0; i<gd->vbundle->cnt; i++ ) {
	hvstem = gd->vbundle->stemlist[i];
	if (( RealNear( hvstem->left.x,min ) && RealNear( hvstem->right.x,max )) ||
	    ( RealNear( hvstem->left.x,max ) && RealNear( hvstem->right.x,min )))
    continue;
	if ( hvstem->left.x >= stem_b.maxx && hvstem->left.x < max ) {
	    max = hvstem->left.x;
	    max_new = hvstem->newleft.x;
	} else if ( hvstem->right.x <= stem_b.minx && hvstem->right.x > min ) {
	    min = hvstem->right.x;
	    min_new = hvstem->newright.x;
	}
    }
    if ( max == min ) {
	stem->toobig = true;
return;
    }
    hscale = ( max_new - min_new )/( max - min );

    stem_b.miny = orig_b->miny; stem_b.maxy = orig_b->maxy;
    GetDStemBounds( gd,stem,&stem_b.miny,&stem_b.maxy,false );
    min = orig_b->miny; max = orig_b->maxy;
    min_new = new_b->miny; max_new = new_b->maxy;
    for ( i=0; i<gd->hbundle->cnt; i++ ) {
	hvstem = gd->hbundle->stemlist[i];
	if ( hvstem->left.y == min && hvstem->right.y == max )
    continue;
	if ( hvstem->right.y >= stem_b.maxy && hvstem->right.y < max ) {
	    max = hvstem->right.y;
	    max_new = hvstem->newright.y;
	} else if ( hvstem->left.y <= stem_b.miny && hvstem->left.y > min ) {
	    min = hvstem->left.y;
	    min_new = hvstem->newleft.y;
	}
    }
    if ( max == min ) {
	stem->toobig = true;
return;
    }
    vscale = ( max_new - min_new )/( max - min );

    /* Now scale positions of the left and right virtual points on the stem */
    /* by the ratios used to resize horizontal and vertical stems. We need  */
    /* this solely to calculate the desired stem width */
    if (genchange->stem_threshold > 0) {
	hscale1 = vscale1 = stem->width > genchange->stem_threshold ?
	    genchange->stem_width_scale : genchange->stem_height_scale;
    } else {
	hscale1 = genchange->stem_height_scale;
	vscale1 = genchange->stem_width_scale;
    }
    stem->newleft.x = stem->left.x * vscale1;
    stem->newleft.y = stem->left.y * hscale1;
    stem->newright.x = stem->right.x * vscale1;
    stem->newright.y = stem->right.y * hscale1;
    stem->newunit.x = stem->unit.x * vscale1;
    stem->newunit.y = stem->unit.y * hscale1;
    new_hyp = sqrt( pow( stem->newunit.x,2 ) + pow( stem->newunit.y,2 ));
    stem->newunit.x /= new_hyp;
    stem->newunit.y /= new_hyp;

    des_w = ( stem->newright.x - stem->newleft.x ) * stem->newunit.y -
	    ( stem->newright.y - stem->newleft.y ) * stem->newunit.x;

    /* For italic stems we move left and right coordinates to the baseline, so   */
    /* that they may well fall outside of the bounding box. We have to take care */
    /* of this situation */
    left = stem->left; right = stem->right;
    if ( left.x < orig_b->minx ) {
	left.y += (( orig_b->minx - left.x ) * stem->unit.y )/stem->unit.x;
	left.x = orig_b->minx;
    }
    if ( right.x < orig_b->minx ) {
	right.y += (( orig_b->minx - right.x ) * stem->unit.y )/stem->unit.x;
	right.x = orig_b->minx;
    }

    /* OK, now we know the desired width, so interpolate the coordinates of our
     * left and right points to determine where our DStem would probably be
     * placed if there were no DStem processor. We are not expecting to get an
     * exact result here, but it still would be a good starting point
     */
    stem->newleft.x = InterpolateBetweenEdges(
	gd,left.x,orig_b->minx,orig_b->maxx,new_b->minx,new_b->maxx,true );
    stem->newleft.y = InterpolateBetweenEdges(
	gd,left.y,orig_b->miny,orig_b->maxy,new_b->miny,new_b->maxy,false );
    stem->newright.x = InterpolateBetweenEdges(
	gd,right.x,orig_b->minx,orig_b->maxx,new_b->minx,new_b->maxx,true );
    stem->newright.y = InterpolateBetweenEdges(
	gd,right.y,orig_b->miny,orig_b->maxy,new_b->miny,new_b->maxy,false );

    /* Adjust the stem unit vector according to the horizontal and vertical */
    /* ratios we have previously determined */
    if ( stem->bundle != NULL && stem->bundle == gd->ibundle ) {
	stem->newunit.x = stem->unit.x;
	stem->newunit.y = stem->unit.y;
    } else {
	stem->newunit.x = stem->unit.x * hscale;
	stem->newunit.y = stem->unit.y * vscale;
	new_hyp = sqrt( pow( stem->newunit.x,2 ) + pow( stem->newunit.y,2 ));
	stem->newunit.x /= new_hyp;
	stem->newunit.y /= new_hyp;
    }

    /* Get a possible width of the stem in the scaled glyph. We are going to */
    /* adjust that width then */
    new_w = ( stem->newright.x - stem->newleft.x ) * stem->newunit.y -
	    ( stem->newright.y - stem->newleft.y ) * stem->newunit.x;
    if ( new_w < 0 ) {
	temp.x = stem->newleft.x; temp.y = stem->newleft.y;
	stem->newleft.x = stem->newright.x; stem->newleft.y = stem->newright.y;
	stem->newright.x = temp.x; stem->newright.y = temp.y;
	new_w = -new_w;
    }
    /* Guess at which normal we want */
    l_to_r.x = stem->newunit.y; l_to_r.y = -stem->newunit.x;
    /* If we guessed wrong, use the other */
    if (( stem->newright.x - stem->newleft.x )*l_to_r.x +
	( stem->newright.y - stem->newleft.y )*l_to_r.y < 0 ) {
	l_to_r.x = -l_to_r.x;
	l_to_r.y = -l_to_r.y;
    }

    /* Now look if there are any points on the edges of our stem which are already fixed */
    /* both by x and y and thus can no longer be moved */
    for ( i=0; i<gd->pcnt && ( lfixed == NULL || rfixed == NULL ); i++ ) if ( gd->points[i].sp != NULL ) {
	pd = &gd->points[i];
	nextidx = IsStemAssignedToPoint( pd,stem,true );
	previdx = IsStemAssignedToPoint( pd,stem,false );
	if (( nextidx != -1 || previdx != -1 ) && IsPointFixed( pd )) {
	    is_l = ( nextidx == -1 ) ? pd->prev_is_l[previdx] : pd->next_is_l[nextidx];
	    if ( is_l ) lfixed = pd;
	    else rfixed = pd;
	}
    }
    /* If there are such points at both edges, then we probably can do nothing useful */
    /* with this stem */
    if ( lfixed != NULL && rfixed != NULL ) {
	stem->toobig = true;
return;
    }
    /* If just one edge is fixed, then use it as a base and move another edge to */
    /* maintain the desired stem width */
    else if ( lfixed != NULL ) {
	stem->newleft.x = lfixed->newpos.x; stem->newleft.y = lfixed->newpos.y;
	stem->newright.x = stem->newleft.x + des_w * l_to_r.x;
	stem->newright.y = stem->newleft.y + des_w * l_to_r.y;
    } else if ( rfixed != NULL ) {
	stem->newright.x = rfixed->newpos.x; stem->newright.y = rfixed->newpos.y;
	stem->newleft.x = stem->newright.x - des_w * l_to_r.x;
	stem->newleft.y = stem->newright.y - des_w * l_to_r.y;
    /* Otherwise move both edges to an equal amount of space from their initial */
    /* positions */
    } else {
	stem->newleft.x = stem->newleft.x - ( des_w - new_w )/2 * l_to_r.x;
	stem->newleft.y = stem->newleft.y - ( des_w - new_w )/2 * l_to_r.y;
	stem->newright.x = stem->newright.x + ( des_w - new_w )/2 * l_to_r.x;
	stem->newright.y = stem->newright.y + ( des_w - new_w )/2 * l_to_r.y;
    }

    /* Determine the preferred direction for moving points which have not */
    /* already been touched */
    pref_y = fabs( stem->unit.y ) > fabs( stem->unit.x );
    min = pref_y ? orig_b->miny : orig_b->minx;
    max = pref_y ? orig_b->maxy : orig_b->maxx;
    min_new = pref_y ? new_b->miny : new_b->minx;
    max_new = pref_y ? new_b->maxy : new_b->maxx;

    /* Now proceed to positioning points */
    for ( i=0; i<gd->pcnt; i++ ) if ( gd->points[i].sp != NULL ) {
	pd = &gd->points[i];
	nextidx = IsStemAssignedToPoint( pd,stem,true );
	previdx = IsStemAssignedToPoint( pd,stem,false );
	if (( nextidx == -1 && previdx == -1 ) || IsPointFixed( pd ))
    continue;
	if (( nextidx != -1 && !PreferredDStem( pd,stem,true)) ||
	    ( previdx != -1 && !PreferredDStem( pd,stem,false)))
    continue;
	is_l = ( nextidx == -1 ) ? pd->prev_is_l[previdx] : pd->next_is_l[nextidx];
	/* Move the point to a diagonal line. This doesn't yes guarantees it will */
	/* be placed inside our bounding box */
	MovePointToDiag( pd,stem,is_l );

	if ( !IsPointFixed( pd )) {
	    /* Interpolate the point between either horizontal or vertical stem
	     * edges along the preferred direction (determined according to the stem
	     * unit vector). This will position points still floating outside the bounding
	     * box and also guarantees e. g. proper point positioning relatively to serifs
	     * if they have been expanded
	     */
	    coord_new = InterpolateBetweenEdges(
		gd,(&pd->base.x)[pref_y],min,max,min_new,max_new,!pref_y );
	    if ( pref_y ) {
		pd->newpos.x += stem->newunit.x * (( coord_new - pd->newpos.y )/stem->newunit.y );
		pd->newpos.y = coord_new;
	    } else {
		pd->newpos.y += stem->newunit.y * (( coord_new - pd->newpos.x  )/stem->newunit.x );
		pd->newpos.x = coord_new;
	    }

	 }
	/* Check if there are obvious displacements relatively to stems going in the
	 * other direction and correct them. For example, the top left point of the diagonal
	 * stem in a Latin "N" may be moved inside the nearby vertical stem, and we have
	 * to prevent this
	 */
	cscale = pref_y ? genchange->hcounter_scale :
	    genchange->use_vert_mapping ? genchange->v_scale : genchange->vcounter_scale;
	if ( !CorrectDPointPos( gd,pd,stem,pref_y?hscale:vscale,true,is_l,pref_y ))
	    ShiftDependent( gd,pd,stem,orig_b,new_b,cscale,true,is_l,pref_y );
	if ( !CorrectDPointPos( gd,pd,stem,pref_y?hscale:vscale,false,is_l,pref_y ))
	    ShiftDependent( gd,pd,stem,orig_b,new_b,cscale,false,is_l,pref_y );
	 cscale = pref_y ? genchange->use_vert_mapping ?
	     genchange->v_scale : genchange->vcounter_scale : genchange->hcounter_scale;
	 ShiftDependent( gd,pd,stem,orig_b,new_b,cscale,true,is_l,!pref_y );
	 ShiftDependent( gd,pd,stem,orig_b,new_b,cscale,false,is_l,!pref_y );
    }

    /* This is to fix relative positioning of starting/terminal points on a diagonal stem
     * which starts/finishes with a "stub" (i. e. no terminal serifs or connections with
     * other features), like in a slash or less/greater signs. We would expect such points
     * to be aligned as in the original outline, even if horizontal and vertical ratios
     * were different
     */
    if ( stem->keypts[0] != stem->keypts[2] && (
	( stem->keypts[0]->sp->next->to == stem->keypts[2]->sp ) ||
	( stem->keypts[0]->sp->prev->from == stem->keypts[2]->sp )))
	AlignPointPair( stem,stem->keypts[0],stem->keypts[2],hscale,vscale );

    if ( stem->keypts[1] != stem->keypts[3] && (
	( stem->keypts[1]->sp->next->to == stem->keypts[3]->sp ) ||
	( stem->keypts[1]->sp->prev->from == stem->keypts[3]->sp )))
	AlignPointPair( stem,stem->keypts[1],stem->keypts[3],hscale,vscale );
}

static void ChangeGlyph( SplineChar *sc_sc, SplineChar *orig_sc, int layer, struct genericchange *genchange ) {
    real scale[6];
    DBounds orig_b, new_b;
    int i, dcnt = 0, removeoverlap = true;
    double owidth = orig_sc->width, ratio, add;
    AnchorPoint *ap;
    GlyphData *gd;
    PointData *pd;
    StemData **dstems = NULL;
    StrokeInfo si;
    SplineSet *temp;
    BlueData bd;

    if ( sc_sc != orig_sc ) {
	sc_sc->layers[layer].splines = SplinePointListCopy( orig_sc->layers[layer].splines );
	sc_sc->anchor = AnchorPointsCopy(orig_sc->anchor);
    }
    memset( &bd,0,sizeof( bd ));
    InitZoneMappings( &genchange->m,&bd,genchange->v_scale );

    if (genchange->stem_height_add!=0 && genchange->stem_width_add!=0 &&
	genchange->stem_height_add/genchange->stem_width_add > 0 ) {

	ratio = genchange->stem_height_add/genchange->stem_width_add;
	memset( scale,0,sizeof( scale ));
	if ( ratio>1 ) {
	    add = genchange->stem_width_add;
	    scale[0] = 1.;
	    scale[3] = 1/ratio;
	} else {
	    add = genchange->stem_height_add;
	    scale[0] = ratio;
	    scale[3] = 1.;
	}
	SplinePointListTransform( sc_sc->layers[layer].splines,scale,tpt_AllPoints );

	memset( &si,0,sizeof( si ));
	si.stroke_type = si_std;
	si.join = lj_miter;
	si.cap = lc_square;
	if ( add >=0 ) {
	    si.radius = add/2.;
	    si.removeinternal = true;
	} else {
	    si.radius = -add/2.;
	    si.removeexternal = true;
	}
	/*si.removeoverlapifneeded = false;*/

	temp = BoldSSStroke( sc_sc->layers[layer].splines,&si,sc_sc->layers[layer].order2,removeoverlap );
	SplinePointListsFree( sc_sc->layers[layer].splines );
	sc_sc->layers[layer].splines = temp;
	if ( ratio != 1.0 ) {
	    if ( ratio>1 ) {
		scale[0] = 1.;
		scale[3] = ratio;
	    } else {
		scale[0] = 1/ratio;
		scale[3] = 1.;
	    }
	}
	SplinePointListTransform( sc_sc->layers[layer].splines,scale,tpt_AllPoints );

	/* If stroke has been expanded/condensed, then the old hints are no longer */
	/* relevant; so just remove them and rely solely on the stem detector */
	StemInfosFree(sc_sc->hstem);  sc_sc->hstem = NULL;
	StemInfosFree(sc_sc->vstem);  sc_sc->vstem = NULL;
	DStemInfosFree(sc_sc->dstem); sc_sc->dstem = NULL;

	/* Resize blues so that GlyphDataBuild() could snap the emboldened stems to them */
	for ( i=0; i<bd.bluecnt; i++ ) {
	    if ( bd.blues[i][0] < 0 ) {
		bd.blues[i][0] -= genchange->stem_height_add/2;
		bd.blues[i][1] -= genchange->stem_height_add/2;
	    } else if ( bd.blues[i][1] > 0 ) {
		bd.blues[i][1] += genchange->stem_height_add/2;
		bd.blues[i][0] += genchange->stem_height_add/2;
	    }
	}
    }

    SplineCharLayerFindBounds( orig_sc,layer,&orig_b );
    memcpy( &new_b,&orig_b,sizeof( DBounds ));
    gd = GlyphDataBuild( sc_sc,layer,&bd,true );
    if ( gd == NULL )
return;
    if (genchange->stem_height_add!=0 && genchange->stem_width_add!=0 &&
	genchange->stem_height_add/genchange->stem_width_add > 0 ) {
	/* Resize blues back to original values so that stems could be shifted to their original positions */
	for ( i=0; i<bd.bluecnt; i++ ) {
	    if ( bd.blues[i][0] < 0 ) {
		bd.blues[i][0] += genchange->stem_height_add/2;
		bd.blues[i][1] += genchange->stem_height_add/2;
	    } else if ( bd.blues[i][1] > 0 ) {
		bd.blues[i][1] -= genchange->stem_height_add/2;
		bd.blues[i][0] -= genchange->stem_height_add/2;
	    }
	}
    }

    /* Have to prepare a DStem list before further operations, since they are needed */
    /* to properly calculate counters between vertical stems */
    if ( genchange->dstem_control ) {
	dstems = calloc( gd->stemcnt,sizeof( StemData *));
	dcnt = PrepareDStemList( gd,dstems );
    }

    StemResize( sc_sc->layers[layer].splines,gd,dstems,dcnt,&orig_b,&new_b,genchange,true );
    if ( genchange->use_vert_mapping && genchange->m.cnt > 0 )
	HStemResize( sc_sc->layers[layer].splines,gd,&orig_b,&new_b,genchange );
    else
	StemResize( sc_sc->layers[layer].splines,gd,dstems,dcnt,&orig_b,&new_b,genchange,false );
    PosStemPoints( gd,genchange,dcnt > 0,true );
    PosStemPoints( gd,genchange,dcnt > 0,false );
    if ( genchange->dstem_control ) {
	for ( i=0; i<dcnt; i++ )
	    FixDStem( gd,dstems[i],dstems,dcnt,&orig_b,&new_b,genchange );
    }
    /* Our manipulations with DStems may have moved some points outside of  */
    /* borders of the bounding box we have previously calculated. So adjust */
    /* those borders now, as they may be important for point interpolations */
    for ( i=0; i<gd->pcnt; i++ ) if ( gd->points[i].sp != NULL ) {
	pd = &gd->points[i];
	if ( pd->touched & tf_d && pd->newpos.x < new_b.minx )
	    new_b.minx = pd->newpos.x;
	else if ( pd->touched & tf_d && pd->newpos.x > new_b.maxx )
	    new_b.maxx = pd->newpos.x;
	if ( pd->touched & tf_d && pd->newpos.y < new_b.miny )
	    new_b.miny = pd->newpos.y;
	else if ( pd->touched & tf_d && pd->newpos.y > new_b.maxy )
	    new_b.maxy = pd->newpos.y;
    }
    InterpolateStrong( gd,&orig_b,&new_b,true );
    InterpolateStrong( gd,&orig_b,&new_b,false );
    InterpolateWeak( gd,&orig_b,&new_b,genchange->stem_width_scale,true );
    InterpolateWeak( gd,&orig_b,&new_b,genchange->stem_height_scale,false );
    InterpolateAnchorPoints( gd,sc_sc->anchor,&orig_b,&new_b,genchange->hcounter_scale,true );
    InterpolateAnchorPoints( gd,sc_sc->anchor,&orig_b,&new_b,
	genchange->use_vert_mapping ? genchange->v_scale : genchange->vcounter_scale,false );

    /* Finally move every point to its new location */
    for ( i=0; i<gd->pcnt; i++ ) if ( gd->points[i].sp != NULL ) {
	pd = &gd->points[i];
	pd->sp->me.x = pd->newpos.x;
	pd->sp->me.y = pd->newpos.y;
	if ( !pd->sp->nonextcp ) {
	    pd->sp->nextcp.x = pd->newnext.x;
	    pd->sp->nextcp.y = pd->newnext.y;
	}
	if ( !pd->sp->noprevcp ) {
	    pd->sp->prevcp.x = pd->newprev.x;
	    pd->sp->prevcp.y = pd->newprev.y;
	}
    }
    SplineSetRefigure(sc_sc->layers[layer].splines);
    SplineCharLayerFindBounds(sc_sc,layer,&new_b);

    /* Set the left and right side bearings appropriately */
    memset(scale,0,sizeof(scale));
    scale[0] = scale[3] = 1;
    if ( genchange->center_in_hor_advance == 1 )
	scale[4] = ( owidth - ( new_b.maxx - new_b.minx ))/2.0 - new_b.minx;
    else if ( genchange->center_in_hor_advance == 2 )
	scale[4] = orig_b.minx/( owidth - ( orig_b.maxx - orig_b.minx )) *
	    ( owidth - ( new_b.maxx - new_b.minx )) - new_b.minx;
    else
	scale[4] = orig_b.minx*genchange->lsb_scale + genchange->lsb_add - new_b.minx;
    SplinePointListTransform(sc_sc->layers[layer].splines,scale,tpt_AllPoints);
    for ( ap = sc_sc->anchor; ap!=NULL; ap=ap->next )
	ap->me.x += scale[4];
    SplineCharLayerFindBounds( sc_sc,layer,&new_b );
    if ( !genchange->center_in_hor_advance )
	sc_sc->width = ( owidth-orig_b.maxx )*genchange->rsb_scale + genchange->rsb_add + new_b.maxx;

    /* If it is a subscript/superscript glyph, then move it to the desired vertical position */
    memset(scale,0,sizeof(scale));
    scale[0] = scale[3] = 1;
    scale[5] = genchange->vertical_offset;
    SplinePointListTransform(sc_sc->layers[layer].splines,scale,tpt_AllPoints);
    for ( ap = sc_sc->anchor; ap!=NULL; ap=ap->next )
	ap->me.y += genchange->vertical_offset;

    if ( genchange->dstem_control )
	free( dstems );
    GlyphDataFree( gd );
    StemInfosFree( sc_sc->hstem );  sc_sc->hstem = NULL;
    StemInfosFree( sc_sc->vstem );  sc_sc->vstem = NULL;
    DStemInfosFree( sc_sc->dstem ); sc_sc->dstem = NULL;
    SCRound2Int( sc_sc,layer, 1.0 );		/* This calls SCCharChangedUpdate(sc_sc,layer); */
}

/* ************************************************************************** */
/* ***************************** Small Capitals ***************************** */
/* ************************************************************************** */


extern int autohint_before_generate;

static int NumberLayerPoints(SplineSet *ss) {
    int cnt;
    SplinePoint *pt;

    cnt = 1;
    for ( ; ss!=NULL; ss=ss->next ) {
	for ( pt=ss->first; ; ) {
	    pt->ptindex = cnt++;
	    if ( pt->next==NULL )
	break;
	    pt = pt->next->to;
	    if ( pt==ss->first )
	break;
	}
    }
return( cnt );
}

static double CaseMajorVerticalStemWidth(SplineFont *sf, int layer,
	unichar_t *list, double tan_ia) {
    const int MW=100;
    struct widths { double width, total; } widths[MW];
    int cnt,i,j;
    double width, sum, total;
    SplineChar *sc, dummy, *which;
    Layer layers[2];
    char *snaps, *end;
    StemInfo *s;
    double val, diff, bestwidth, bestdiff;
    real deskew[6];

    memset(deskew,0,sizeof(deskew));
    deskew[0] = deskew[3] = 1;
    deskew[2] = tan_ia;

    cnt = 0;
    for ( i=0; list[i]!=0; ++i ) {
	sc = SFGetChar(sf,list[i],NULL);
	if ( sc==NULL )
    continue;
	if ( tan_ia== 0 && autohint_before_generate && (sc->changedsincelasthinted || sc->vstem==NULL ) &&
		!sc->manualhints )
	    SplineCharAutoHint(sc,layer,NULL);
	if ( tan_ia==0 && sc->vstem!=NULL ) {
	    which = sc;
	} else {
/* Ok, we couldn't hint it, or it was italic. Make a copy of it, deskew it, */
/*  and hint that */
	    memset(&dummy,0,sizeof(dummy));
	    memset(layers,0,sizeof(layers));
	    dummy.color = COLOR_DEFAULT;
	    dummy.layer_cnt = 2;
	    dummy.layers = layers;
	    dummy.parent = sc->parent;
	    dummy.name = copy("Fake");

	    dummy.layers[ly_fore].order2 = sc->layers[layer].order2;
	    dummy.layers[ly_fore].splines = SplinePointListTransform(
		    SplinePointListCopy(LayerAllSplines(&sc->layers[layer])),
		    deskew,tpt_AllPoints);
	    LayerUnAllSplines(&sc->layers[ly_fore]);

	    SplineCharAutoHint(&dummy,ly_fore,NULL);
	    which = &dummy;
	}
	for ( s= which->vstem; s!=NULL; s=s->next ) if ( !s->ghost ) {
	    for ( j=0; j<cnt; ++j )
		if ( widths[j].width==s->width )
	    break;
	    if ( j<cnt )
		widths[j].total += HIlen(s);
	    else if ( j<MW ) {
		++cnt;
		widths[j].width = s->width;
		widths[j].total = HIlen(s);
	    }
	}
	if ( which==&dummy )
	    SCClearContents(&dummy,ly_fore);
    }
    if ( cnt==0 )
return( -1 );

    /* Is there a width that occurs significantly more often than any other? */
    for ( i=0; i<cnt; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( widths[i].total > widths[j].total ) {
	    struct widths temp;
	    temp = widths[j];
	    widths[j] = widths[i];
	    widths[i] = temp;
	}
    }
    if ( cnt==1 || widths[cnt-1].total > 1.5*widths[cnt-2].total )
	width = widths[cnt-1].width;
    else {
	/* Ok, that didn't work find an average */
	sum = 0; total=0;
	for ( i=0; i<cnt; ++i ) {
	    sum += widths[i].total*widths[i].width;
	    total += widths[i].total;
	}
	width = sum/total;
    }

    /* Do we have a StemSnapV entry? */
    /* If so, snap width to the closest value in it */
    if ( sf->private!=NULL && (snaps = PSDictHasEntry(sf->private,"StemSnapV"))!=NULL ) {
	while ( *snaps==' ' || *snaps=='[' ) ++snaps;
	/* Must get at least this close, else we'll just use what we found */
	bestwidth = width; bestdiff = (sf->ascent+sf->descent)/100.0;
	while ( *snaps!='\0' && *snaps!=']' ) {
	    val = strtod(snaps,&end);
	    if ( snaps==end )
	break;
	    snaps = end;
	    while ( *snaps==' ' ) ++snaps;
	    if ( (diff = val-width)<0 ) diff = -diff;
	    if ( diff<bestdiff ) {
		bestwidth = val;
		bestdiff = diff;
	    }
	}
	width = bestwidth;
    }
return( width );
}

void SmallCapsFindConstants(struct smallcaps *small, SplineFont *sf,
	int layer ) {

    memset(small,0,sizeof(*small));

    small->sf = sf; small->layer = layer;
    small->italic_angle = sf->italicangle * 3.1415926535897932/180.0;
    small->tan_ia = tan( small->italic_angle );

    small->lc_stem_width = CaseMajorVerticalStemWidth(sf, layer,lc_stem_str, small->tan_ia );
    small->uc_stem_width = CaseMajorVerticalStemWidth(sf, layer,uc_stem_str, small->tan_ia );

    if ( small->uc_stem_width<=small->lc_stem_width || small->lc_stem_width==0 )
	small->stem_factor = 1;
    else
	small->stem_factor = small->lc_stem_width / small->uc_stem_width;
    small->v_stem_factor = small->stem_factor;

    small->xheight   = SFXHeight  (sf,layer,false);
    small->capheight = SFCapHeight(sf,layer,false);
    small->scheight  = small->xheight;
    if ( small->capheight>0 )
	small->vscale = small->scheight / small->capheight;
    else
	small->vscale = .75;
    small->hscale     = small->vscale;
}

static void MakeLookups(SplineFont *sf,OTLookup **lookups,int ltn,int crl,int grk,
	int symbols, uint32 ftag) {
    OTLookup *any = NULL;
    int i;
    struct lookup_subtable *sub;

    for ( i=0; i<3; ++i ) {
	if ( any==NULL )
	    any = lookups[i];
	else if ( lookups[i]!=NULL && lookups[i]!=any )
	    any = (OTLookup *) -1;
    }

    if ( any==(OTLookup *) -1 ) {
	/* Each script has it's own lookup. So if we are missing a script we */
	/*  should create a new lookup for it */
	if ( lookups[0]==NULL && ltn ) {
	    sub = SFSubTableFindOrMake(sf,ftag,CHR('l','a','t','n'),gsub_single);
	    lookups[0] = sub->lookup;
	}
	if ( lookups[1]==NULL && crl ) {
	    sub = SFSubTableFindOrMake(sf,ftag,CHR('c','y','r','l'),gsub_single);
	    lookups[1] = sub->lookup;
	}
	if ( lookups[2]==NULL && grk ) {
	    sub = SFSubTableFindOrMake(sf,ftag,CHR('g','r','e','k'),gsub_single);
	    lookups[2] = sub->lookup;
	}
	if ( lookups[3]==NULL && symbols ) {
	    sub = SFSubTableFindOrMake(sf,ftag,DEFAULT_SCRIPT,gsub_single);
	    lookups[3] = sub->lookup;
	}
    } else {
	if ( any!=NULL ) {
	    /* There's only one lookup, let's extend it to deal with any script */
	    /*  we need for which there is no lookup */
	} else {
	    /* No lookup. Create one for all the scripts we need */
	    sub = SFSubTableFindOrMake(sf,ftag,
		    ltn?CHR('l','a','t','n'):
		    crl?CHR('c','y','r','l'):
			CHR('g','r','e','k'),gsub_single);
	    any = sub->lookup;
	}
	if ( lookups[0]==NULL && ltn ) {
	    lookups[0] = any;
	    FListAppendScriptLang(FindFeatureTagInFeatureScriptList(ftag,any->features),CHR('l','a','t','n'),DEFAULT_LANG);
	}
	if ( lookups[1]==NULL && crl ) {
	    lookups[1] = any;
	    FListAppendScriptLang(FindFeatureTagInFeatureScriptList(ftag,any->features),CHR('c','y','r','l'),DEFAULT_LANG);
	}
	if ( lookups[2]==NULL && grk ) {
	    lookups[2] = any;
	    FListAppendScriptLang(FindFeatureTagInFeatureScriptList(ftag,any->features),CHR('g','r','e','k'),DEFAULT_LANG);
	}
	if ( lookups[3]==NULL && symbols ) {
	    lookups[3] = any;
	    FListAppendScriptLang(FindFeatureTagInFeatureScriptList(ftag,any->features),DEFAULT_SCRIPT,DEFAULT_LANG);
	}
    }
    for ( i=0; i<4; ++i ) {
	if ( lookups[i]!=NULL && lookups[i]->subtables==NULL ) {
	    lookups[i]->subtables = chunkalloc(sizeof(struct lookup_subtable));
	    lookups[i]->subtables->lookup = lookups[i];
	    lookups[i]->subtables->per_glyph_pst_or_kern = true;
	    NameOTLookup(lookups[i],sf);
	}
    }
}

static void MakeSCLookups(SplineFont *sf,struct lookup_subtable **c2sc,
	struct lookup_subtable **smcp,
	int ltn,int crl,int grk,int symbols,int petite ) {
    OTLookup *test;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    OTLookup *lc2sc[4], *lsmcp[4];
    int i;
    uint32 ucfeat, lcfeat;

    memset(lc2sc,0,sizeof(lc2sc)); memset(lsmcp,0,sizeof(lsmcp));

    if ( petite ) {
	ucfeat = CHR('c','2','p','c');
	lcfeat = CHR('p','c','a','p');
    } else {
	ucfeat = CHR('c','2','s','c');
	lcfeat = CHR('s','m','c','p');
    }
    if ( sf->cidmaster ) sf=sf->cidmaster;
    for ( test=sf->gsub_lookups; test!=NULL; test=test->next ) if ( test->lookup_type==gsub_single ) {
	for ( fl=test->features; fl!=NULL; fl=fl->next ) {
	    if ( fl->featuretag==ucfeat || fl->featuretag==lcfeat ) {
		for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
		    if ( sl->script==CHR('l','a','t','n')) {
			if ( fl->featuretag==ucfeat )
			    lc2sc[0] = test;
			else
			    lsmcp[0] = test;
		    } else if ( sl->script==CHR('c','y','r','l')) {
			if ( fl->featuretag==ucfeat )
			    lc2sc[1] = test;
			else
			    lsmcp[1] = test;
		    } else if ( sl->script==CHR('g','r','e','k')) {
			if ( fl->featuretag==ucfeat )
			    lc2sc[2] = test;
			else
			    lsmcp[2] = test;
		    }
		}
	    }
	}
    }

    MakeLookups(sf,lc2sc,ltn,crl,grk,symbols,ucfeat);
    MakeLookups(sf,lsmcp,ltn,crl,grk,symbols,lcfeat);

    for ( i=0; i<4; ++i ) {
	if ( lc2sc[i]!=NULL )
	    c2sc[i] = lc2sc[i]->subtables;
	if ( lsmcp[i]!=NULL )
	    smcp[i] = lsmcp[i]->subtables;
    }
}

static SplineChar *MakeSmallCapName(char *buffer, int bufsize, SplineFont *sf,
	SplineChar *sc,struct genericchange *genchange) {
    SplineChar *lc_sc;
    const char *ext;
    int lower;

    if ( sc->unicodeenc>=0 && sc->unicodeenc<0x10000 ) {
	lower = tolower(sc->unicodeenc);
	ext = isupper(sc->unicodeenc) ? genchange->extension_for_letters :
	      islower(sc->unicodeenc) ? genchange->extension_for_letters :
		sc->unicodeenc==0xdf  ? genchange->extension_for_letters :
		sc->unicodeenc>=0xfb00 && sc->unicodeenc<=0xfb06 ? genchange->extension_for_letters :
					    genchange->extension_for_symbols;
    } else {
	lower = sc->unicodeenc;
	ext = genchange->extension_for_symbols;
    }
    lc_sc = SFGetChar(sf,lower,NULL);
    if ( lc_sc!=NULL )
	snprintf(buffer,bufsize,"%s.%s", lc_sc->name, ext );
    else {
	const char *pt = StdGlyphName(buffer,lower,sf->uni_interp,sf->for_new_glyphs);
	if ( pt!=buffer )
	    strcpy(buffer,pt);
	strcat(buffer,".");
	strcat(buffer,ext);
    }
return( lc_sc );
}

static SplineChar *MakeSmallCapGlyphSlot(SplineFont *sf,SplineChar *cap_sc,
	uint32 script,struct lookup_subtable **c2sc,struct lookup_subtable **smcp,
	FontViewBase *fv, struct genericchange *genchange) {
    SplineChar *sc_sc, *lc_sc;
    char buffer[300];
    PST *pst;
    int enc;
    int script_index;

    lc_sc = MakeSmallCapName(buffer,sizeof(buffer),sf,cap_sc,genchange);
    sc_sc = SFGetChar(sf,-1,buffer);
    if ( sc_sc!=NULL ) {
	SCPreserveLayer(sc_sc,fv->active_layer,false);
	SCClearLayer(sc_sc,fv->active_layer);
return( sc_sc );
    }
    enc = SFFindSlot(sf, fv->map, -1, buffer );
    if ( enc==-1 )
	enc = fv->map->enccount;
    sc_sc = SFMakeChar(sf,fv->map,enc);
    free(sc_sc->name);
    sc_sc->name = copy(buffer);
    SFHashGlyph(sf,sc_sc);

    pst = chunkalloc(sizeof(PST));
    pst->next = cap_sc->possub;
    cap_sc->possub = pst;
    script_index = script==CHR('l','a','t','n')?0:
			 script==CHR('c','y','r','l')?1:
			 script==CHR('g','r','e','k')?2:3;
    pst->subtable = c2sc[script_index];
    pst->type = pst_substitution;
    pst->u.subs.variant = copy(buffer);

    /* Adobe's convention seems to be that symbols get both the lc and the uc */
    /*  feature attached to them. */
    if ( lc_sc!=NULL ) {
	pst = chunkalloc(sizeof(PST));
	pst->next = lc_sc->possub;
	lc_sc->possub = pst;
	pst->subtable = smcp[script_index];
	pst->type = pst_substitution;
	pst->u.subs.variant = copy(buffer);
    }
return( sc_sc );
}

struct overlaps { double start, stop, new_start, new_stop; };

static struct overlaps *SCFindHintOverlaps(StemInfo *hints,double min_coord,
	double max_coord, int *_tot, double *_counter_len) {
    struct overlaps *overlaps;
    StemInfo *h;
    int cnt, tot, i, j;
    double counter_len;

    for ( h=hints, cnt=0; h!=NULL; h=h->next ) if ( !h->ghost )
	++cnt;

    overlaps = malloc((cnt+3)*sizeof(struct overlaps));
    overlaps[0].start = min_coord; overlaps[0].stop = min_coord;
    overlaps[1].start = max_coord; overlaps[1].stop = max_coord;
    tot = 2;

    for ( h=hints; h!=NULL; h=h->next ) if ( !h->ghost ) {
	for ( i=0; i<tot && overlaps[i].stop<h->start; ++i );
	if ( i==tot )	/* Can't happen */
    continue;
	/* So h->start<=overlaps[i].stop */
	if ( h->start+h->width<overlaps[i].start ) {
	    /* New entry */
	    for ( j=tot; j>i; --j )
		overlaps[j] = overlaps[j-1];
	    overlaps[i].start = h->start;
	    overlaps[i].stop  = h->start+h->width;
	    ++tot;
	} else {
	    if ( h->start<overlaps[i].start )
		overlaps[i].start = h->start;
	    if ( h->start+h->width > overlaps[i].stop )
		overlaps[i].stop = h->start+h->width;
	    while ( i+1<tot && overlaps[i].stop>=overlaps[i+1].start ) {
		overlaps[i].stop = overlaps[i+1].stop;
		--tot;
		for ( j=i+1; j<tot; ++j )
		    overlaps[j] = overlaps[j+1];
	    }
	}
    }
    for ( i=0, counter_len=0; i<tot-1; ++i )
	counter_len += overlaps[i+1].start-overlaps[i].stop;

    *_tot = tot;
    *_counter_len = counter_len;
return( overlaps );
}

static double SCFindCounterLen(StemInfo *hints,double min_coord,
	double max_coord) {
    int tot=0;
    double counter_len=0;
    struct overlaps *overlaps;

    overlaps = SCFindHintOverlaps(hints,min_coord,max_coord,&tot,&counter_len);
    free(overlaps);
return( counter_len );
}

static void SmallCapsPlacePoints(SplineSet *ss,AnchorPoint *aps,
	int coord, StemInfo *hints,
	struct overlaps *overlaps, int tot) {
    struct ptpos { double old, new; int hint_index; } *ptpos;
    int cnt, i, order2;
    double val;
    StemInfo *h;
    SplineSet *spl;
    SplinePoint *sp, *first, *start, *last;
    AnchorPoint *ap;

    cnt = NumberLayerPoints(ss);
    ptpos = calloc(cnt,sizeof(struct ptpos));

    /* Position any points which lie within a hint zone */
    order2 = false;
    for ( spl=ss; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; ) {
	    sp->ticked = false;
	    val = (&sp->me.x)[coord];
	    ptpos[sp->ptindex].old = val;
	    ptpos[sp->ptindex].hint_index = -2;
	    for ( i=0; i<tot; ++i ) {
		if ( val>=overlaps[i].start && val<=overlaps[i].stop ) {
		    for ( h=hints; h!=NULL; h=h->next ) {
			if ( RealNear(val,h->start) || RealNear(val,h->start+h->width)) {
			    sp->ticked = true;
			    ptpos[sp->ptindex].hint_index = i;
			    ptpos[sp->ptindex].new = overlaps[i].new_start +
				    (val-overlaps[i].start);
		    break;
			}
		    }
	    break;
		}
	    }
	    if ( sp->next==NULL )
	break;
	    order2 = sp->next->order2;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    /* Look for any local minimum or maximum points */
    for ( spl=ss; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; ) {
	    if ( sp->next==NULL )
	break;
	    val = (&sp->me.x)[coord];
	    if ( !sp->ticked && ( IsExtremum(sp,coord) || IsAnglePoint(sp))) {
		for ( i=0; i<tot; ++i ) {
		    if ( val>=overlaps[i].start && val<overlaps[i].stop ) {
			sp->ticked = true;
			ptpos[sp->ptindex].new = overlaps[i].new_start +
				(val-overlaps[i].start) *
				    (overlaps[i].new_stop - overlaps[i].new_start)/
				    (overlaps[i].stop - overlaps[i].start);
		break;
		    } else if ( i>0 && val>=overlaps[i-1].stop && val<=overlaps[i].start ) {
			sp->ticked = true;
			ptpos[sp->ptindex].new = overlaps[i-1].new_stop +
				(val-overlaps[i-1].stop) *
				    (overlaps[i].new_start - overlaps[i-1].new_stop)/
				    (overlaps[i].start - overlaps[i-1].stop);
		break;
		    }
		}
	    }
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    /* Position any points which line between points already positioned */
    for ( spl=ss; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; !sp->ticked; ) {
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( !sp->ticked )		/* Nothing on this contour positioned */
    continue;
	first = sp;
	do {
	    start = sp;
	    if ( sp->next==NULL )
	break;
	    /* Find the next point which has been positioned */
	    for ( sp=sp->next->to; !sp->ticked; ) {
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==first )
	    break;
	    }
	    if ( !sp->ticked )
	break;
	    /* Interpolate any points between these two */
	    last = sp;
	    /* Make sure all the points are BETWEEN */
	    for ( sp=start->next->to; sp!=last; sp=sp->next->to ) {
		if (( (&sp->me.x)[coord] < (&start->me.x)[coord] && (&sp->me.x)[coord] < (&last->me.x)[coord]) ||
			( (&sp->me.x)[coord] > (&start->me.x)[coord] && (&sp->me.x)[coord] > (&last->me.x)[coord]))
	    break;
	    }
	    if ( sp==last ) {
		for ( sp=start->next->to; sp!=last; sp=sp->next->to ) {
		    ptpos[sp->ptindex].new = ptpos[start->ptindex].new +
			    ((&sp->me.x)[coord] - ptpos[start->ptindex].old) *
			      (ptpos[last->ptindex].new - ptpos[start->ptindex].new) /
			      (ptpos[last->ptindex].old - ptpos[start->ptindex].old);
		    sp->ticked = true;
		}
	    } else
		sp = last;
	} while ( sp!=first );
    }

    /* Any points which aren't currently positioned, just interpolate them */
    /*  between the hint zones between which they lie */
    /* I don't think this can actually happen... but do it just in case */
    for ( spl=ss; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; ) {
	    if ( !sp->ticked ) {
		val = (&sp->me.x)[coord];
		for ( i=0; i<tot; ++i ) {
		    if ( val>=overlaps[i].start && val<overlaps[i].stop ) {
			sp->ticked = true;
			ptpos[sp->ptindex].new = overlaps[i].new_start +
				(val-overlaps[i].start) *
				    (overlaps[i].new_stop - overlaps[i].new_start)/
				    (overlaps[i].stop - overlaps[i].start);
		break;
		    } else if ( i>0 && val>=overlaps[i-1].stop && val<=overlaps[i].start ) {
			sp->ticked = true;
			ptpos[sp->ptindex].new = overlaps[i-1].new_stop +
				(val-overlaps[i-1].stop) *
				    (overlaps[i].new_start - overlaps[i-1].new_stop)/
				    (overlaps[i].start - overlaps[i-1].stop);
		break;
		    }
		}
		if ( !sp->ticked ) {
		    IError( "Unticked point in remove space (smallcaps/italic/etc.)" );
		    ptpos[sp->ptindex].new = ptpos[sp->ptindex].old;
		}
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }
    /* And do the same for anchor points */
    for ( ap=aps; ap!=NULL; ap=ap->next ) {
	val = (&ap->me.x)[coord];
	for ( i=0; i<tot; ++i ) {
	    if ( val>=overlaps[i].start && val<overlaps[i].stop ) {
		(&ap->me.x)[coord] = overlaps[i].new_start +
			(val-overlaps[i].start) *
			    (overlaps[i].new_stop - overlaps[i].new_start)/
			    (overlaps[i].stop - overlaps[i].start);
	break;
	    } else if ( i>0 && val>=overlaps[i-1].stop && val<=overlaps[i].start ) {
		(&ap->me.x)[coord] = overlaps[i-1].new_stop +
			(val-overlaps[i-1].stop) *
			    (overlaps[i].new_start - overlaps[i-1].new_stop)/
			    (overlaps[i].start - overlaps[i-1].stop);
	break;
	    }
	}
	/* Anchor points might be outside the bounding box */
	if ( i==tot ) {
	    if ( val<overlaps[0].start )
		(&ap->me.x)[coord] += overlaps[0].new_start - overlaps[0].start;
	    else
		(&ap->me.x)[coord] += overlaps[tot-1].new_stop - overlaps[tot-1].stop;
	}
    }

    /* Interpolate the control points. More complex in order2. We want to */
    /*  preserve interpolated points, but simplified as we only have one cp */
    if ( !order2 ) {
	for ( spl=ss; spl!=NULL; spl=spl->next ) {
	    for ( sp=spl->first; ; ) {
		if ( sp->prev!=NULL ) {
		    if ( ptpos[sp->prev->from->ptindex].old == ptpos[sp->ptindex].old )
			(&sp->prevcp.x)[coord] = ptpos[sp->ptindex].new;
		    else
			(&sp->prevcp.x)[coord] = ptpos[sp->ptindex].new +
				((&sp->prevcp.x)[coord] - ptpos[sp->ptindex].old)*
				(ptpos[sp->prev->from->ptindex].new-ptpos[sp->ptindex].new)/
				(ptpos[sp->prev->from->ptindex].old-ptpos[sp->ptindex].old);
		}
		if ( sp->next==NULL )
	    break;
		if ( ptpos[sp->next->to->ptindex].old == ptpos[sp->ptindex].old )
		    (&sp->nextcp.x)[coord] = ptpos[sp->ptindex].new;
		else
		    (&sp->nextcp.x)[coord] = ptpos[sp->ptindex].new +
			    ((&sp->nextcp.x)[coord] - ptpos[sp->ptindex].old)*
			    (ptpos[sp->next->to->ptindex].new-ptpos[sp->ptindex].new)/
			    (ptpos[sp->next->to->ptindex].old-ptpos[sp->ptindex].old);
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
    } else {
	for ( spl=ss; spl!=NULL; spl=spl->next ) {
	    for ( sp=spl->first; ; ) {
		sp->ticked = SPInterpolate(sp);
		if ( sp->next==NULL )
	    break;
		if ( ptpos[sp->next->to->ptindex].old == ptpos[sp->ptindex].old )
		    (&sp->nextcp.x)[coord] = ptpos[sp->ptindex].new;
		else
		    (&sp->nextcp.x)[coord] = ptpos[sp->ptindex].new +
			    ((&sp->nextcp.x)[coord] - ptpos[sp->ptindex].old)*
			    (ptpos[sp->next->to->ptindex].new-ptpos[sp->ptindex].new)/
			    (ptpos[sp->next->to->ptindex].old-ptpos[sp->ptindex].old);
		(&sp->next->to->prevcp.x)[coord] = (&sp->nextcp.x)[coord];
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	    for ( sp=spl->first; ; ) {
		if ( sp->ticked ) {
		    ptpos[sp->ptindex].new = ((&sp->nextcp.x)[coord] + (&sp->prevcp.x)[coord])/2;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
    }

    /* Finally move every point to its new location */
    for ( spl=ss; spl!=NULL; spl=spl->next ) {
	for ( sp=spl->first; ; ) {
	    (&sp->me.x)[coord] = ptpos[sp->ptindex].new;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    free(ptpos);
}

static double SmallCapsRemoveSpace(SplineSet *ss,AnchorPoint *aps,StemInfo *hints,int coord,double remove,
	double min_coord, double max_coord ) {
    struct overlaps *overlaps;
    int i, tot, set;
    double counter_len, shrink;

    if ( remove > max_coord-min_coord )
return(0);

    /* Coalesce overlapping hint zones. These won't shrink, but the counters */
    /*  between them will */
    overlaps = SCFindHintOverlaps(hints, min_coord, max_coord, &tot, &counter_len );
    /* A glyph need not have any counters at all (lower case "l" doesn't) */
    if ( counter_len==0 ) {
	free( overlaps );
return( 0 );
    }

    if ( 1.5*remove > counter_len ) {
	/* The amount we need to remove is disproportionate to the counter */
	/*  space we have available from which to remove it. So just remove */
	/*  what seems reasonable */
	remove = 2*counter_len/3;
    }

    shrink = (counter_len-remove)/counter_len;
    /* 0 is a fixed point */
    /* extra->current is a known point */
    for ( i=0; i<tot && overlaps[i].stop<0; ++i );
    if ( i==tot ) {
	/* glyph is entirely <0 */
	set = tot-1;
	overlaps[set].new_stop = shrink*overlaps[set].stop;
	overlaps[set].new_start = overlaps[set].new_stop - (overlaps[set].stop - overlaps[set].start);
    } else if ( overlaps[i].start>0 ) {
	set = i;
	overlaps[set].new_start = shrink*overlaps[set].start;
	overlaps[set].new_stop  = overlaps[set].new_start + (overlaps[set].stop - overlaps[set].start);
    } else {
	set = i;
	overlaps[set].new_start = overlaps[set].start;
	overlaps[set].new_stop = overlaps[set].stop;
    }
    for ( i=set+1; i<tot; ++i ) {
	overlaps[i].new_start = overlaps[i-1].new_stop +
		(overlaps[i].start - overlaps[i-1].stop)*shrink;
	overlaps[i].new_stop  = overlaps[i].new_start +
		(overlaps[i].stop  -overlaps[i].start);
    }
    for ( i=set-1; i>=0; --i ) {
	overlaps[i].new_stop = overlaps[i+1].new_start -
		(overlaps[i+1].start - overlaps[i].stop)*shrink;
	overlaps[i].new_start = overlaps[i].new_stop - (overlaps[i].stop - overlaps[i].start);
    }

    SmallCapsPlacePoints(ss,aps,coord,hints,overlaps, tot);
    free(overlaps);
return( remove );
}

static void LowerCaseRemoveSpace(SplineSet *ss,AnchorPoint *aps,StemInfo *hints,int coord,
	struct fixed_maps *fix ) {
    struct overlaps *overlaps;
    int i,j,k,l, tot;
    double counter_len, shrink;

    /* This is a variant on the previous routine. Instead of removing a given */
    /*  amount to be spread out among all the counters, here we are given */
    /*  certain locations and told where they map to. Basically the same idea */
    /*  except the glyph is sub-divided and each chunk can have a different */
    /*  amount/proportion removed from it */
    /* The end maps will be for the min/max points of the glyph. so they will */
    /*  be present. Intermediate maps might not be (lower case "l" has nothing*/
    /*  at the xheight), in which case we just ignore that map. */

    /* Coalesce overlapping hint zones. These won't shrink, but the counters */
    /*  between them will */
    overlaps = SCFindHintOverlaps(hints, fix->maps[0].current, fix->maps[fix->cnt-1].current, &tot, &counter_len );
    /* A glyph need not have any counters at all */
    if ( counter_len==0 ) {
	free( overlaps );
return;
    }

    for ( j=0; j<tot; ++j )
	overlaps[j].new_start = -10000;

    k=-1;
    for ( i=0; i<fix->cnt; ++i ) {
	fix->maps[i].overlap_index = -1;
	for ( j=k+1; j<tot; ++j ) {
	    if ( (overlaps[j].start<=fix->maps[i].current+2 && overlaps[j].stop>=fix->maps[i].current-2 ) &&
		    (j==tot-1 ||
		     !(overlaps[j+1].start<=fix->maps[i].current+2 && overlaps[j+1].stop>=fix->maps[i].current-2 ))) {
		overlaps[j].new_start = fix->maps[i].desired + overlaps[j].start-fix->maps[i].current;
		overlaps[j].new_stop  = fix->maps[i].desired + overlaps[j].stop -fix->maps[i].current;
		fix->maps[i].overlap_index = j;
		if ( k!=-1 ) {
		    /* Position any hint overlap zones between the one we just*/
		    /*  positioned, and the one we positioned just before this*/
		    double osum = 0;
		    for ( l=k+1; l<j; ++l )
			osum += overlaps[l].stop-overlaps[l].start;
		    shrink = (overlaps[j].new_start-overlaps[k].new_stop - osum) /
				(overlaps[j].start -overlaps[k].stop     - osum);
		    for ( l=k+1; l<j; ++l ) {
			overlaps[l].new_start = overlaps[l-1].new_stop +
				shrink*(overlaps[l].start - overlaps[l-1].stop);
			overlaps[l].new_stop  = overlaps[l].new_start +
				(overlaps[l].stop - overlaps[l].start);
		    }
		}
		k = j;
	break;
	    }
	}
	if ( fix->maps[i].overlap_index==-1 ) {
	    /* remove this mapping, doesn't correspond to anything in the */
	    /*  current glyph */
	    if ( i==fix->cnt-1 && k!=-1 &&
		    overlaps[k].start<=fix->maps[i].current+2 &&
		    overlaps[k].stop>=fix->maps[i].current-2 )
		/* Ok, normally it's a bad thing if the last fix point doesn't */
		/*  get mapped, but here it gets mapped to essentially the same*/
		/*  place as the previous point (A glyph which is as high as */
		/*  the xheight, for instance), so we can afford to ignore it */;
	    else if ( i==0 || i==fix->cnt-1 )
		IError("Failed to position end points in LowerCaseRemoveSpace" );
	    for ( j=i+1; j<fix->cnt; ++j )
		fix->maps[j-1] = fix->maps[j];
	    --(fix->cnt);
	    --i;		/* Try again on the new zone */
	}
    }

    for ( j=0; j<tot; ++j ) {
	if ( overlaps[j].new_start == -10000 ) {
	    IError( "Hint zone not positioned" );
return;
	}
    }

    SmallCapsPlacePoints(ss,aps,coord,hints,overlaps, tot);
    free(overlaps);
}

static void BuildSCLigatures(SplineChar *sc_sc,SplineChar *cap_sc,int layer,
	struct genericchange *genchange) {
    static char *ligs[] = { "ff", "fi", "fl", "ffi", "ffl", "st", "st" };
    char *components;
    int width;
    RefChar *rlast, *r;
    SplineChar *rsc;
    char buffer[300];
    /* German eszet (the double s ligature) should become two small cap "S"es */

    if ( cap_sc->unicodeenc==0xdf )
	components = "ss";
    else if ( cap_sc->unicodeenc>=0xfb00 && cap_sc->unicodeenc<=0xfb06 )
	components = ligs[cap_sc->unicodeenc-0xfb00];
    else
return;

    width=0;
    rlast = NULL;
    while ( *components!='\0' ) {
	snprintf(buffer,sizeof(buffer),"%c.%s", *components, genchange->extension_for_letters );
	rsc = SFGetChar(genchange->sf,-1,buffer);
	if ( rsc!=NULL ) {
	    r = RefCharCreate();
	    free(r->layers);
	    r->layers = NULL;
	    r->layer_cnt = 0;
	    r->sc = rsc;
	    r->unicode_enc = rsc->unicodeenc;
	    r->orig_pos = rsc->orig_pos;
	    r->adobe_enc = getAdobeEnc(rsc->name);
	    r->transform[0] = r->transform[3] = 1.0;
	    r->transform[4] = width;
	    width += rsc->width;
	    r->next = NULL;
	    SCMakeDependent(sc_sc,rsc);
	    SCReinstanciateRefChar(sc_sc,r,layer);
	    if ( rlast==NULL )
		sc_sc->layers[layer].refs=r;
	    else
		rlast->next = r;
	    rlast = r;
	}
	++components;
    }
    sc_sc->width = width;
    SCCharChangedUpdate(sc_sc,layer);
}

void FVAddSmallCaps(FontViewBase *fv, struct genericchange *genchange) {
    int gid, enc, cnt, ltn,crl,grk,symbols;
    SplineFont *sf = fv->sf;
    SplineChar *sc, *sc_sc, *rsc, *achar=NULL;
    RefChar *ref, *r;
    struct lookup_subtable *c2sc[4], *smcp[4];
    char buffer[200];

    if ( sf->cidmaster!=NULL )
return;		/* Can't randomly add things to a CID keyed font */
    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL )
	sc->ticked = false;
    cnt=ltn=crl=grk=symbols=0;
    memset(c2sc,0,sizeof(c2sc)); memset(smcp,0,sizeof(smcp));

    for ( enc=0; enc<fv->map->enccount; ++enc ) {
	if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL ) {
	    if ( genchange->do_smallcap_symbols || ( sc->unicodeenc<0x10000 &&
		    (isupper(sc->unicodeenc) || islower(sc->unicodeenc) ||
		     (sc->unicodeenc>=0xfb00 && sc->unicodeenc<=0xfb06)) )) {
		uint32 script = SCScriptFromUnicode(sc);
		if ( script==CHR('l','a','t','n'))
		    ++ltn, ++cnt;
		else if ( script==CHR('g','r','e','k'))
		    ++grk, ++cnt;
		else if ( script==CHR('c','y','r','l'))
		    ++crl, ++cnt;
		else if ( genchange->do_smallcap_symbols )
		    ++symbols, ++cnt;
	    }
	}
    }
    if ( cnt==0 )
return;

    genchange->g.cnt = genchange->m.cnt+2;
    genchange->g.maps = malloc(genchange->g.cnt*sizeof(struct position_maps));
    genchange->sf     = fv->sf;
    genchange->layer  = fv->active_layer;

    MakeSCLookups(sf,c2sc,smcp,ltn,crl,grk,symbols,genchange->petite);
    ff_progress_start_indicator(10,_("Small Capitals"),
	_("Building small capitals"),NULL,cnt,1);
    for ( enc=0; enc<fv->map->enccount; ++enc ) {
	if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL ) {
	    if ( genchange->do_smallcap_symbols || ( sc->unicodeenc<0x10000 &&
		    (isupper(sc->unicodeenc) || islower(sc->unicodeenc) ||
		     (sc->unicodeenc>=0xfb00 && sc->unicodeenc<=0xfb06)) )) {
		uint32 script = SCScriptFromUnicode(sc);
		if ( script!=CHR('l','a','t','n') &&
			script!=CHR('g','r','e','k') &&
			script!=CHR('c','y','r','l') &&
			!genchange->do_smallcap_symbols )
    continue;
		if ( sc->unicodeenc<0x10000 && islower(sc->unicodeenc)) {
		    sc = SFGetChar(sf,toupper(sc->unicodeenc),NULL);
		    if ( sc==NULL )
      goto end_loop;
		}
		if ( sc->ticked )
      goto end_loop;
		/* make the glyph now, even if it contains refs, because we */
		/*  want to retain the encoding ordering */
		sc_sc = MakeSmallCapGlyphSlot(sf,sc,script,c2sc,smcp,fv,genchange);
		if ( sc_sc==NULL )
      goto end_loop;
		if ( sc->layers[fv->active_layer].splines==NULL )
    continue;	/* Deal with these later */
		sc->ticked = true;
		if ( achar==NULL )
		    achar = sc_sc;
		if ( sc->unicodeenc==0xdf || (sc->unicodeenc>=0xfb00 && sc->unicodeenc<=0xfb06))
		    BuildSCLigatures(sc_sc,sc,fv->active_layer,genchange);
		else
		    ChangeGlyph( sc_sc,sc,fv->active_layer,genchange );
      end_loop:
		if ( !ff_progress_next())
    break;
	    }
	}
    }
    /* OK. Here we have done all the base glyphs we are going to do. Now let's*/
    /*  look at things which depend on references */
    for ( enc=0; enc<fv->map->enccount; ++enc ) {
	if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL ) {
	    if ( genchange->do_smallcap_symbols || ( sc->unicodeenc<0x10000 &&
		    (isupper(sc->unicodeenc) || islower(sc->unicodeenc) ||
		     (sc->unicodeenc>=0xfb00 && sc->unicodeenc<=0xfb06)) )) {
		uint32 script = SCScriptFromUnicode(sc);
		if ( script!=CHR('l','a','t','n') &&
			script!=CHR('g','r','e','k') &&
			script!=CHR('c','y','r','l') &&
			!genchange->do_smallcap_symbols )
    continue;
		if ( sc->unicodeenc<0x10000 &&islower(sc->unicodeenc)) {
		    sc = SFGetChar(sf,toupper(sc->unicodeenc),NULL);
		    if ( sc==NULL )
      goto end_loop2;
		}
		if ( sc->layers[fv->active_layer].refs==NULL )
    continue;
		MakeSmallCapName(buffer,sizeof(buffer),sf,sc,genchange);
		sc_sc = SFGetChar(sf,-1,buffer);
		if ( sc_sc==NULL )	/* Should not happen */
		    sc_sc = MakeSmallCapGlyphSlot(sf,sc,script,c2sc,smcp,fv,genchange);
		if ( sc_sc==NULL )
      goto end_loop2;
		if ( achar==NULL )
		    achar = sc_sc;
		if ( SFGetAlternate(sf,sc->unicodeenc,sc,false)!=NULL )
		    SCBuildComposit(sf,sc_sc,fv->active_layer,NULL,true);
		if ( sc_sc->layers[fv->active_layer].refs==NULL ) {
		    RefChar *rlast = NULL;
		    for ( ref=sc->layers[fv->active_layer].refs; ref!=NULL; ref=ref->next ) {
			MakeSmallCapName(buffer,sizeof(buffer),sf,ref->sc,genchange);
			rsc = SFGetChar(sf,-1,buffer);
			/* Look for grave.taboldstyle, grave.sc, and grave */
			if ( rsc==NULL && isaccent(ref->sc->unicodeenc)) {
			    snprintf(buffer,sizeof(buffer),"%s.%s", ref->sc->name, genchange->extension_for_letters );
			    rsc = SFGetChar(sf,-1,buffer);
			}
			if ( rsc==NULL && isaccent(ref->sc->unicodeenc))
			    rsc = ref->sc;
			if ( rsc!=NULL ) {
			    r = RefCharCreate();
			    free(r->layers);
			    *r = *ref;
			    r->layers = NULL;
			    r->layer_cnt = 0;
			    r->next = NULL;
			    r->sc = rsc;
			    r->transform[4] *= genchange->hcounter_scale;
			    r->transform[5] *= genchange->use_vert_mapping ? genchange->v_scale : genchange->vcounter_scale;
			    SCMakeDependent(sc_sc,rsc);
			    SCReinstanciateRefChar(sc_sc,r,fv->active_layer);
			    if ( rlast==NULL )
				sc_sc->layers[fv->active_layer].refs=r;
			    else
				rlast->next = r;
			    rlast = r;
			    sc_sc->width = rsc->width;
			}
		    }
		    SCCharChangedUpdate(sc_sc,fv->active_layer);
		}
      end_loop2:
		if ( sc!=NULL ) {
		    if ( !sc->ticked && !ff_progress_next())
    break;
		    sc->ticked = true;
		}
	    }
	}
    }
    ff_progress_end_indicator();
    if ( achar!=NULL )
	FVDisplayGID(fv,achar->orig_pos);
    free(genchange->g.maps);
}

/* ************************************************************************** */
/* ************************** Subscript/Superscript ************************* */
/* ************************************************************************** */

static struct lookup_subtable *MakeSupSupLookup(SplineFont *sf,uint32 feature_tag,
	uint32 *scripts,int scnt) {
    OTLookup *test, *found;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int i;
    struct lookup_subtable *sub;

    if ( sf->cidmaster ) sf=sf->cidmaster;
    found = NULL;
    for ( i=0; i<scnt && found == NULL; ++i ) {
	for ( test=sf->gsub_lookups; test!=NULL; test=test->next ) if ( test->lookup_type==gsub_single ) {
	    if ( FeatureScriptTagInFeatureScriptList(feature_tag,scripts[i],test->features)) {
		found = test;
	break;
	    }
	}
    }

    if ( found==NULL ) {
	sub = SFSubTableFindOrMake(sf,feature_tag,scripts[0],gsub_single);
	found = sub->lookup;
    }
    fl = FindFeatureTagInFeatureScriptList(feature_tag,found->features);
    for ( i=0; i<scnt; ++i ) {
	for ( sl=fl->scripts; sl!=NULL && sl->script!=scripts[i]; sl=sl->next );
	if ( sl==NULL ) {
	    sl = chunkalloc(sizeof(struct scriptlanglist));
	    sl->script = scripts[i];
	    sl->lang_cnt = 1;
	    sl->langs[0] = DEFAULT_LANG;
	    sl->next = fl->scripts;
	    fl->scripts = sl;
	}
    }

return( found->subtables );
}

static SplineChar *MakeSubSupGlyphSlot(SplineFont *sf,SplineChar *sc,
	struct lookup_subtable *feature,
	FontViewBase *fv, struct genericchange *genchange) {
    SplineChar *sc_sc;
    char buffer[300];
    PST *pst;
    int enc;

    snprintf(buffer,sizeof(buffer),"%s.%s", sc->name, genchange->glyph_extension );
    sc_sc = SFGetChar(sf,-1,buffer);
    if ( sc_sc!=NULL ) {
	SCPreserveLayer(sc_sc,fv->active_layer,false);
	SCClearLayer(sc_sc,fv->active_layer);
return( sc_sc );
    }
    enc = SFFindSlot(sf, fv->map, -1, buffer );
    if ( enc==-1 )
	enc = fv->map->enccount;
    sc_sc = SFMakeChar(sf,fv->map,enc);
    free(sc_sc->name);
    sc_sc->name = copy(buffer);
    SFHashGlyph(sf,sc_sc);

    pst = chunkalloc(sizeof(PST));
    pst->next = sc->possub;
    sc->possub = pst;
    pst->subtable = feature;
    pst->type = pst_substitution;
    pst->u.subs.variant = copy(buffer);

return( sc_sc );
}

void FVGenericChange(FontViewBase *fv, struct genericchange *genchange) {
    int gid, enc, cnt;
    SplineFont *sf = fv->sf;
    SplineChar *sc, *sc_sc, *rsc, *achar=NULL;
    RefChar *ref, *r;
    struct lookup_subtable *feature;
    char buffer[200];

    if ( sf->cidmaster!=NULL && genchange->gc == gc_subsuper )
return;		/* Can't randomly add things to a CID keyed font */

    if ( genchange->small != NULL ) {
	genchange->italic_angle = genchange->small->italic_angle;
	genchange->tan_ia = genchange->small->tan_ia;
    }

    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL )
	sc->ticked = false;

    cnt = 0;
    for ( enc=0; enc<fv->map->enccount; ++enc ) {
	if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL ) {
	    ++cnt;
	}
    }
    if ( cnt==0 )
return;

    genchange->g.cnt = genchange->m.cnt+2;
    genchange->g.maps = malloc(genchange->g.cnt*sizeof(struct position_maps));

    if ( genchange->feature_tag!=0 ) {
	uint32 *scripts = malloc(cnt*sizeof(uint32));
	int scnt = 0;
	for ( enc=0; enc<fv->map->enccount; ++enc ) {
	    if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL ) {
		uint32 script = SCScriptFromUnicode(sc);
		int i;
		for ( i=0; i<scnt; ++i )
		    if ( scripts[i]==script )
		break;
		if ( i==scnt )
		    scripts[scnt++] = script;
	    }
	}

	feature = MakeSupSupLookup(sf,genchange->feature_tag,scripts,scnt);
	free(scripts);
    } else
	feature = NULL;

    if ( genchange->gc==gc_subsuper )
	ff_progress_start_indicator(10,_("Subscripts/Superscripts"),
	    _("Building sub/superscripts"),NULL,cnt,1);
    else
	ff_progress_start_indicator(10,_("Generic change"),
	    _("Changing glyphs"),NULL,cnt,1);

    for ( enc=0; enc<fv->map->enccount; ++enc ) {
	if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL ) {
	    if ( sc->ticked )
  goto end_loop;
	    if ( sc->layers[fv->active_layer].splines==NULL ) {
		/* Create the glyph now, so it gets encoded right, but otherwise */
		/*  skip for now */
		if ( genchange->glyph_extension != NULL )
		    sc_sc = MakeSubSupGlyphSlot(sf,sc,feature,fv,genchange);
    continue;	/* Deal with these later */
	    }
	    sc->ticked = true;
	    if ( genchange->glyph_extension != NULL ) {
		sc_sc = MakeSubSupGlyphSlot(sf,sc,feature,fv,genchange);
		if ( sc_sc==NULL )
      goto end_loop;
	    } else {
		sc_sc = sc;
		SCPreserveLayer(sc,fv->active_layer,true);
	    }
	    if ( achar==NULL )
		achar = sc_sc;
	    ChangeGlyph( sc_sc,sc,fv->active_layer,genchange );
      end_loop:
	    if ( !ff_progress_next())
    break;
	}
    }
    /* OK. Here we have done all the base glyphs we are going to do. Now let's*/
    /*  look at things which depend on references */
    /* This is only relevant if we've got an extension, else we just use the */
    /*  same old refs */
    if ( genchange->glyph_extension != NULL ) for ( enc=0; enc<fv->map->enccount; ++enc ) {
	if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL ) {
	    if ( sc->layers[fv->active_layer].refs==NULL )
	continue;
	    snprintf(buffer,sizeof(buffer),"%s.%s", sc->name, genchange->glyph_extension );
	    sc_sc = SFGetChar(sf,-1,buffer);
	    if ( sc_sc==NULL )	/* Should not happen */
		sc_sc = MakeSubSupGlyphSlot(sf,sc,feature,fv,genchange);
	    if ( sc_sc==NULL )
	  goto end_loop2;
	    if ( achar==NULL )
		achar = sc_sc;
	    /* BuildComposite can do a better job of positioning accents */
	    /*  than I'm going to do here... */
	    if ( sc->layers[fv->active_layer].splines==NULL &&
		    SFGetAlternate(sf,sc->unicodeenc,sc,false)!=NULL )
		SCBuildComposit(sf,sc_sc,fv->active_layer,NULL,true);
	    if ( sc_sc->layers[fv->active_layer].refs==NULL ) {
		RefChar *rlast = NULL;
		for ( ref=sc->layers[fv->active_layer].refs; ref!=NULL; ref=ref->next ) {
		    snprintf(buffer,sizeof(buffer),"%s.%s", ref->sc->name, genchange->glyph_extension );
		    rsc = SFGetChar(sf,-1,buffer);
		    if ( rsc==NULL && isaccent(ref->sc->unicodeenc))
			rsc = ref->sc;
		    if ( rsc!=NULL ) {
			r = RefCharCreate();
			free(r->layers);
			*r = *ref;
			r->layers = NULL;
			r->layer_cnt = 0;
			r->next = NULL;
			r->sc = rsc;
			r->transform[4] *= genchange->hcounter_scale;
			r->transform[5] *= genchange->use_vert_mapping ? genchange->v_scale : genchange->vcounter_scale;
			if ( rsc==ref->sc )
			    r->transform[5] += genchange->vertical_offset;
			SCMakeDependent(sc_sc,rsc);
			SCReinstanciateRefChar(sc_sc,r,fv->active_layer);
			if ( rlast==NULL )
			    sc_sc->layers[fv->active_layer].refs=r;
			else
			    rlast->next = r;
			rlast = r;
		    }
		}
		SCCharChangedUpdate(sc_sc,fv->active_layer);
	    }
      end_loop2:
	    if ( !sc->ticked && !ff_progress_next())
    break;
	    sc->ticked = true;
	}
    } else for ( enc=0; enc<fv->map->enccount; ++enc ) {
	if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL ) {
	    for ( ref=sc->layers[fv->active_layer].refs; ref!=NULL; ref=ref->next ) {
		ref->transform[4] *= genchange->hcounter_scale;
		ref->transform[5] *= genchange->use_vert_mapping ? genchange->v_scale : genchange->vcounter_scale;
	    }
	    if ( sc->layers[fv->active_layer].refs!=NULL )
		SCCharChangedUpdate(sc,fv->active_layer);
	}
    }
    ff_progress_end_indicator();
    if ( achar!=NULL )
	FVDisplayGID(fv,achar->orig_pos);
    free(genchange->g.maps);
}

void CVGenericChange(CharViewBase *cv, struct genericchange *genchange) {
    SplineChar *sc=cv->sc;
    int layer = CVLayer(cv);

    if ( genchange->gc != gc_generic || layer<0 )
return;

    if ( genchange->small != NULL ) {
	genchange->italic_angle = genchange->small->italic_angle;
	genchange->tan_ia = genchange->small->tan_ia;
    }

    genchange->g.cnt = genchange->m.cnt+2;
    genchange->g.maps = malloc(genchange->g.cnt*sizeof(struct position_maps));

    if ( sc->layers[layer].splines!=NULL ) {
	SCPreserveLayer(sc,layer,true);
	ChangeGlyph( sc,sc,layer,genchange );
    }

    free(genchange->g.maps);
}

SplineSet *SSControlStems(SplineSet *ss,double stemwidthscale, double stemheightscale,
	double hscale, double vscale, double xheight) {
    SplineFont dummysf;
    SplineChar dummy;
    Layer layers[2];
    LayerInfo li[2];
    struct genericchange genchange;
    SplineSet *spl;
    int order2 = 0;

    for ( spl=ss; spl!=NULL ; spl=spl->next ) {
	if ( spl->first->next!= NULL ) {
	    order2 = spl->first->next->order2;
    break;
	}
    }

    memset(&dummysf,0,sizeof(dummysf));
    memset(&dummy,0,sizeof(dummy));
    memset(&li,0,sizeof(li));
    memset(&layers,0,sizeof(layers));
    memset(&genchange,0,sizeof(genchange));

    dummysf.ascent = 800; dummysf.descent = 200;
    dummysf.layer_cnt = 2;
    dummysf.layers = li;
    li[ly_fore].order2 = order2;
    dummy.parent=&dummysf;
    dummy.name="nameless";
    dummy.layer_cnt = 2;
    dummy.layers = layers;
    dummy.unicodeenc = -1;
    layers[ly_fore].order2 = order2;
    layers[ly_fore].splines = ss;

    if ( hscale==-1 && vscale==-1 )
	hscale = vscale = 1;
    if ( stemwidthscale==-1 && stemheightscale==-1 )
	stemwidthscale = stemheightscale = 1;

    genchange.stem_width_scale  = stemwidthscale !=-1 ? stemwidthscale  : stemheightscale;
    genchange.stem_height_scale = stemheightscale!=-1 ? stemheightscale : stemwidthscale ;
    genchange.hcounter_scale    = hscale !=-1 ? hscale  : vscale;
    genchange.v_scale	   = vscale !=-1 ? vscale  : hscale;
    genchange.lsb_scale = genchange.rsb_scale = genchange.hcounter_scale;

    ChangeGlyph( &dummy,&dummy,ly_fore,&genchange );
return( ss );
}

/* ************************************************************************** */
/* ***************************** Condense/Extend **************************** */
/* ************************************************************************** */

/* We need to look at the counters. There are two types of counters: */
/*  the left and right side bearings */
/*  internal counters		*/
/* If the glyph is nicely hinted with vertical stems then all we need to */
/*  do is look at the hints. Complications "B" which has slightly different */
/*  counters top and bottom. */
/* I'm going to assume that LCG glyphs have at most two counter zones, one */
/*  near the bottom (baseline), one near the top */
/* However many glyphs have diagonal stems: V, A, W, M, K, X, Y */
/*  Many of these have two zones (like "B" above) W, M, K, Y (maybe X) */
/* Find the places where these guys hit the baseline/cap-height (x-height) */
/*  and define these as potential counter boundries. Ignore places where   */
/*  glyphs hit with curves (like O, Q, p). */
/* Remember to merge a hint with a top/bottom hit (harder with serifs) */

/* We may still not have a useful counter: 7 3 2 C E T */
/*  Use the left and right sides of the bounding box (do I need to know */
/*  StemSnapV? -- standard stem size -- yes, don't want to expand the stem) */
/* Don't try to make I 1 l i grow, only one stem even if the serifs make the */
/*  bounding box bigger */

/* If the font is italic, then skew it by the italic angle in hopes of getting*/
/*  some real vertical stems, rehint, condense/extend & unskew */

double SFStdVW(SplineFont *sf) {
    double stdvw = 0;
    char *ret;

    if ( sf->private!=NULL && (ret=PSDictHasEntry(sf->private,"StdVW"))!=NULL )
	stdvw = strtod(ret,NULL);

    if ( stdvw<=0 )
	stdvw = (sf->ascent+sf->descent)/12.5;
return( stdvw );
}

static void CIAdd(struct counterinfo *ci,int z,double start,double width) {
    int i, j;

    if ( width<0 ) {
	start += width;
	width = -width;
    }
    for ( i = 0; i<ci->cnts[z]; ++i ) {
	if ( start+width<ci->zones[z][i].start )
    break;
	if ( start<ci->zones[z][i].start + ci->zones[z][i].width )
return;		/* It intersects something that's already there */
		/* Assume the previous entry came from a hint and */
		/* so specifies the stem without the serifs and is better */
    }

    /* Need to add */
    if ( ci->cnts[z]>=ci->maxes[z] )
	ci->zones[z] = realloc(ci->zones[z],(ci->maxes[z]+=10)*sizeof(struct ci_zones));
    for ( j=ci->cnts[z]; j>i; --j )
	ci->zones[z][j] = ci->zones[z][j-1];
    ci->zones[z][i].start = ci->zones[z][i].moveto   = start;
    ci->zones[z][i].width = ci->zones[z][i].newwidth = width;
    ++ ci->cnts[z];
}

static int SpOnEdge(SplinePoint *sp,double y,int dir,struct counterinfo *ci,int z) {
    SplinePoint *nsp, *nnsp, *psp;

    if ( sp->me.y<=y-1 || sp->me.y>y+1 )
return( false );

    /* We've already checked that we have a closed contour, so we don't need */
    /*  to worry that something might be NULL */
    psp = sp->prev->from;		/* the previous point must not be near the y value */
    if (( psp->me.y>y-1 && psp->me.y<=y+1 ) ||
	    ( dir>0 && psp->me.y<=y ) ||
	    ( dir<0 && psp->me.y>=y ) )
return( true );				/* But the point itself was on the edge */

    /* Either the next point is on the edge too, or we can have a dished */
    /*  serif, where the next point is off the edge, but the one after is on */
    /*  In a TrueType font there may be several such points, but for a PS */
    /*  flex hint there will be only one */
    nsp = sp->next->to;
    while ( nsp!=sp &&
	    ((dir>0 && nsp->me.y>y+1 && nsp->me.y<y+10) ||
	     (dir<0 && nsp->me.y<y-1 && nsp->me.y>y-10)) )
	nsp = nsp->next->to;
    if ( nsp==sp )
return( true );
    if ( nsp->me.y<=y-1 || nsp->me.y>y+1 )
return( true );
    nnsp = nsp->next->to;
    if (( nnsp->me.y>y-1 && nnsp->me.y<=y+1 ) ||
	( dir>0 && nnsp->me.y<=y ) ||
	( dir<0 && nnsp->me.y>=y ) )
return( true );

    if ( nsp->me.x-sp->me.x > 3.5 * ci->stdvw || nsp->me.x-sp->me.x < -3.5*ci->stdvw )
return( true );
    CIAdd(ci,z,sp->me.x,nsp->me.x-sp->me.x);
return( true );
}

static int HintActiveAt(StemInfo *h,double y) {
    HintInstance *hi;

    for ( hi=h->where; hi!=NULL; hi=hi->next ) {
	if ( y>=hi->begin && y<=hi->end )
return( true );
    }
return( false );
}

static void PerGlyphFindCounters(struct counterinfo *ci,SplineChar *sc, int layer) {
    StemInfo *h;
    double y, diff;
    int i,z;
    DBounds b;
    SplineSet *ss;
    SplinePoint *sp;

    ci->sc = sc;
    ci->layer = layer;
    ci->bottom_y = 0;
    if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 && isupper(sc->unicodeenc))
	ci->top_y = ci->bd.caph>0?ci->bd.caph:4*sc->parent->ascent/5;
    else
	ci->top_y = ci->bd.xheight>0?ci->bd.xheight:sc->parent->ascent/2;
    ci->boundry = ci->top_y/2;
    ci->has_two_zones = false;
    ci->cnts[0] = ci->cnts[1] = 0;

    diff = (ci->top_y - ci->bottom_y)/16.0;
    for ( h=sc->vstem; h!=NULL; h=h->next ) {
	for ( i=1, y=ci->bottom_y+diff; i<16; ++i, y+=diff ) {
	    if ( HintActiveAt(h,y)) {
		if ( i<8 ) {
		    CIAdd(ci,BOT_Z,h->start,h->width);
		    y += (7-i)*diff;
		    i = 7;
		} else if ( i==8 ) {
		    CIAdd(ci,BOT_Z,h->start,h->width);
		    CIAdd(ci,TOP_Z,h->start,h->width);
	break;
		} else {
		    CIAdd(ci,TOP_Z,h->start,h->width);
	break;
		}
	    }
	}
    }

    for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	if ( ss->first->prev==NULL )
    continue;
	for ( sp=ss->first; ; ) {
	    if ( SpOnEdge(sp,ci->bottom_y,1,ci,BOT_Z)) {
		/* All Done */
		;
	    } else if ( SpOnEdge(sp,ci->top_y,-1,ci,TOP_Z)) {
		/* All Done */
		;
            }
	    /* Checked for sp->next==NULL above loop */
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }

    SplineSetFindBounds(sc->layers[layer].splines,&b);
    ci->bb = b;
    if ( ci->cnts[0]<2 && ci->cnts[1]<2 ) {
	if ( b.maxx - b.minx > 4*ci->stdvw ) {
	    for ( i=0; i<2; ++i ) {
		CIAdd(ci,i,b.minx,1.5*ci->stdvw);
		CIAdd(ci,i,b.maxx-1.5*ci->stdvw, 1.5*ci->stdvw);
	    }
	}
    }

    if ( ci->cnts[0]!=ci->cnts[1] )
	ci->has_two_zones = true;
    else {
	for ( i=0; i<ci->cnts[0]; ++i ) {
	    /* if one stem is entirely within the other, then that counts as */
	    /*  the same */
	    if (( ci->zones[0][i].start<=ci->zones[1][i].start &&
		    ci->zones[0][i].start+ci->zones[0][i].width >= ci->zones[1][i].start+ci->zones[1][i].width) ||
		  ( ci->zones[1][i].start<=ci->zones[0][i].start &&
		    ci->zones[1][i].start+ci->zones[1][i].width >= ci->zones[0][i].start+ci->zones[0][i].width) )
	continue;		/* They match, close enough */
	    ci->has_two_zones = true;
	break;
	}
    }

    /* We do not include the side bearing adjustment here. That must be done  */
    /*  separately, because we skip counter movements if there are no counters*/
    for ( z=0; z<2; ++z ) {
	for ( i=1; i<ci->cnts[z]; ++i ) {
	    ci->zones[z][i].moveto = ci->zones[z][i-1].moveto + ci->zones[z][i-1].newwidth +
		    ci->c_factor/100.0 * (ci->zones[z][i].start-(ci->zones[z][i-1].start+ci->zones[z][i-1].width)) +
		    ci->c_add;
	}
    }

    if ( ci->has_two_zones ) {
	int j,k;
	double diff;
	/* Now are there any common stems in the two zones? Common stems */
	/*  should be forced to the same location even if that isn't what */
	/*  we calculated above */
	for ( i=0; i<ci->cnts[0]; ++i ) {
	    for ( j=0; j<ci->cnts[1]; ++j ) {
		if ( ci->zones[0][i].start == ci->zones[1][j].start &&
			ci->zones[0][i].moveto != ci->zones[1][j].moveto ) {
		    if ( ci->zones[0][i].moveto > ci->zones[1][j].moveto ) {
			diff = ci->zones[0][i].moveto - ci->zones[1][j].moveto;
			for ( k=j; k<ci->cnts[1]; ++k )
			    ci->zones[1][j].moveto += diff;
		    } else {
			diff = ci->zones[1][j].moveto - ci->zones[0][i].moveto;
			for ( k=j; k<ci->cnts[0]; ++k )
			    ci->zones[0][i].moveto += diff;
		    }
		}
	    }
	}
    }
}

static void BPAdjustCEZ(BasePoint *bp, struct counterinfo *ci, int z) {
    int i;

    if ( ci->cnts[z]<2 )	/* No counters */
return;
    if ( bp->x<ci->zones[z][0].start+ci->zones[z][0].width ) {
	if ( bp->x<ci->zones[z][0].start || ci->zones[z][0].width==ci->zones[z][0].newwidth )
	    bp->x += ci->zones[z][0].moveto - ci->zones[z][0].start;
	else
	    bp->x = ci->zones[z][0].moveto +
		    ci->zones[z][0].newwidth * (bp->x-ci->zones[z][0].start)/ ci->zones[z][0].width;
return;
    }

    for ( i=1; i<ci->cnts[z]; ++i ) {
	if ( bp->x<ci->zones[z][i].start+ci->zones[z][i].width ) {
	    if ( bp->x<ci->zones[z][i].start ) {
		double base = ci->zones[z][i-1].moveto + ci->zones[z][i-1].newwidth;
		double oldbase = ci->zones[z][i-1].start + ci->zones[z][i-1].width;
		bp->x = base +
			(bp->x-oldbase) *
				(ci->zones[z][i].moveto-base)/
				(ci->zones[z][i].start-oldbase);
	    } else {
		bp->x = ci->zones[z][i].moveto +
		    ci->zones[z][i].newwidth * (bp->x-ci->zones[z][i].start)/ ci->zones[z][i].width;
	    }
return;
	}
    }

    bp->x += ci->zones[z][i-1].moveto + ci->zones[z][i-1].newwidth -
	    (ci->zones[z][i-1].start + ci->zones[z][i-1].width);
}

static void BPAdjustCE(BasePoint *bp, struct counterinfo *ci) {

    if ( !ci->has_two_zones )
	BPAdjustCEZ(bp,ci,0);
    else if ( ci->cnts[BOT_Z]<2 && ci->cnts[TOP_Z]>=2 )
	BPAdjustCEZ(bp,ci,TOP_Z);
    else if ( ci->cnts[TOP_Z]<2 && ci->cnts[BOT_Z]>=2 )
	BPAdjustCEZ(bp,ci,BOT_Z);
    else if ( bp->y > ci->boundry )
	BPAdjustCEZ(bp,ci,TOP_Z);
    else
	BPAdjustCEZ(bp,ci,BOT_Z);
}

void CI_Init(struct counterinfo *ci,SplineFont *sf) {

    QuickBlues(sf, ci->layer, &ci->bd);

    ci->stdvw = SFStdVW(sf);
}

void SCCondenseExtend(struct counterinfo *ci,SplineChar *sc, int layer,
	int do_undoes) {
    SplineSet *ss;
    SplinePoint *sp;
    Spline *s, *first;
    DBounds b;
    int width;
    double offset;
    real transform[6];
    int order2 = sc->layers[layer].order2;

    if ( do_undoes )
	SCPreserveLayer(sc,layer,false);

    if ( ci->correct_italic && sc->parent->italicangle!=0 ) {
	memset(transform,0,sizeof(transform));
	transform[0] = transform[3] = 1;
	transform[2] = tan( sc->parent->italicangle * 3.1415926535897932/180.0 );
	SplinePointListTransform(sc->layers[layer].splines,transform,tpt_AllPoints);
	StemInfosFree(sc->vstem); sc->vstem=NULL;
    }
    if ( sc->vstem==NULL )
	_SplineCharAutoHint(sc,ci->layer,&ci->bd,NULL,false);

    PerGlyphFindCounters(ci,sc, layer);

    for ( ss = sc->layers[layer].splines; ss!=NULL; ss = ss->next ) {
	for ( sp=ss->first; ; ) {
	    BPAdjustCE(&sp->nextcp,ci);
	    BPAdjustCE(&sp->prevcp,ci);
	    if ( sp->ttfindex==0xffff && order2 ) {
		sp->me.x = (sp->nextcp.x+sp->prevcp.x)/2;
		sp->me.y = (sp->nextcp.y+sp->prevcp.y)/2;
	    } else
		BPAdjustCE(&sp->me,ci);
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
	first = NULL;
	for ( s=ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	    if ( first==NULL ) first = s;
	    SplineRefigure(s);
	}
    }

    SplineSetFindBounds(sc->layers[layer].splines,&b);
    memset(transform,0,sizeof(transform));
    transform[0] = transform[3] = 1;
    transform[4] = ci->bb.minx*ci->sb_factor/100. + ci->sb_add - b.minx;
    if ( transform[4]!=0 )
	SplinePointListTransform(sc->layers[layer].splines,transform,tpt_AllPoints);
    if ( layer!=ly_back ) {
	width = b.maxx + (sc->width-ci->bb.maxx)*ci->sb_factor/100. + ci->sb_add;
	SCSynchronizeWidth(sc,width,sc->width,NULL);
	offset = (b.maxx-b.minx)/2 - (ci->bb.maxx-ci->bb.minx)/2;
	/* We haven't really changed the left side bearing by offset, but */
	/*  this is the amount (about) by which we need to adjust accents */
	SCSynchronizeLBearing(sc,offset,ci->layer);
    }

    if ( ci->correct_italic && sc->parent->italicangle!=0 ) {
	/* If we unskewed it, we want to skew it now */
	memset(transform,0,sizeof(transform));
	transform[0] = transform[3] = 1;
	transform[2] = -tan( sc->parent->italicangle * 3.1415926535897932/180.0 );
	SplinePointListTransform(sc->layers[layer].splines,transform,tpt_AllPoints);
    }

    if ( layer!=ly_back ) {
	/* Hints will be inccorrect (misleading) after these transformations */
	StemInfosFree(sc->vstem); sc->vstem=NULL;
	StemInfosFree(sc->hstem); sc->hstem=NULL;
	DStemInfosFree(sc->dstem); sc->dstem=NULL;
	SCOutOfDateBackground(sc);
    }
    SCCharChangedUpdate(sc,layer);
}

void ScriptSCCondenseExtend(SplineChar *sc,struct counterinfo *ci) {

    SCCondenseExtend(ci, sc, ci->layer, true);

    free( ci->zones[0]);
    free( ci->zones[1]);
}

void FVCondenseExtend(FontViewBase *fv,struct counterinfo *ci) {
    int i, gid;
    SplineChar *sc;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL ) {
	SCCondenseExtend(ci,sc,ly_fore,true);
    }

    free( ci->zones[0]);
    free( ci->zones[1]);
}

/* ************************************************************************** */
/* ***************************** Embolden Dialog **************************** */
/* ************************************************************************** */


struct ptmoves {
    SplinePoint *sp;
    BasePoint pdir, ndir;
    double factor;
    BasePoint newpos;
    uint8 touched;
};

static int MaxContourCount(SplineSet *ss) {
    int cnt, ccnt;
    SplinePoint *sp;

    ccnt = 0;
    for ( ; ss!=NULL ; ss=ss->next ) {
	if ( ss->first->prev==NULL )
    continue;
	for ( cnt=0, sp=ss->first; ; ) {
	    sp = sp->next->to; ++cnt;
	    if ( sp==ss->first )
	break;
	}
	if ( cnt>ccnt ) ccnt = cnt;
    }
return( ccnt );
}

static int PtMovesInitToContour(struct ptmoves *ptmoves,SplineSet *ss) {
    int cnt;
    SplinePoint *sp;
    BasePoint dir1, dir2;
    double len;

    for ( cnt=0, sp=ss->first; ; ) {
	ptmoves[cnt].sp = sp;
	ptmoves[cnt].newpos = sp->me;
	ptmoves[cnt].touched = false;
	if ( sp->nonextcp ) {
	    dir1.x = sp->next->to->me.x - sp->me.x;
	    dir1.y = sp->next->to->me.y - sp->me.y;
	} else {
	    dir1.x = sp->nextcp.x - sp->me.x;
	    dir1.y = sp->nextcp.y - sp->me.y;
	}
	len = dir1.x*dir1.x + dir1.y*dir1.y;
	if ( len!=0 ) {
	    len = sqrt(len);
	    dir1.x /= len;
	    dir1.y /= len;
	}
	ptmoves[cnt].ndir = dir1;
	if ( dir1.x<0 ) dir1.x = -dir1.x;

	if ( sp->noprevcp ) {
	    dir2.x = sp->prev->from->me.x - sp->me.x;
	    dir2.y = sp->prev->from->me.y - sp->me.y;
	} else {
	    dir2.x = sp->prevcp.x - sp->me.x;
	    dir2.y = sp->prevcp.y - sp->me.y;
	}
	len = dir2.x*dir2.x + dir2.y*dir2.y;
	if ( len!=0 ) {
	    len = sqrt(len);
	    dir2.x /= len;
	    dir2.y /= len;
	}
	ptmoves[cnt].pdir = dir2;
	if ( dir2.x<0 ) dir2.x = -dir2.x;

	ptmoves[cnt].factor = dir1.x>dir2.x ? dir1.x : dir2.x;

	sp = sp->next->to; ++cnt;
	if ( sp==ss->first )
    break;
    }
    ptmoves[cnt] = ptmoves[0];	/* Life is easier if we don't have to worry about edge effects */
return( cnt );
}

static void InterpolateControlPointsAndSet(struct ptmoves *ptmoves,int cnt) {
    SplinePoint *sp, *nsp;
    int i;
    int order2 = ptmoves[0].sp->next!=NULL && ptmoves[0].sp->next->order2;

    ptmoves[cnt].newpos = ptmoves[0].newpos;
    for ( i=0; i<cnt; ++i ) {
	sp = ptmoves[i].sp;
	nsp = ptmoves[i+1].sp;
	if ( sp->nonextcp )
	    sp->nextcp = ptmoves[i].newpos;
	if ( nsp->noprevcp )
	    nsp->prevcp = ptmoves[i+1].newpos;
	if ( isnan(ptmoves[i].newpos.y) )
	    IError( "Nan value in InterpolateControlPointsAndSet\n" );
	if ( sp->me.y!=nsp->me.y ) {
	    sp->nextcp.y = ptmoves[i].newpos.y + (sp->nextcp.y-sp->me.y)*
				(ptmoves[i+1].newpos.y - ptmoves[i].newpos.y)/
				(nsp->me.y - sp->me.y);
	    nsp->prevcp.y = ptmoves[i].newpos.y + (nsp->prevcp.y-sp->me.y)*
				(ptmoves[i+1].newpos.y - ptmoves[i].newpos.y)/
				(nsp->me.y - sp->me.y);
	    if ( sp->me.x!=nsp->me.x ) {
		sp->nextcp.x = ptmoves[i].newpos.x + (sp->nextcp.x-sp->me.x)*
				    (ptmoves[i+1].newpos.x - ptmoves[i].newpos.x)/
				    (nsp->me.x - sp->me.x);
		nsp->prevcp.x = ptmoves[i].newpos.x + (nsp->prevcp.x-sp->me.x)*
				    (ptmoves[i+1].newpos.x - ptmoves[i].newpos.x)/
				    (nsp->me.x - sp->me.x);
	    }
	}
	if ( isnan(sp->nextcp.y) )
	    IError( "Nan value in InterpolateControlPointsAndSet\n" );
    }
    for ( i=0; i<cnt; ++i )
	ptmoves[i].sp->me = ptmoves[i].newpos;
    if ( order2 ) {
	for ( i=0; i<cnt; ++i ) if ( (sp = ptmoves[i].sp)->ttfindex==0xffff ) {
	    sp->me.x = (sp->nextcp.x+sp->prevcp.x)/2;
	    sp->me.y = (sp->nextcp.y+sp->prevcp.y)/2;
	}
    }
    for ( i=0; i<cnt; ++i )
	SplineRefigure(ptmoves[i].sp->next);
}

static void CorrectLeftSideBearing(SplineSet *ss_expanded,SplineChar *sc,int layer) {
    real transform[6];
    DBounds old, new;
    /* Now correct the left side bearing */

    SplineSetFindBounds(sc->layers[layer].splines,&old);
    SplineSetFindBounds(ss_expanded,&new);
    memset(transform,0,sizeof(transform));
    transform[0] = transform[3] = 1;
    transform[4] = old.minx - new.minx;
    if ( transform[4]!=0 ) {
	SplinePointListTransform(ss_expanded,transform,tpt_AllPoints);
	if ( layer==ly_fore )
	    SCSynchronizeLBearing(sc,transform[4],layer);
    }
}

static SplinePoint *FindMatchingPoint(int ptindex,SplineSet *ss) {
    SplinePoint *sp;

    if( ptindex==0 )
return( NULL );
    for ( ; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->ptindex == ptindex )
return( sp );
	    if ( sp->next == NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
return( NULL );
}

static StemInfo *OnHint(StemInfo *stems,double searchy,double *othery) {
    StemInfo *h;

    for ( h=stems; h!=NULL; h=h->next ) {
	if ( h->start == searchy ) {
	    *othery = h->start+h->width;
return( h );
	} else if ( h->start+h->width == searchy ) {
	    *othery = h->start;
return( h );
	}
    }

    for ( h=stems; h!=NULL; h=h->next ) {
	if ( searchy>=h->start-2 && searchy<=h->start+2 ) {
	    *othery = h->start+h->width;
return( h );
	} else if ( searchy>=h->start+h->width-2 && searchy<=h->start+h->width+2 ) {
	    *othery = h->start;
return( h );
	}
    }

return( NULL );
}

static StemInfo *MightBeOnHint(StemInfo *stems,struct lcg_zones *zones,
	struct ptmoves *pt,double *othery) {
    double offset;

    if ( pt->ndir.y!=0 && pt->pdir.y!=0 )
return( NULL );			/* Not horizontal, not on a stem */

    offset = (pt->ndir.y==0 && pt->ndir.x>0) ||
	    (pt->pdir.y==0 && pt->pdir.x<0 ) ?  zones->stroke_width/2
					     : -zones->stroke_width/2;
return( OnHint(stems,pt->sp->me.y-offset,othery));
}

static int InHintAroundZone(StemInfo *stems,double searchy,double contains_y) {
    StemInfo *h;

    for ( h=stems; h!=NULL; h=h->next ) {
	if ( h->start >= searchy && h->start+h->width<=searchy &&
		 h->start >= contains_y && h->start+h->width<=contains_y )
return( true );
    }
return( false );
}

static int IsStartPoint(SplinePoint *sp, SplineChar *sc, int layer) {
    SplineSet *ss;

    if ( sp->ptindex==0 )
return( false );
    for ( ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	if ( ss->first->ptindex==sp->ptindex )
return( true );
    }
return( false );
}

static void FindStartPoint(SplineSet *ss_expanded, SplineChar *sc, int layer) {
    SplinePoint *sp;
    SplineSet *ss;

    for ( ss=ss_expanded; ss!=NULL; ss=ss->next ) {
	int found = false;
	if ( ss->first->prev==NULL )
    continue;
	for ( sp=ss->first; ; ) {
	    if ( IsStartPoint(sp,sc,layer) ) {
		ss->first = ss->last = sp;
		ss->start_offset = 0;
		found = true;
	break;
	    }
	    sp = sp->prev->from;
	    if ( sp==ss->first )
	break;
	}
	if ( !found ) {
	    ss->first = ss->last = sp->prev->from;		/* Often true */
	    ss->start_offset = 0;
	}
    }
}

static SplineSet *LCG_EmboldenHook(SplineSet *ss_expanded,struct lcg_zones *zones,
	SplineChar *sc, int layer) {
    SplineSet *ss;
    SplinePoint *sp, *nsp, *psp;
    int cnt, ccnt, i;
    struct ptmoves *ptmoves;
    /* When we do an expand stroke we expect that a glyph's bounding box will */
    /*  increase by half the stroke width in each direction. Now we want to do*/
    /*  different operations in each dimension. We don't want to increase the */
    /*  x-height, or the cap height, whereas it is ok to make the glyph wider */
    /*  but it would be nice to preserve the left and right side bearings. */
    /* So shift the glyph over by width/2 (this preserves the left side bearing)*/
    /* Increment the width by width (this preserves the right side bearing)   */
    /* The y dimension is more complex. In a normal latin (greek, cyrillic)   */
    /*  font the x-height is key, in a font with only capital letters it would*/
    /*  be the cap-height. In the subset of the font containing superscripts  */
    /*  we might want to key off of the superscript size. The expansion will  */
    /*  mean that the glyph protrudes width/2 below the baseline, and width/2 */
    /*  above the x-height and cap-height. Neither of those is acceptable. So */
    /*  choose some intermediate level, say x-height/2, any point above this  */
    /*  moves down by width/2, any point below this moves up by width/2 */
    /* That'll work for many glyphs. But consider "o", depending on how it is */
    /*  designed we might be moving the leftmost point either up or down when */
    /*  it should remain near the center. So perhaps we have a band which is  */
    /*  width wide right around x-height/2 and points in that band don't move */
    /* Better. Consider a truetype "o" were there will be intermediate points */
    /*  Between the top-most and left-most. These shouldn't move by the full */
    /*  width/2. They should move by some interpolated amount. Based on the */
    /*  center of the glyph? That would screw up "h". Take the normal to the */
    /*  curve. Dot it into the y direction. Move the point by that fraction */
    /* Control points should be interpolated between the movements of their */
    /*  on-curve points */

    ccnt = MaxContourCount(ss_expanded);
    if ( ccnt==0 )
return(ss_expanded);			/* No points? Nothing to do */
    ptmoves = malloc((ccnt+1)*sizeof(struct ptmoves));
    for ( ss = ss_expanded; ss!=NULL ; ss=ss->next ) {
	if ( ss->first->prev==NULL )
    continue;
	cnt = PtMovesInitToContour(ptmoves,ss);
	for ( i=0; i<cnt; ++i ) {
	    int p = i==0?cnt-1:i-1, sign = 0;
	    sp = ptmoves[i].sp;
	    nsp = ptmoves[i+1].sp;
	    psp = ptmoves[p].sp;
	    if ( sp->me.y>=zones->top_zone )
		sign = -1;
	    else if ( sp->me.y<zones->bottom_zone )
		sign = 1;

	    /* Fix vertical serifs */
	    if ( zones->serif_height>0 &&
		     RealWithin(sp->me.y,zones->bottom_bound+zones->serif_height+zones->stroke_width/2,zones->serif_fuzz) ) {
		ptmoves[i].newpos.y = zones->bottom_bound+zones->serif_height;
	    } else if ( zones->serif_height>0 &&
		     RealWithin(sp->me.y,zones->top_bound-zones->serif_height-zones->stroke_width/2,zones->serif_fuzz) ) {
		ptmoves[i].newpos.y = zones->top_bound-zones->serif_height;
	    } else if ( sign ) {
		ptmoves[i].newpos.y += sign*zones->stroke_width*ptmoves[i].factor/2;
		/* This is to improve the looks of diagonal stems */
		if ( sp->next->islinear && sp->prev->islinear && nsp->next->islinear &&
			nsp->me.y == sp->me.y &&
			ptmoves[i].pdir.x*ptmoves[i+1].ndir.x + ptmoves[i].pdir.y*ptmoves[i+1].ndir.y<-.999 ) {
		    if ( ptmoves[i].pdir.y<0 )
			sign = -sign;
		    ptmoves[i].newpos.x += sign*zones->stroke_width*ptmoves[i].pdir.x;
		} else if ( sp->next->islinear && sp->prev->islinear && psp->next->islinear &&
			psp->me.y == sp->me.y &&
			ptmoves[i].ndir.x*ptmoves[p].pdir.x + ptmoves[i].ndir.y*ptmoves[p].pdir.y<-.999 ) {
		    if ( ptmoves[i].ndir.y<0 )
			sign = -sign;
		    ptmoves[i].newpos.x += sign*zones->stroke_width*ptmoves[i].ndir.x;
		}
	    }
	}
	InterpolateControlPointsAndSet(ptmoves,cnt);
    }
    free(ptmoves);

    if ( zones->counter_type == ct_retain || (sc->width!=0 && zones->counter_type == ct_auto))
	CorrectLeftSideBearing(ss_expanded,sc,layer);
return(ss_expanded);
}

static SplineSet *LCG_HintedEmboldenHook(SplineSet *ss_expanded,struct lcg_zones *zones,
	SplineChar *sc,int layer) {
    SplineSet *ss;
    SplinePoint *sp, *nsp, *psp, *origsp;
    int cnt, ccnt, i, n, p, j;
    struct ptmoves *ptmoves;
    StemInfo *h;
    double othery;
    /* Anything below the baseline moves up by width/2 */
    /* Anything that was on the base line moves up so that it still is on the baseline */
    /*  If it's a diagonal stem, may want to move in x as well as y */
    /* Anything on a hint one side of which is on the base line, then the other */
    /*  side moves up so it is hint->width + width above the baseline */

    /* Same for either x-height or cap-height, except that everything moves down */

    /* Any points on hints between baseline/x-height are fixed */

    /* Other points between baseline/x-height follow IUP rules */

    if ( layer!=ly_fore || sc->hstem==NULL )
return( LCG_EmboldenHook(ss_expanded,zones,sc,layer));

    FindStartPoint(ss_expanded,sc,layer);
    ccnt = MaxContourCount(ss_expanded);
    if ( ccnt==0 )
return(ss_expanded);			/* No points? Nothing to do */
    ptmoves = malloc((ccnt+1)*sizeof(struct ptmoves));
    for ( ss = ss_expanded; ss!=NULL ; ss=ss->next ) {
	if ( ss->first->prev==NULL )
    continue;
	cnt = PtMovesInitToContour(ptmoves,ss);
	/* Touch (usually move) all points which are either in our zones or on a hint */
	for ( i=0; i<cnt; ++i ) {
	    int sign = 0;
	    sp = ptmoves[i].sp;
	    origsp = FindMatchingPoint(sp->ptindex,sc->layers[layer].splines);
	    h = NULL; othery = 0;
	    if ( origsp!=NULL )
		h = OnHint(sc->hstem,origsp->me.y,&othery);
	    else
		h = MightBeOnHint(sc->hstem,zones,&ptmoves[i],&othery);

	    /* Fix vertical serifs */
	    if ( zones->serif_height>0 &&
		    (( origsp!=NULL && RealWithin(origsp->me.y, zones->bottom_bound+zones->serif_height,zones->serif_fuzz)) ||
		     RealWithin(sp->me.y,zones->bottom_bound+zones->serif_height+zones->stroke_width/2,zones->serif_fuzz)) ) {
		ptmoves[i].touched = true;
		ptmoves[i].newpos.y = zones->bottom_bound+zones->serif_height;
	    } else if ( zones->serif_height>0 &&
		    (( origsp!=NULL && RealWithin(origsp->me.y, zones->top_bound-zones->serif_height,zones->serif_fuzz)) ||
		     RealWithin(sp->me.y,zones->top_bound-zones->serif_height-zones->stroke_width/2,zones->serif_fuzz)) ) {
		ptmoves[i].touched = true;
		ptmoves[i].newpos.y = zones->top_bound-zones->serif_height;
	    } else if ( sp->me.y>=zones->top_bound || (h!=NULL && othery+zones->stroke_width/2>=zones->top_bound))
		sign = -1;
	    else if ( sp->me.y<=zones->bottom_bound || (h!=NULL && othery-zones->stroke_width/2<=zones->bottom_bound))
		sign = 1;
	    else if ( origsp!=NULL &&
			    (InHintAroundZone(sc->hstem,origsp->me.y,zones->top_bound) ||
			     InHintAroundZone(sc->hstem,origsp->me.y,zones->bottom_bound)) )
		/* It's not on the hint, so we want to interpolate it even if */
		/*  it's in an area we'd normally move (tahoma "s" has two */
		/*  points which fall on the baseline but aren't on the bottom */
		/*  hint */
		/* Do Nothing */;
	    else if ( h!=NULL &&
		    ((h->start>=zones->bottom_zone && h->start<=zones->top_zone) ||
		     (h->start+h->width>=zones->bottom_zone && h->start+h->width<=zones->top_zone)) ) {
		/* Point on a hint. In the middle of the glyph */
		/*  This one not in the zones, so it is fixed */
		ptmoves[i].touched = true;
	    }
	    if ( sign ) {
		ptmoves[i].touched = true;
		p = i==0?cnt-1:i-1;
		nsp = ptmoves[i+1].sp;
		psp = ptmoves[p].sp;
		if ( origsp!=NULL && ((origsp->me.y>=zones->bottom_bound-2 && origsp->me.y<=zones->bottom_bound+2 ) ||
			(origsp->me.y>=zones->top_bound-2 && origsp->me.y<=zones->top_bound+2 )) ) {
		    ptmoves[i].newpos.y += sign*zones->stroke_width*ptmoves[i].factor/2;
		    /* This is to improve the looks of diagonal stems */
		    if ( sp->next->islinear && sp->prev->islinear && nsp->next->islinear &&
			    nsp->me.y == sp->me.y &&
			    ptmoves[i].pdir.x*ptmoves[i+1].ndir.x + ptmoves[i].pdir.y*ptmoves[i+1].ndir.y<-.999 ) {
			if ( ptmoves[i].pdir.y<0 )
			    sign = -sign;
			ptmoves[i].newpos.x += sign*zones->stroke_width*ptmoves[i].pdir.x;
		    } else if ( sp->next->islinear && sp->prev->islinear && psp->next->islinear &&
			    psp->me.y == sp->me.y &&
			    ptmoves[i].ndir.x*ptmoves[p].pdir.x + ptmoves[i].ndir.y*ptmoves[p].pdir.y<-.999 ) {
			if ( ptmoves[i].ndir.y<0 )
			    sign = -sign;
			ptmoves[i].newpos.x += sign*zones->stroke_width*ptmoves[i].ndir.x;
		    }
		} else
		    ptmoves[i].newpos.y += sign*zones->stroke_width/2;
	    }
	}
	/* Now find each untouched point and interpolate how it moves */
	for ( i=0; i<cnt; ++i ) if ( !ptmoves[i].touched ) {
	    for ( p=i-1; p!=i; --p ) {
		if ( p<0 ) {
		    p=cnt-1;
		    if ( p==i )
	    break;
		}
		if ( ptmoves[p].touched )
	    break;
	    }
	    if ( p==i )
	break;			/* Nothing on the contour touched. Can't change anything */
	    for ( n=i+1; n!=i; ++n ) {
		if ( n>=cnt ) {
		    n=0;
		    if ( n==i )
	    break;
		}
		if ( ptmoves[n].touched )
	    break;
	    }
	    nsp = ptmoves[n].sp;
	    psp = ptmoves[p].sp;
	    for ( j=p+1; j!=n; ++j ) {
		if ( j==cnt ) {
		    j=0;
		    if ( n==0 )
	    break;
		}
		sp = ptmoves[j].sp;
		if (( sp->me.y>nsp->me.y && sp->me.y>psp->me.y ) ||
			(sp->me.y<nsp->me.y && sp->me.y<psp->me.y )) {
		    if (( sp->me.y>nsp->me.y && nsp->me.y>psp->me.y ) ||
			    (sp->me.y<nsp->me.y && nsp->me.y<psp->me.y )) {
			ptmoves[j].newpos.y += ptmoves[n].newpos.y-nsp->me.y;
			ptmoves[j].newpos.x += ptmoves[n].newpos.x-nsp->me.x ;
		    } else {
			ptmoves[j].newpos.y += ptmoves[p].newpos.y-psp->me.y;
			ptmoves[j].newpos.x += ptmoves[p].newpos.x-psp->me.x ;
		    }
		} else {
		    double diff;
		    diff = nsp->me.y - psp->me.y;
		    if ( diff!=0 ) {
			ptmoves[j].newpos.y += (ptmoves[p].newpos.y-psp->me.y)
						+ (sp->me.y-psp->me.y)*(ptmoves[n].newpos.y-nsp->me.y-(ptmoves[p].newpos.y-psp->me.y))/diff;
			/* Note we even interpolate the x direction depending on */
			/*  y position */
			ptmoves[j].newpos.x += (ptmoves[p].newpos.x-psp->me.x)
						+ (sp->me.y-psp->me.y)*(ptmoves[n].newpos.x-nsp->me.x-(ptmoves[p].newpos.x-psp->me.x))/diff;
		    } else if ( (diff = nsp->me.x - psp->me.x)!=0 ) {
			ptmoves[j].newpos.x += (ptmoves[p].newpos.x-psp->me.x)
						+ (sp->me.x-psp->me.x)*(ptmoves[n].newpos.x-nsp->me.x-(ptmoves[p].newpos.x-psp->me.x))/diff;
			/* Note we even interpolate the y direction depending on */
			/*  x position */
			ptmoves[j].newpos.y += (ptmoves[p].newpos.y-psp->me.y)
						+ (sp->me.x-psp->me.x)*(ptmoves[n].newpos.y-nsp->me.y-(ptmoves[p].newpos.y-psp->me.y))/diff;
		    }
		}
		ptmoves[j].touched = true;
		if ( isnan( ptmoves[j].newpos.y ))
		    IError("Nan value in LCG_HintedEmboldenHook\n" );
	    }
	}
	InterpolateControlPointsAndSet(ptmoves,cnt);
    }
    free(ptmoves);

    if ( zones->counter_type == ct_retain || (sc->width!=0 && zones->counter_type == ct_auto))
	CorrectLeftSideBearing(ss_expanded,sc,layer);
return( ss_expanded );
}

static void AdjustCounters(SplineChar *sc, struct lcg_zones *zones,
	DBounds *old, DBounds *new) {
    struct counterinfo ci;

    /* I did the left side bearing as I went along. I'll do the right side */
    /*  bearing now. I don't use condense/extend because I have more info */
    /*  here, and because I might not want to adjust both by the same amount */
    SCSynchronizeWidth(sc,sc->width+rint(zones->stroke_width),sc->width,NULL);
    /* Now do the internal counters. The Emboldening will (for vertical stems)*/
    /*  have made counters smaller by stroke_width (diagonal stems who knows) */
    /*  so make them bigger by that amount */
    memset(&ci,0,sizeof(ci));
    ci.bd = zones->bd;
    ci.stdvw = zones->stdvw;
    ci.top_y = zones->top_bound;
    ci.bottom_y = zones->bottom_bound;
    ci.boundry = (zones->top_bound+zones->bottom_bound)/2;
    ci.c_add = zones->stroke_width;
    ci.c_factor = ci.sb_factor = 100;
    StemInfosFree(sc->vstem); sc->vstem = NULL;
    SCCondenseExtend(&ci,sc,ly_fore,false);
}

static void SCEmbolden(SplineChar *sc, struct lcg_zones *zones, int layer) {
    StrokeInfo si;
    SplineSet *temp;
    DBounds old, new;
    int adjust_counters;

    memset(&si,0,sizeof(si));
    si.stroke_type = si_std;
    si.join = lj_miter;
    si.cap = lc_square;
    if ( zones->stroke_width>=0 ) {
	si.radius = zones->stroke_width/2.;
	si.removeinternal = true;
    } else {
	si.radius = -zones->stroke_width/2.;
	si.removeexternal = true;
    }
    /*si.removeoverlapifneeded = false;*/

    if ( layer!=ly_back && zones->wants_hints &&
	    sc->hstem == NULL && sc->vstem==NULL && sc->dstem==NULL ) {
	_SplineCharAutoHint(sc,layer==ly_all?ly_fore:layer,&zones->bd,NULL,false);
    }

    adjust_counters = zones->counter_type==ct_retain ||
	    (zones->counter_type==ct_auto &&
		zones->embolden_hook==LCG_HintedEmboldenHook &&
		sc->width>0 );

    if ( layer==ly_all ) {
	SCPreserveState(sc,false);
	SplineCharFindBounds(sc,&old);
	for ( layer = ly_fore; layer<sc->layer_cnt; ++layer ) {
	    NumberLayerPoints(sc->layers[layer].splines);
	    temp = BoldSSStroke(sc->layers[layer].splines,&si,sc->layers[layer].order2,zones->removeoverlap);
	    if ( zones->embolden_hook!=NULL )
		temp = (zones->embolden_hook)(temp,zones,sc,layer);
	    SplinePointListsFree( sc->layers[layer].splines );
	    sc->layers[layer].splines = temp;
	}
	SplineCharFindBounds(sc,&new);
	if ( adjust_counters )
	    AdjustCounters(sc,zones,&old,&new);
	layer = ly_all;
    } else if ( layer>=0 ) {
	SCPreserveLayer(sc,layer,false);
	NumberLayerPoints(sc->layers[layer].splines);
	SplineSetFindBounds(sc->layers[layer].splines,&old);
	temp = BoldSSStroke(sc->layers[layer].splines,&si,sc->layers[layer].order2,zones->removeoverlap);
	if ( zones->embolden_hook!=NULL )
	    temp = (zones->embolden_hook)(temp,zones,sc,layer);
	SplineSetFindBounds(temp,&new);
	SplinePointListsFree( sc->layers[layer].splines );
	sc->layers[layer].splines = temp;
	if ( adjust_counters && layer==ly_fore )
	    AdjustCounters(sc,zones,&old,&new);
    }

    if ( layer!=ly_back ) {
	/* Hints will be inccorrect (misleading) after these transformations */
	StemInfosFree(sc->vstem); sc->vstem=NULL;
	StemInfosFree(sc->hstem); sc->hstem=NULL;
	DStemInfosFree(sc->dstem); sc->dstem=NULL;
	SCOutOfDateBackground(sc);
    }
    SCCharChangedUpdate(sc,layer);
}

static struct {
    uint32 script;
    SplineSet *(*embolden_hook)(SplineSet *,struct lcg_zones *,SplineChar *, int layer);
} script_hooks[] = {
    { CHR('l','a','t','n'), LCG_HintedEmboldenHook },
    { CHR('c','y','r','l'), LCG_HintedEmboldenHook },
    { CHR('g','r','e','k'), LCG_HintedEmboldenHook },
	/* Hebrew probably works too */
    { CHR('h','e','b','r'), LCG_HintedEmboldenHook },
    { 0, NULL }
};

static struct {
    unichar_t from, to;
    SplineSet *(*embolden_hook)(SplineSet *,struct lcg_zones *,SplineChar *, int layer);
} char_hooks[] = {
    { '0','9', LCG_HintedEmboldenHook },
    { '$','%', LCG_HintedEmboldenHook },
    { '\0', '\0', NULL }
};

static void LCG_ZoneInit(SplineFont *sf, int layer, struct lcg_zones *zones,enum embolden_type type) {

    if ( type == embolden_lcg || type == embolden_custom) {
	zones->embolden_hook = LCG_HintedEmboldenHook;
    } else {
	zones->embolden_hook = NULL;
    }
    QuickBlues(sf, layer, &zones->bd);
    zones->stdvw = SFStdVW(sf);
}

static double BlueSearch(char *bluestring, double value, double bestvalue) {
    char *end;
    double try, diff, bestdiff;

    if ( *bluestring=='[' ) ++bluestring;
    if ( (bestdiff = bestvalue-value)<0 ) bestdiff = -bestdiff;

    for (;;) {
	try = strtod(bluestring,&end);
	if ( bluestring==end )
return( bestvalue );
	if ( (diff = try-value)<0 ) diff = -diff;
	if ( diff<bestdiff ) {
	    bestdiff = diff;
	    bestvalue = try;
	}
	bluestring = end;
	(void) strtod(bluestring,&end);		/* Skip the top of blue zone value */
	bluestring = end;
    }
}

static double SearchBlues(SplineFont *sf,int type,double value) {
    char *blues, *others;
    double bestvalue;

    if ( type=='x' )
	value = sf->ascent/2;		/* Guess that the x-height is about half the ascent and then see what we find */
    if ( type=='I' )
	value = 4*sf->ascent/5;		/* Guess that the cap-height is 4/5 the ascent */

    blues = others = NULL;
    if ( sf->private!=NULL ) {
	blues = PSDictHasEntry(sf->private,"BlueValues");
	others = PSDictHasEntry(sf->private,"OtherBlues");
    }
    bestvalue = 0x100000;		/* Random number outside coord range */
    if ( blues!=NULL )
	bestvalue = BlueSearch(blues,value,bestvalue);
    if ( others!=NULL )
	bestvalue = BlueSearch(others,value,bestvalue);
    if ( bestvalue == 0x100000 )
return( value );

return( bestvalue );
}

double SFSerifHeight(SplineFont *sf) {
    SplineChar *isc;
    SplineSet *ss;
    SplinePoint *sp;
    DBounds b;

    if ( sf->strokedfont || sf->multilayer )
return( 0 );

    isc = SFGetChar(sf,'I',NULL);
    if ( isc==NULL )
	isc = SFGetChar(sf,0x0399,"Iota");
    if ( isc==NULL )
	isc = SFGetChar(sf,0x0406,NULL);
    if ( isc==NULL )
return( 0 );

    ss = isc->layers[ly_fore].splines;
    if ( ss==NULL || ss->next!=NULL )		/* Too complicated, probably doesn't have simple serifs (black letter?) */
return( 0 );
    if ( ss->first->prev==NULL )
return( 0 );
    for ( sp=ss->first; ; ) {
	if ( sp->me.y==0 )
    break;
	sp = sp->next->to;
	if ( sp==ss->first )
    break;
    }
    if ( sp->me.y!=0 )
return( 0 );
    SplineCharFindBounds(isc,&b);
    if ( sp->next->to->me.y==0 || sp->next->to->next->to->me.y==0 ) {
	SplinePoint *psp = sp->prev->from;
	if ( psp->me.y>=b.maxy/3 )
return( 0 );			/* Sans Serif, probably */
	if ( !psp->nonextcp && psp->nextcp.x==psp->me.x ) {
	    /* A curve point half-way up the serif? */
	    psp = psp->prev->from;
	    if ( psp->me.y>=b.maxy/3 )
return( 0 );			/* I give up, I don't understand this */
	}
return( psp->me.y );
    } else if ( sp->prev->from->me.y==0 || sp->prev->from->prev->from->me.y==0 ) {
	SplinePoint *nsp = sp->next->to;
	if ( nsp->me.y>=b.maxy/3 )
return( 0 );			/* Sans Serif, probably */
	if ( !nsp->nonextcp && nsp->nextcp.x==nsp->me.x ) {
	    /* A curve point half-way up the serif? */
	    nsp = nsp->next->to;
	    if ( nsp->me.y>=b.maxy/3 )
return( 0 );			/* I give up, I don't understand this */
	}
return( nsp->me.y );
    }

    /* Too complex for me */
return( 0 );
}

static void PerGlyphInit(SplineChar *sc, struct lcg_zones *zones,
	enum embolden_type type) {
    int j;
    SplineChar *hebrew;

    if ( type == embolden_auto ) {
	zones->embolden_hook = NULL;
	for ( j=0; char_hooks[j].from!=0; ++j ) {
	    if ( sc->unicodeenc>=char_hooks[j].from && sc->unicodeenc<=char_hooks[j].to ) {
		zones->embolden_hook = char_hooks[j].embolden_hook;
	break;
	    }
	}
	if ( zones->embolden_hook == NULL ) {
	    uint32 script = SCScriptFromUnicode(sc);
	    for ( j=0; script_hooks[j].script!=0; ++j ) {
		if ( script==script_hooks[j].script ) {
		    zones->embolden_hook = script_hooks[j].embolden_hook;
	    break;
		}
	    }
	}
    }
    if ( type == embolden_lcg || type == embolden_auto ) {
	zones->bottom_bound = 0;
	if ( SCScriptFromUnicode(sc)==CHR('h','e','b','r') &&
		(hebrew=SFGetChar(sc->parent,0x05df,NULL))!=NULL ) {
	    DBounds b;
	    SplineCharFindBounds(hebrew,&b);
	    zones->bottom_zone = b.maxy/3;
	    zones->top_zone = 2*b.maxy/3;
	    zones->top_bound = b.maxy;
	} else if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 && islower(sc->unicodeenc)) {
	    if ( zones->bd.xheight<=0 )
		zones->bd.xheight = SearchBlues(sc->parent,'x',0);
	    zones->bottom_zone = zones->bd.xheight>0 ? zones->bd.xheight/3 :
			    zones->bd.caph>0 ? zones->bd.caph/3 :
			    (sc->parent->ascent/4);
	    zones->top_zone = zones->bd.xheight>0 ? 2*zones->bd.xheight/3 :
			    zones->bd.caph>0 ? zones->bd.caph/2 :
			    (sc->parent->ascent/3);
	    zones->top_bound = zones->bd.xheight>0 ? zones->bd.xheight :
			    zones->bd.caph>0 ? 2*zones->bd.caph/3 :
			    (sc->parent->ascent/2);
	} else if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 && isupper(sc->unicodeenc)) {
	    if ( zones->bd.caph<0 )
		zones->bd.caph = SearchBlues(sc->parent,'I',0);
	    zones->bottom_zone = zones->bd.caph>0 ? zones->bd.caph/3 :
			    (sc->parent->ascent/4);
	    zones->top_zone = zones->bd.caph>0 ? 2*zones->bd.caph/3 :
			    (sc->parent->ascent/2);
	    zones->top_bound = zones->bd.caph>0?zones->bd.caph:4*sc->parent->ascent/5;
	} else {
	    /* It's not upper case. It's not lower case. Hmm. Look for blue */
	    /*  values near the top and bottom of the glyph */
	    DBounds b;

	    SplineCharFindBounds(sc,&b);
	    zones->top_bound = SearchBlues(sc->parent,0,b.maxy);
	    zones->bottom_bound = SearchBlues(sc->parent,-1,b.miny);
	    zones->top_zone = zones->bottom_bound + 3*(zones->top_bound-zones->bottom_bound)/4;
	    zones->bottom_zone = zones->bottom_bound + (zones->top_bound-zones->bottom_bound)/4;
	}
    }
    zones->wants_hints = zones->embolden_hook == LCG_HintedEmboldenHook;
}

void FVEmbolden(FontViewBase *fv,enum embolden_type type,struct lcg_zones *zones) {
    int i, gid, cnt;
    SplineChar *sc;

    LCG_ZoneInit(fv->sf,fv->active_layer,zones,type);

    for (i=0, cnt=0; i < fv->map->enccount; ++i) {
        if (fv->selected[i] && (gid = fv->map->map[i]) != -1 &&
            (sc=fv->sf->glyphs[gid]) != NULL) {

            cnt++;
        }
    }

    ff_progress_start_indicator(10, _("Change Weight"),
        _("Changing glyph weights"), NULL, cnt, 1);

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL ) {
	PerGlyphInit(sc,zones,type);
	SCEmbolden(sc, zones, -2);		/* -2 => all foreground layers */
    if (!ff_progress_next()) {
        break;
    }
    }
    ff_progress_end_indicator();
}

void CVEmbolden(CharViewBase *cv,enum embolden_type type,struct lcg_zones *zones) {
    SplineChar *sc = cv->sc;

    if ( cv->drawmode == dm_grid )
return;

    LCG_ZoneInit(sc->parent,CVLayer(cv),zones,type);

    PerGlyphInit(sc,zones,type);
    SCEmbolden(sc, zones, CVLayer(cv));
}

void ScriptSCEmbolden(SplineChar *sc,int layer,enum embolden_type type,struct lcg_zones *zones) {

    LCG_ZoneInit(sc->parent,layer,zones,type);

    PerGlyphInit(sc,zones,type);
    SCEmbolden(sc, zones, layer);
}

/* ************************************************************************** */
/* ********************************* Italic ********************************* */
/* ************************************************************************** */

static void FigureGoodStems(StemInfo *hints) {
    StemInfo *h, *test, *best;
    double max, best_len;

    for ( h=hints; h!=NULL; ) {
	h->tobeused = false;
	if ( 2*HIlen(h)<h->width ) {
	    h = h->next;
    continue;
	}
	if ( !h->hasconflicts ) {
	    h->tobeused = true;
	    h = h->next;
	} else {
	    max = h->start+h->width;
	    best = h;
	    best_len = HIlen(h);
	    for ( test=h->next; test!=NULL; test=test->next ) {
		if ( test->start>max )
	    break;
		if ( test->start+test->width>max )
		    max = test->start + test->width;
		if ( HIlen(test)>best_len ) {
		    best = test;
		    best_len = HIlen(test);
		}
	    }
	    best->tobeused = true;
	    h = test;
	}
    }
}

enum { cs_lc, cs_uc, cs_smallcaps, cs_neither };
static int FigureCase(SplineChar *sc) {
    char *under, *dot, ch;
    int uni;
    int smallcaps = false;

    if ( sc->unicodeenc<0x10000 && sc->unicodeenc!=-1 )
return( islower(sc->unicodeenc) ? cs_lc : isupper(sc->unicodeenc) ? cs_uc : cs_neither );
    else if ( sc->unicodeenc!=-1 )
return( cs_neither );

    under = strchr(sc->name,'_');
    dot   = strchr(sc->name,'.');
    if ( dot!=NULL )
	smallcaps = strcmp(dot,".sc")==0 || strcmp(dot,".small");
    if ( under!=NULL && (dot==NULL || dot>under))
	dot = under;
    if ( dot==NULL )
return( cs_neither );
    ch = *dot; *dot = '\0';
    uni = UniFromName(sc->name,ui_none,&custom);
    *dot = ch;
    if ( uni==-1 || uni>=0x10000 )
return( cs_neither );

    if ( smallcaps && (islower(uni) || isupper(uni)) )
return( cs_smallcaps );

return( islower(uni) ? cs_lc : isupper(uni) ? cs_uc : cs_neither );
}

static int LikeAnF(SplineChar *sc) {
    char *under, *start;
    int cnt;

    if ( sc->unicodeenc=='f' || sc->unicodeenc==0x17f/*longs*/ ||
	    sc->unicodeenc==0xfb /* longs-s ligature (es-zet) */ ||
	    sc->unicodeenc==0xfb01 || sc->unicodeenc==0xfb02 ||	/* fi, fl */
	    sc->unicodeenc==0xfb05 /*longs_t */ )
return( true );
    if ( sc->unicodeenc==0xfb00 || sc->unicodeenc==0xfb03 || sc->unicodeenc==0xfb04 )
return( 2 );	/* ff, ffi, ffl */

    cnt = 0;
    for ( start = sc->name; (under= strchr(start,'_'))!=NULL; ) {
	if ( *start=='f' && under-start==1 )
	    ++cnt;		/* f ligature */
	else if ( under-start==5 && strncmp(start,"longs",5)==0 )
	    ++cnt;		/* longs ligature */
	else
return( cnt );
	start = under+1;
    }
    if ( *start=='f' && start[1]=='\0' )
	++cnt;
    else if ( strcmp(start,"longs")==0 )
	++cnt;
return( cnt );
}

static int LikePQ(SplineChar *sc) {
    int i;

    for ( i=0; descender_str[i]!=0; ++i )
	if ( sc->unicodeenc == descender_str[i] )
return( true );

return( false );
}

static void ItalicCompress(SplineChar *sc,int layer,struct hsquash *squash) {
    DBounds pre, mid, post;
    real stemsquash[6], translate[6];
    double counter_len;
    int tot;

    SplineCharLayerFindBounds(sc,layer,&pre);
    if ( squash->stem_percent!=1 ) {
	memset(stemsquash,0,sizeof(stemsquash));
	stemsquash[0] = squash->stem_percent;
	stemsquash[3] = 1;
	SplinePointListTransform(sc->layers[layer].splines,stemsquash,tpt_AllPoints);
    }
    if ( !RealNear(squash->stem_percent,squash->counter_percent)) {
	SplineCharAutoHint(sc,layer,NULL);

	free( SCFindHintOverlaps(sc->vstem, pre.minx*squash->stem_percent,
		pre.maxx*squash->stem_percent, &tot, &counter_len ));
	if ( counter_len>0 ) {
	    SplineCharLayerFindBounds(sc,layer,&mid);
	    SmallCapsRemoveSpace(sc->layers[layer].splines,sc->anchor,sc->vstem,0,
		    counter_len*(1-squash->counter_percent/squash->stem_percent),
		    mid.minx,mid.maxx);
	    SplineSetRefigure(sc->layers[layer].splines);
	}
    }
    if ( sc->layers[layer].refs==NULL ) {
	SplineCharLayerFindBounds(sc,layer,&post);
	memset(translate,0,sizeof(translate));
	if ( (pre.minx==0 && post.minx!=0 ) ||
		(pre.minx!=0 && !RealNear(post.minx/pre.minx,squash->lsb_percent))) {
	    translate[0] = translate[3] = 1;
	    translate[4] = pre.minx==0 ? -post.minx : pre.minx*squash->lsb_percent - post.minx;
	    SplinePointListTransform(sc->layers[layer].splines,translate,tpt_AllPoints);
	}
	sc->width = (sc->width-pre.maxx)*squash->rsb_percent + post.maxx + translate[4];
    }
}

enum pt_type { pt_oncurve, pt_offcurve, pt_end };

struct italicserifdata {
    int emsize;
    double stemwidth;
    double xheight;
    double ia;
    double stem_angle;
    struct { double x,y; enum pt_type type; } points[27];
};
static struct italicserifdata normalitalicserif =	/* faces right */
    { 1000, 84, 450, -15.2, 90, /* vertical */{
	{ 84,     116, pt_oncurve },
	{ 84,     97,  pt_offcurve },
	{ 85.19,  61,  pt_offcurve },
	{ 86.07,  51,  pt_oncurve },
	{ 88.09,  44,  pt_offcurve },
	{ 98.86,  36,  pt_offcurve },
	{ 107.34, 36,  pt_oncurve },
	{ 116.87, 36,  pt_offcurve },
	{ 133.42, 46,  pt_offcurve },
	{ 143,    63,  pt_oncurve },
	{ 171.75, 114, pt_oncurve },
	{ 188.68, 103, pt_oncurve },
	{ 152.48, 19,  pt_offcurve },
	{ 121.92, -11, pt_offcurve },
	{ 73.20,  -11, pt_oncurve },
	{ 14.64,  -11, pt_offcurve },
	{ 0,      47,  pt_offcurve },
	{ 0,      123, pt_oncurve },
	{ 0, 0, pt_end }} };
static struct italicserifdata bolditalicserif =
    { 1000, 139, 450, -15.2, 90, /* vertical */{
	{ 139,    128, pt_oncurve },
	{ 139,    116, pt_offcurve },
	{ 143.21, 82,  pt_offcurve },
	{ 145.17, 76,  pt_oncurve },
	{ 147.78, 68,  pt_offcurve },
	{ 158.48, 61,  pt_offcurve },
	{ 168.09, 61,  pt_oncurve },
	{ 186.12, 61,  pt_offcurve },
	{ 205.36, 82,  pt_offcurve },
	{ 225.43, 121, pt_oncurve },
	{ 235.72, 141, pt_oncurve },
	{ 266.74, 127, pt_oncurve },
	{ 216.89, 22,  pt_offcurve },
	{ 182.54, -9,  pt_offcurve },
	{ 110.42, -9,  pt_oncurve },
	{ 0.74,   -9,  pt_offcurve },
	{ 0,      80,  pt_offcurve },
	{ 0,      123, pt_oncurve },
	{ 0, 0, pt_end }} };
static struct italicserifdata leftfacingitalicserif =	/* bottom left of "x", and top right */
    { 1000, 34, 450, -15.2, 90, /* vertical */{
	{   25,    111, pt_oncurve },
	{   25,    91, pt_offcurve },
	{   19,     75, pt_offcurve },
	{   10,     59, pt_offcurve },
	{  -24,      1, pt_offcurve },
	{  -33,    -11, pt_offcurve },
	{  -64,    -11, pt_oncurve },
	{  -94,    -11, pt_offcurve },
	{ -118,      5, pt_offcurve },
	{ -125,     31, pt_oncurve },
	{ -130,     51, pt_offcurve },
	{ -120,     66, pt_offcurve },
	{ -101,     66, pt_oncurve },
	{  -93,     66, pt_offcurve },
	{  -81,     63, pt_offcurve },
	{  -66,     56, pt_oncurve },
	{  -54,     50, pt_offcurve },
	{  -43,     47, pt_offcurve },
	{  -37,     47, pt_oncurve },
	{  -28,     47, pt_offcurve },
	{  -18,     56, pt_offcurve },
	{   -8,     76, pt_oncurve },
	{   -2,     89, pt_offcurve },
	{    0,     98, pt_offcurve },
	{    0,    111, pt_oncurve },
	{ 0, 0, pt_end }} };
static struct italicserifdata boldleftfacingitalicserif =	/* bottom left of "x", and top right */
    { 1000, 60, 450, -15.2, 90, /* vertical */{
	{   39,    176, pt_oncurve },
	{   39,    159, pt_offcurve },
	{   33,    131, pt_offcurve },
	{   24,    103, pt_offcurve },
	{   -8,      7, pt_offcurve },
	{  -25,    -13, pt_offcurve },
	{  -75,    -13, pt_oncurve },
	{ -113,    -13, pt_offcurve },
	{ -146,     11, pt_offcurve },
	{ -156,     46, pt_oncurve },
	{ -164,     77, pt_offcurve },
	{ -147,    102, pt_offcurve },
	{ -116,    102, pt_oncurve },
	{ -104,    102, pt_offcurve },
	{  -91,     98, pt_offcurve },
	{  -72,     89, pt_oncurve },
	{  -61,     83, pt_offcurve },
	{  -55,     81, pt_offcurve },
	{  -49,     81, pt_oncurve },
	{  -32,     81, pt_offcurve },
	{  -23,     90, pt_offcurve },
	{  -12,    122, pt_oncurve },
	{   -4,    144, pt_offcurve },
	{    0,    162, pt_offcurve },
	{    0,    180, pt_oncurve },
	{ 0, 0, pt_end }} };
static struct italicserifdata *normalserifs[] = { &normalitalicserif, &leftfacingitalicserif, &leftfacingitalicserif };
static struct italicserifdata *boldserifs[] = { &bolditalicserif, &boldleftfacingitalicserif, &boldleftfacingitalicserif };

static void InterpBp(BasePoint *bp, int index, double xscale, double yscale,
	double interp, double endx, struct italicserifdata *normal,struct italicserifdata  *bold) {
    bp->x = xscale * ((1-interp)*normal->points[index].x + interp*bold->points[index].x) + endx;
    bp->y = yscale * ((1-interp)*normal->points[index].y + interp*bold->points[index].y);
}

static SplineSet *MakeBottomItalicSerif(double stemwidth,double endx,
	ItalicInfo *ii, int seriftype) {
    double xscale, yscale, interp;
    BasePoint bp;
    int i;
    SplinePoint *last, *cur;
    SplineSet *ss;
    struct italicserifdata *normal, *bold;

    normal = normalserifs[seriftype];
    bold   =   boldserifs[seriftype];

    if ( stemwidth<0 )
	stemwidth = -stemwidth;

    xscale = ii->emsize/1000.0;
    interp = (stemwidth/xscale - normal->stemwidth)/
	    (bold->stemwidth-normal->stemwidth);
    yscale = ii->x_height/normal->xheight;

    ss = chunkalloc(sizeof(SplineSet));
    i=0;
    InterpBp(&bp,i,xscale,yscale,interp,endx,normal,bold);
    ss->first = last = SplinePointCreate(bp.x,bp.y);
    for ( ++i; normal->points[i].type!=pt_end; ) {
	if ( normal->points[i].type==pt_oncurve ) {
	    InterpBp(&bp,i,xscale,yscale,interp,endx,normal,bold);
	    cur = SplinePointCreate(bp.x,bp.y);
	    SplineMake3(last,cur);
	    ++i;
	} else {
	    InterpBp(&last->nextcp,i,xscale,yscale,interp,endx,normal,bold);
	    last->nonextcp = false;
	    i+=2;
	    InterpBp(&bp,i,xscale,yscale,interp,endx,normal,bold);
	    cur = SplinePointCreate(bp.x,bp.y);
	    InterpBp(&cur->prevcp,i-1,xscale,yscale,interp,endx,normal,bold);
	    cur->noprevcp = false;
	    SplineMake3(last,cur);
	    ++i;
	}
	last = cur;
    }
    ss->last = last;
    if ( ii->order2 ) {
	SplineSet *newss;
	SplineSetsRound2Int(ss,1.0,false,false);
	newss = SSttfApprox(ss);
	SplinePointListFree(ss);
	ss = newss;
    } else {
	SPLCategorizePoints(ss);
    }
    { double temp;
	if ( (temp = ss->first->me.x-ss->last->me.x)<0 ) temp = -temp;
	if ( seriftype==0 && !RealWithin(temp,stemwidth,.1))
	    IError( "Stem width doesn't match serif" );
    }
return( ss );
}

static SplineSet *MakeTopItalicSerif(double stemwidth,double endx,
	ItalicInfo *ii,int at_xh) {
    SplineSet *ss = MakeBottomItalicSerif(stemwidth,0,ii,0);
    real trans[6];

    memset(trans,0,sizeof(trans));
    trans[0] = trans[3] = -1;
    trans[4] = endx; trans[5] = at_xh ? ii->x_height : ii->ascender_height;
return( SplinePointListTransform(ss,trans,tpt_AllPoints));
}

static SplineSet *MakeItalicDSerif(DStemInfo *d,double stemwidth,
	double endx, ItalicInfo *ii,int seriftype, int top) {
    SplineSet *ss;
    real trans[6];
    int i;
    double spos, epos, dpos;
    extended t1, t2;
    SplinePoint *sp;
    Spline *s;
    double cur_sw;
    int order2 = ii->order2;

    ii->order2 = false;		/* Don't convert to order2 untill after */
    ss = MakeBottomItalicSerif(stemwidth,0,ii,seriftype);
    ii->order2 = order2;	/* We finish messing with the serif */

    memset(trans,0,sizeof(trans));

    if ( top ) {
	trans[0] = trans[3] = -1;
	trans[4] = endx; trans[5] = top==1 ? ii->x_height : ii->ascender_height;
	SplinePointListTransform(ss,trans,tpt_AllPoints);
    }
    /* given the orientation of the dstem, do we need to flip the serif? */
    if ( (seriftype==0 && d->unit.x*d->unit.y>0) || (seriftype!=0 && d->unit.x*d->unit.y<0)) {
	trans[0] = -1; trans[3] = 1;
	trans[4] = ss->first->me.x+endx;
	trans[5] = 0;
	SplinePointListTransform(ss,trans,tpt_AllPoints);
	SplineSetReverse(ss);
    }

    /* Now we want to find the places near the start and end of the serif */
    /*  where it is parallel to the stem. We want to truncate the serif   */
    /*  at those points, so that the serif now starts and ends parallel to*/
    /*  the stem. */
    /* How to do that? rotate it parallel to the stem and then look for */
    /*  min/max points */
    trans[0] = trans[3] = d->unit.x;
    trans[2] = -(trans[1] = -d->unit.y);
    trans[4] = trans[5] = 0;
    SplinePointListTransform(ss,trans,tpt_AllPoints);

    /* Now the min/max point will probably be on the first spline, but might */
    /*  be on the second (and will probably be on the last spline, but might */
    /*  be on the penultimate) */
    s = ss->first->next;
    for ( i=0; i<2; ++i ) {
	SplineFindExtrema(&s->splines[1],&t1,&t2);
	if ( t1>=0 && t1<=1 ) {
	    if ( s!=ss->first->next ) {
		SplineFree(ss->first->next);
		SplinePointFree(ss->first);
		ss->first = s->from;
		ss->start_offset = 0;
	    }
	    if ( t1>=.999 ) {
		SplinePointFree(ss->first);
		ss->first = s->to;
		ss->start_offset = 0;
		SplineFree(ss->first->prev);
		ss->first->prev = NULL;
	    } else if ( t1>.001 ) {
		sp = SplineBisect(s,t1);
		SplinePointFree(ss->first);
		SplineFree(sp->prev);
		sp->prev = NULL;
		ss->first = sp;
		ss->start_offset = 0;
	    }
    break;
	}
	s=s->to->next;
    }
    s = ss->last->prev;
    for ( i=0; i<2; ++i ) {
	SplineFindExtrema(&s->splines[1],&t1,&t2);
	if ( t1>=0 && t1<=1 ) {
	    if ( s!=ss->last->prev ) {
		SplineFree(ss->last->prev);
		SplinePointFree(ss->last);
		ss->last = s->to;
	    }
	    if ( t1<=.001 ) {
		SplinePointFree(ss->last);
		ss->last = s->from;
		SplineFree(ss->last->next);
		ss->last->next = NULL;
	    } else if ( t1<.999 ) {
		sp = SplineBisect(s,t1);
		SplinePointFree(ss->last);
		SplineFree(sp->next);
		sp->next = NULL;
		ss->last = sp;
	    }
    break;
	}
	s = s->from->prev;
    }
    /* Now rotate it back to the correct orientation */
    trans[2] = -trans[2]; trans[1] = -trans[1];
    SplinePointListTransform(ss,trans,tpt_AllPoints);

    /* Now it probably doesn't have the correct stemwidth */
    cur_sw = (ss->first->me.x-ss->last->me.x)*d->unit.y - (ss->first->me.y-ss->last->me.y)*d->unit.x;
    if ( cur_sw<0 ) cur_sw = -cur_sw;
    if ( cur_sw!=stemwidth ) {
	double diff3 = (cur_sw-stemwidth)/3;
	if ( ss->first->me.x*d->unit.y-ss->first->me.y*d->unit.x <
		ss->last->me.x*d->unit.y-ss->last->me.y*d->unit.x )
	    diff3 = -diff3;
	ss->first->me.x -= diff3*d->unit.y;
	ss->first->me.y -= -diff3*d->unit.x;
	ss->first->nextcp.x -= diff3*d->unit.y;
	ss->first->nextcp.y -= -diff3*d->unit.x;
	SplineRefigure(ss->first->next);
	ss->last->me.x += 2*diff3*d->unit.y;
	ss->last->me.y += -2*diff3*d->unit.x;
	ss->last->prevcp.x += 2*diff3*d->unit.y;
	ss->last->prevcp.y += -2*diff3*d->unit.x;
	SplineRefigure(ss->last->prev);
    }

    /* Finally, position so that the serif lies on the dstem. We've already */
    /*  given it the correct height, so all we can adjust is the x value */
    /* (We played with the x-value earlier, but that only worked if we didn't */
    /*  need to truncate anything) */
    memset(trans,0,sizeof(trans));
    trans[0] = trans[3] = 1;

    spos = d->left .x + (ss->first->me.y-d->left .y)*d->unit.x/d->unit.y;
    epos = d->right.x + (ss->first->me.y-d->right.y)*d->unit.x/d->unit.y;
    dpos = ss->first->me.x + (ss->last->me.y-ss->first->me.y)*d->unit.x/d->unit.y;
    if ( dpos>ss->last->me.x ) {
	/* ss->last is to the left of ss->first */
	/* move ss->first to the rightmost edge of the dstem */
	if ( spos>epos )
	    trans[4] = spos-ss->first->me.x;
	else
	    trans[4] = epos-ss->first->me.x;
    } else {
	if ( spos<epos )
	    trans[4] = spos-ss->first->me.x;
	else
	    trans[4] = epos-ss->first->me.x;
    }
    SplinePointListTransform(ss,trans,tpt_AllPoints);
    if ( ii->order2 ) {
	SplineSet *newss;
	SplineSetsRound2Int(ss,1.0,false,false);
	newss = SSttfApprox(ss);
	SplinePointListFree(ss);
	ss = newss;
    } else {
	SPLCategorizePoints(ss);
    }
return( ss );
}

static int InHintRange(HintInstance *hi, double pos) {

    for ( ; hi!=NULL; hi=hi->next ) {
	if ( pos>=hi->begin && pos<=hi->end )
return( true );
    }
return( false );
}

static double ValidBottomSerif(SplinePoint *start,SplinePoint *end,
	double depth, double fuzz, double minbound, double maxbound ) {
    double max = start->me.y>end->me.y ? start->me.y : end->me.y;
    SplinePoint *sp, *last;
    int got_down = false, got_up=false;
    /* To be a serif it must go down about as far down as we expect       */
    /*  (baseline, or the bottom of descenders), it can bounce around     */
    /*  a little down there, but it can't go too far down. Once it starts */
    /*  going significantly up, it must continue going up until the end.  */

    if ( start==end )
return( false );

    last = NULL;
    for ( sp=start ; ; ) {
	if ( sp->me.x<minbound || sp->me.x>maxbound )
return( false );
	if ( sp->me.y > max+fuzz )
return( false );
	if ( sp->me.y < depth-fuzz )
return( false );
	if ( sp->me.y < depth+fuzz/2+1 )
	    got_down = true;
	else if ( got_down && sp->me.y > depth+fuzz/2 )
	    got_up = true;
	if ( last!=NULL ) {
	    if ( !got_down && sp->me.y>last->me.y+fuzz/10 )
return( false );
	    else if ( got_up && sp->me.y<last->me.y-fuzz/10 )
return( false );
	}
	last = sp;
	if ( sp==end )
return( got_down );
	if ( sp->next==NULL )
return( false );
	sp = sp->next->to;
    }
}

static void FindBottomSerifOnStem(SplineChar *sc,int layer,StemInfo *h,
	double depth, ItalicInfo *ii,
	SplinePoint **_start,SplinePoint **_end, SplineSet **_ss) {
    SplinePoint *start=NULL, *end=NULL, *sp;
    SplinePointList *ss;
    double sdiff, ediff;
    double fuzz = (sc->parent->ascent+sc->parent->descent)/100.0;

    for ( ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	start=end=NULL;
	for ( sp=ss->first; ; ) {
	    if ( ( sdiff = sp->me.x-h->start)<0 ) sdiff = -sdiff;
	    if ( ( ediff = sp->me.x-(h->start+h->width))<0 ) ediff = -ediff;
	    if ( sdiff<=3 && ( start==NULL ||
		    ( sp->me.y<start->me.y && (InHintRange(h->where,sp->me.y) || !InHintRange(h->where,start->me.y))))) {
		start=sp;
	    } else if ( ediff<=3 && ( end==NULL ||
		    ( sp->me.y<end->me.y && (InHintRange(h->where,sp->me.y) || !InHintRange(h->where,end->me.y)) ))) {
		end=sp;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
	if ( sp->next==NULL )
    continue;
	if ( start!=NULL && end!=NULL ) {
	    if ( ValidBottomSerif(start,end,depth,fuzz,h->start-ii->serif_extent-fuzz,h->start+h->width+ii->serif_extent+fuzz))
    break;
	    else if ( ValidBottomSerif(end,start,depth,fuzz,h->start-ii->serif_extent-fuzz,h->start+h->width+ii->serif_extent+fuzz)) {
		SplinePoint *temp = start;
		start = end;
		end = temp;
    break;
	    } else
		start = NULL;
	}
    }

    if ( start==NULL || end==NULL )
	start = end = NULL;
    *_start = start;
    *_end   = end;
    *_ss    = ss;
}

static double ValidBottomDSerif(SplinePoint *start,SplinePoint *end,
	double depth, double fuzz, ItalicInfo *ii, DStemInfo *d ) {
    double max = start->me.y>end->me.y ? start->me.y : end->me.y;
    SplinePoint *sp, *last;
    int got_down = false, got_up=false;
    /* To be a serif it must go down about as far down as we expect       */
    /*  (baseline, or the bottom of descenders), it can bounce around     */
    /*  a little down there, but it can't go too far down. Once it starts */
    /*  going significantly up, it must continue going up until the end.  */
    double dlpos, drpos;

    if ( start==end )
return( false );

    last = NULL;
    for ( sp=start ; ; ) {
	dlpos = (sp->me.x-d->left .x)*d->unit.y - (sp->me.y-d->left .y)*d->unit.x;
	drpos = (sp->me.x-d->right.x)*d->unit.y - (sp->me.y-d->right.y)*d->unit.x;
	if ( dlpos< -1.5*ii->serif_extent-fuzz || drpos > 1.5*ii->serif_extent+fuzz )
return( false );
	if ( sp->me.y > max+fuzz )
return( false );
	if ( sp->me.y < depth-fuzz )
return( false );
	if ( sp->me.y < depth+fuzz/2+1 )
	    got_down = true;
	else if ( got_down && sp->me.y > depth+fuzz/2 )
	    got_up = true;
	if ( last!=NULL ) {
	    if ( !got_down && sp->me.y>last->me.y+fuzz/10 )
return( false );
	    else if ( got_up && sp->me.y<last->me.y-fuzz/10 )
return( false );
	}
	last = sp;
	if ( sp==end )
return( got_down );
	if ( sp->next==NULL )
return( false );
	sp = sp->next->to;
    }
}

static double ValidTopDSerif(SplinePoint *start,SplinePoint *end,
	double height, double fuzz, ItalicInfo *ii, DStemInfo *d ) {
    double min = start->me.y<end->me.y ? start->me.y : end->me.y;
    SplinePoint *sp, *last;
    int got_down = false, got_up=false;
    double dlpos, drpos;

    if ( start==end )
return( false );

    last = NULL;
    for ( sp=start ; ; ) {
	dlpos = (sp->me.x-d->left .x)*d->unit.y - (sp->me.y-d->left .y)*d->unit.x;
	drpos = (sp->me.x-d->right.x)*d->unit.y - (sp->me.y-d->right.y)*d->unit.x;
	if ( dlpos< -1.5*ii->serif_extent-fuzz || drpos > 1.5*ii->serif_extent+fuzz )
return( false );
	if ( sp->me.y < min-fuzz )
return( false );
	if ( sp->me.y > height+2*fuzz )
return( false );
	if ( sp->me.y > height-fuzz/2 )
	    got_up = true;
	else if ( got_up && sp->me.y < height-fuzz/2-1 )
	    got_down = true;
	if ( last!=NULL ) {
	    if ( !got_up && sp->me.y<last->me.y-fuzz/2 )
return( false );
	    else if ( got_down && sp->me.y>last->me.y+fuzz/2 )
return( false );
	}
	last = sp;
	if ( sp==end )
return( got_up );
	if ( sp->next==NULL )
return( false );
	sp = sp->next->to;
    }
}

static int RoughlyParallel(SplinePoint *sp,BasePoint *unit) {
    BasePoint diff;
    double len, off;

    if ( sp->nonextcp && sp->next!=NULL ) {
	diff.x = sp->next->to->me.x - sp->me.x;
	diff.y = sp->next->to->me.y - sp->me.y;
    } else {
	diff.x = sp->nextcp.x - sp->me.x;
	diff.y = sp->nextcp.y - sp->me.y;
    }
    len = sqrt(diff.x*diff.x + diff.y*diff.y);
    if ( len!=0 ) {
	if ( (off = (diff.x*unit->y - diff.y*unit->x)/len) < 0 ) off = -off;
	if ( off<.04 )
return( true );
    }

    if ( sp->noprevcp && sp->prev!=NULL ) {
	diff.x = sp->prev->from->me.x - sp->me.x;
	diff.y = sp->prev->from->me.y - sp->me.y;
    } else {
	diff.x = sp->prevcp.x - sp->me.x;
	diff.y = sp->prevcp.y - sp->me.y;
    }
    len = sqrt(diff.x*diff.x + diff.y*diff.y);
    if ( len!=0 ) {
	if ( (off = (diff.x*unit->y - diff.y*unit->x)/len) < 0 ) off = -off;
	if ( off<.04 )
return( true );
    }
return( false );
}

static void FindBottomSerifOnDStem(SplineChar *sc,int layer,DStemInfo *d,
	double depth, ItalicInfo *ii,
	SplinePoint **_start,SplinePoint **_end, SplineSet **_ss) {
    SplinePoint *start=NULL, *end=NULL, *sp;
    SplinePointList *ss;
    double sdiff, ediff;
    double pos;
    double fuzz = (sc->parent->ascent+sc->parent->descent)/100.0;

    for ( ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	start=end=NULL;
	for ( sp=ss->first; ; ) {
	    if ( RoughlyParallel(sp,&d->unit)) {
		pos = (sp->me.x-d->left.x)*d->unit.x + (sp->me.y-d->left.y)*d->unit.y;
		if ( ( sdiff = (sp->me.x-d->left.x)*d->unit.y - (sp->me.y-d->left.y)*d->unit.x)<0 ) sdiff = -sdiff;
		if ( ( ediff = (sp->me.x-d->right.x)*d->unit.y - (sp->me.y-d->right.y)*d->unit.x)<0 ) ediff = -ediff;
		/* Hint Ranges for diagonals seem not to be what we want here */
		if ( sdiff<=10 && ( start==NULL ||
			( sp->me.y<start->me.y /*&& (InHintRange(d->where,pos) || !InHintRange(d->where,spos))*/ ))) {
		    start=sp;
		} else if ( ediff<=10 && ( end==NULL ||
			( sp->me.y<end->me.y /*&& (InHintRange(d->where,pos) || !InHintRange(d->where,epos))*/ ))) {
		    end=sp;
		}
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
	if ( sp->next==NULL )
    continue;
	if ( start!=NULL && end!=NULL ) {
	    if ( ValidBottomDSerif(start,end,depth,fuzz,ii,d))
    break;
	    else if ( ValidBottomDSerif(end,start,depth,fuzz,ii,d)) {
		SplinePoint *temp = start;
		start = end;
		end = temp;
    break;
	    } else
		start = NULL;
	}
    }

    if ( start==NULL || end==NULL )
	start = end = NULL;
    *_start = start;
    *_end   = end;
    *_ss    = ss;
}


static void FindTopSerifOnDStem(SplineChar *sc,int layer,DStemInfo *d,
	double height, ItalicInfo *ii,
	SplinePoint **_start,SplinePoint **_end, SplineSet **_ss) {
    SplinePoint *start=NULL, *end=NULL, *sp;
    SplinePointList *ss;
    double sdiff, ediff;
    double pos;
    double fuzz = (sc->parent->ascent+sc->parent->descent)/100.0;

    for ( ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	start=end=NULL;
	for ( sp=ss->first; ; ) {
	    if ( RoughlyParallel(sp,&d->unit)) {
		pos = (sp->me.x-d->left.x)*d->unit.x + (sp->me.y-d->left.y)*d->unit.y;
		if ( ( sdiff = (sp->me.x-d->left.x)*d->unit.y - (sp->me.y-d->left.y)*d->unit.x)<0 ) sdiff = -sdiff;
		if ( ( ediff = (sp->me.x-d->right.x)*d->unit.y - (sp->me.y-d->right.y)*d->unit.x)<0 ) ediff = -ediff;
		/* Hint Ranges for diagonals seem not to be what we want here */
		if ( sdiff<=10 && ( start==NULL ||
			( sp->me.y>start->me.y /*&& (InHintRange(d->where,pos) || !InHintRange(d->where,spos))*/ ))) {
		    start=sp;
		} else if ( ediff<=10 && ( end==NULL ||
			( sp->me.y>end->me.y /*&& (InHintRange(d->where,pos) || !InHintRange(d->where,epos))*/ ))) {
		    end=sp;
		}
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
	if ( sp->next==NULL )
    continue;
	if ( start!=NULL && end!=NULL ) {
	    if ( ValidTopDSerif(start,end,height,fuzz,ii,d))
    break;
	    else if ( ValidTopDSerif(end,start,height,fuzz,ii,d)) {
		SplinePoint *temp = start;
		start = end;
		end = temp;
    break;
	    } else
		start = NULL;
	}
    }

    if ( start==NULL || end==NULL )
	start = end = NULL;
    *_start = start;
    *_end   = end;
    *_ss    = ss;
}

static void SerifRemove(SplinePoint *start,SplinePoint *end,SplineSet *ss) {
    SplinePoint *mid, *spnext;

    for ( mid=start; mid!=end; mid=spnext ) {
	spnext = mid->next->to;
	if ( mid!=start ) {
	    SplinePointFree(mid);
	    if ( mid==ss->first ) {
		ss->first = ss->last = start;
		ss->start_offset = 0;
	    }
	}
	SplineFree(spnext->prev);
    }
    start->next = end->prev = NULL;
    start->nonextcp = end->noprevcp = true;
}

static SplinePoint *StemMoveBottomEndTo(SplinePoint *sp,double y,int is_start) {
    SplinePoint *other;

    if ( is_start ) {
	if ( sp->noprevcp || y>=sp->me.y ) {
	    sp->prevcp.y += (y-sp->me.y);
	    if ( sp->prev->order2 && !sp->prev->from->nonextcp )
		sp->prev->from->nextcp = sp->prevcp;
	    sp->me.y = y;
	    SplineRefigure(sp->prev);
	} else {
	    other = SplinePointCreate(sp->me.x,y);
	    sp->nonextcp = true;
	    SplineMake(sp,other,sp->prev->order2);
	    sp = other;
	}
    } else {
	if ( sp->nonextcp || y>=sp->me.y ) {
	    sp->nextcp.y += (y-sp->me.y);
	    if ( sp->next->order2 && !sp->next->to->noprevcp )
		sp->next->to->prevcp = sp->nextcp;
	    sp->me.y = y;
	    SplineRefigure(sp->next);
	} else {
	    other = SplinePointCreate(sp->me.x,y);
	    sp->noprevcp = true;
	    SplineMake(other,sp,sp->next->order2);
	    sp = other;
	}
    }
return( sp );
}

static SplinePoint *StemMoveDBottomEndTo(SplinePoint *sp,double y,DStemInfo *d,
	int is_start) {
    SplinePoint *other;
    double xoff;

    xoff = (y-sp->me.y)*d->unit.x/d->unit.y;
    if ( is_start ) {
	if ( sp->noprevcp || y>=sp->me.y ) {
	    sp->prevcp.y += (y-sp->me.y);
	    sp->prevcp.x += xoff;
	    if ( sp->prev->order2 && !sp->prev->from->nonextcp )
		sp->prev->from->nextcp = sp->prevcp;
	    sp->me.y = y;
	    sp->me.x += xoff;
	    SplineRefigure(sp->prev);
	} else {
	    other = SplinePointCreate(sp->me.x+xoff,y);
	    sp->nonextcp = true;
	    SplineMake(sp,other,sp->prev->order2);
	    sp = other;
	}
    } else {
	if ( sp->nonextcp || y>=sp->me.y ) {
	    sp->nextcp.y += (y-sp->me.y);
	    sp->nextcp.x += xoff;
	    if ( sp->next->order2 && !sp->next->to->noprevcp )
		sp->next->to->prevcp = sp->nextcp;
	    sp->me.y = y;
	    sp->me.x += xoff;
	    SplineRefigure(sp->next);
	} else {
	    other = SplinePointCreate(sp->me.x+xoff,y);
	    sp->noprevcp = true;
	    SplineMake(other,sp,sp->next->order2);
	    sp = other;
	}
    }
return( sp );
}

static SplinePoint *StemMoveBottomEndCarefully(SplinePoint *sp,SplineSet *oldss,
	SplineSet *ss,DStemInfo *d, int is_start) {
    SplinePoint *other = is_start ? ss->first : ss->last;

    if ( is_start ) {
	if ( sp->me.y<other->me.y &&
		(( sp->noprevcp && other->me.y>sp->prev->from->me.y) ||
		 (!sp->noprevcp && other->me.y>sp->prevcp.y)) ) {
	    /* We need to move sp up, but we can't because it turns down */
	    /*  So instead, move "other" down to sp */
	    extended ts[3];
	    /* Well, we might be able to move it up a little... */
	    if ( sp->prev->from->me.x==sp->me.x ) {
		SplinePoint *newsp = sp->prev->from;
		SplineFree(sp->prev);
		SplinePointFree(sp);
		if ( sp==oldss->first ) {
		    oldss->first = oldss->last = newsp;
		    oldss->start_offset = 0;
		}
		sp=newsp;
	    }
	    CubicSolve(&other->next->splines[1],sp->me.y,ts);
	    if ( ts[0]!=-1 ) {
		SplinePoint *newend = SplineBisect(other->next,ts[0]);
		SplineFree(newend->prev);
		SplinePointFree(other);
		newend->prev = NULL;
		newend->nextcp.x += sp->me.x-newend->me.x;
		if ( newend->next->order2 && !newend->nonextcp )
		    newend->next->to->prevcp = newend->nextcp;
		newend->me.x = sp->me.x;
		ss->first = newend;
		ss->start_offset = 0;
return( sp );
	    }
	}
    } else {
	if ( sp->me.y<other->me.y &&
		(( sp->nonextcp && other->me.y>sp->next->to->me.y) ||
		 (!sp->nonextcp && other->me.y>sp->nextcp.y)) ) {
	    extended ts[3];
	    if ( sp->next->to->me.x==sp->me.x ) {
		SplinePoint *newsp = sp->next->to;
		SplineFree(sp->next);
		SplinePointFree(sp);
		if ( sp==oldss->first ) {
		    oldss->first = oldss->last = newsp;
		    oldss->start_offset = 0;
		}
		sp=newsp;
	    }
	    CubicSolve(&other->prev->splines[1],sp->me.y,ts);
	    if ( ts[0]!=-1 ) {
		SplinePoint *newend = SplineBisect(other->prev,ts[0]);
		SplineFree(newend->next);
		SplinePointFree(other);
		newend->next = NULL;
		newend->prevcp.x += sp->me.x-newend->me.x;
		if ( newend->prev->order2 && !newend->noprevcp )
		    newend->prev->from->nextcp = newend->prevcp;
		newend->me.x = sp->me.x;
		ss->last = newend;
return( sp );
	    }
	}
    }
    if ( d==NULL )
return( StemMoveBottomEndTo(sp,other->me.y,is_start));
    else
return( StemMoveDBottomEndTo(sp,other->me.y,d,is_start));
}

static void DeSerifBottomStem(SplineChar *sc,int layer,StemInfo *h,ItalicInfo *ii,
	double y, SplinePoint **_start, SplinePoint **_end) {
    SplinePoint *start, *end, *mid;
    SplineSet *ss;

    if ( _start!=NULL )
	*_start = *_end = NULL;

    if ( h==NULL )
return;
    FindBottomSerifOnStem(sc,layer,h,y,ii,&start,&end,&ss);
    if ( start==NULL || end==NULL || start==end )
return;
    SerifRemove(start,end,ss);

    if ( ii->secondary_serif == srf_flat ) {
	start = StemMoveBottomEndTo(start,y,true);
	end = StemMoveBottomEndTo(end,y,false);
	start->nonextcp = end->noprevcp = true;
	SplineMake(start,end,sc->layers[layer].order2);
    } else if ( ii->secondary_serif == srf_simpleslant ) {
	if ( ii->tan_ia<0 ) {
	    start = StemMoveBottomEndTo(start,y+ (end->me.x-start->me.x)*ii->tan_ia,true);
	    end = StemMoveBottomEndTo(end,y,false);
	} else {
	    start = StemMoveBottomEndTo(start,y,true);
	    end = StemMoveBottomEndTo(end,y - (end->me.x-start->me.x)*ii->tan_ia,false);
	}
	start->nonextcp = end->noprevcp = true;
	SplineMake(start,end,sc->layers[layer].order2);
    } else {
	if ( ii->tan_ia<0 ) {
	    start = StemMoveBottomEndTo(start,y+ .8*(end->me.x-start->me.x)*ii->tan_ia,true);
	    end = StemMoveBottomEndTo(end,y+ .2*(end->me.x-start->me.x)*ii->tan_ia,false);
	    mid = SplinePointCreate(.2*start->me.x+.8*end->me.x,y);
	} else {
	    start = StemMoveBottomEndTo(start,y- .2*(end->me.x-start->me.x)*ii->tan_ia,true);
	    end = StemMoveBottomEndTo(end,y- .8*(end->me.x-start->me.x)*ii->tan_ia,false);
	    mid = SplinePointCreate(.2*end->me.x+.8*start->me.x,y);
	}
	start->nonextcp = end->noprevcp = true;
	mid->pointtype = pt_corner;
	SplineMake(start,mid,sc->layers[layer].order2);
	SplineMake(mid,end,sc->layers[layer].order2);
    }
    start->pointtype = end->pointtype = pt_corner;
    if ( _start!=NULL ) {
	*_start = start;
	*_end = end;
    }
}

static void DeSerifDescender(SplineChar *sc,int layer,ItalicInfo *ii) {
    /* sc should only have one descender. Find it */
    StemInfo *h;
    int i;
    HintInstance *hi;
    StemInfo *smallest=NULL;

    for ( i=0, h=sc->vstem; h!=NULL; ++i, h=h->next ) {
	for ( hi=h->where; hi!=NULL; hi=hi->next )
	    if ( hi->begin<0 || hi->end<0 ) {
		if ( smallest==NULL || h->width<smallest->width ) {
		    smallest = h;
	break;
		}
	    }
    }
    if ( smallest!=NULL )
	DeSerifBottomStem(sc,layer,smallest,ii,ii->pq_depth,NULL,NULL);
}

static double ValidTopSerif(SplinePoint *start,SplinePoint *end,
	double height, double fuzz, double minbound, double maxbound ) {
    double min = start->me.y<end->me.y ? start->me.y : end->me.y;
    SplinePoint *sp, *last;
    int got_down = false, got_up=false;
    /* To be a serif it must go up about as far up as we expect       */
    /*  (xheight, or the top of ascenders), it can bounce around     */
    /*  a little down there, but it can't go too far down. Once it starts */
    /*  going significantly up, it must continue going up until the end.  */

    if ( start==end )
return( false );

    last = NULL;
    for ( sp=start ; ; ) {
	if ( sp->me.x<minbound || sp->me.x>maxbound )
return( false );
	if ( sp->me.y < min-fuzz )
return( false );
	if ( sp->me.y > height+2*fuzz )
return( false );
	if ( sp->me.y > height-fuzz/2 )
	    got_up = true;
	else if ( got_up && sp->me.y < height-fuzz/2-1 )
	    got_down = true;
	if ( last!=NULL ) {
	    if ( !got_up && sp->me.y<last->me.y-fuzz/2 )
return( false );
	    else if ( got_down && sp->me.y>last->me.y+fuzz/2 )
return( false );
	}
	last = sp;
	if ( sp==end )
return( got_up );
	if ( sp->next==NULL )
return( false );
	sp = sp->next->to;
    }
}

static int IsLeftHalfSerif(SplinePoint *start,SplinePoint *end,
	StemInfo *h) {
    const double fuzz = 10;
    int wentleft=0;
    SplinePoint *sp;

    for ( sp=start; sp!=end; sp=sp->next->to ) {
	if ( sp->me.x > h->start+h->width+fuzz )
return( false );
	if ( sp->me.x < h->start )
	    wentleft = true;
    }
return( wentleft );
}

static void FindTopSerifOnStem(SplineChar *sc,int layer,StemInfo *h,
	double height, ItalicInfo *ii,
	SplinePoint **_start,SplinePoint **_end, SplineSet **_ss) {
    SplinePoint *start=NULL, *end=NULL, *sp;
    SplinePointList *ss;
    double sdiff, ediff;
    double fuzz = (sc->parent->ascent+sc->parent->descent)/100.0;

    for ( ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	start=end=NULL;
	for ( sp=ss->first; ; ) {
	    if ( ( sdiff = sp->me.x-h->start)<0 ) sdiff = -sdiff;
	    if ( ( ediff = sp->me.x-(h->start+h->width))<0 ) ediff = -ediff;
	    if ( sdiff<=3 && ( start==NULL ||
		    ( sp->me.y>start->me.y && (InHintRange(h->where,sp->me.y) || !InHintRange(h->where,start->me.y))))) {
		start=sp;
	    } else if ( ediff<=3 && ( end==NULL ||
		    ( sp->me.y>end->me.y && (InHintRange(h->where,sp->me.y) || !InHintRange(h->where,end->me.y)) ))) {
		end=sp;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
	if ( sp->next==NULL )
    continue;
	if ( start!=NULL && end!=NULL ) {
	    if ( ValidTopSerif(start,end,height,fuzz,h->start-ii->serif_extent-fuzz,h->start+h->width+ii->serif_extent+fuzz))
    break;
	    else if ( ValidTopSerif(end,start,height,fuzz,h->start-ii->serif_extent-fuzz,h->start+h->width+ii->serif_extent+fuzz)) {
		SplinePoint *temp = start;
		start = end;
		end = temp;
    break;
	    } else
		start = NULL;
	}
    }

    if ( start==NULL || end==NULL ) {
	start = end = NULL;
	ss = NULL;
    }
    *_start = start;
    *_end   = end;
    *_ss    = ss;
}

static SplinePoint *StemMoveTopEndTo(SplinePoint *sp,double y,int is_start) {
    SplinePoint *other;

    if ( is_start ) {
	if ( sp->noprevcp || y<=sp->me.y ) {
	    sp->prevcp.y += (y-sp->me.y);
	    if ( sp->prev->order2 && !sp->prev->from->nonextcp )
		sp->prev->from->nextcp = sp->prevcp;
	    sp->me.y = y;
	    SplineRefigure(sp->prev);
	} else {
	    other = SplinePointCreate(sp->me.x,y);
	    sp->nonextcp = true;
	    SplineMake(sp,other,sp->prev->order2);
	    sp = other;
	}
    } else {
	if ( sp->nonextcp || y<=sp->me.y ) {
	    sp->nextcp.y += (y-sp->me.y);
	    if ( sp->next->order2 && !sp->next->to->noprevcp )
		sp->next->to->prevcp = sp->nextcp;
	    sp->me.y = y;
	    SplineRefigure(sp->next);
	} else {
	    other = SplinePointCreate(sp->me.x,y);
	    sp->noprevcp = true;
	    SplineMake(other,sp,sp->next->order2);
	    sp = other;
	}
    }
return( sp );
}

static SplinePoint *StemMoveDTopEndTo(SplinePoint *sp,double y,DStemInfo *d,
	int is_start) {
    SplinePoint *other;
    double xoff;

    xoff = (y-sp->me.y)*d->unit.x/d->unit.y;
    if ( is_start ) {
	other = SplinePointCreate(sp->me.x+xoff,y);
	sp->nonextcp = true;
	SplineMake(sp,other,sp->prev->order2);
	sp = other;
    } else {
	other = SplinePointCreate(sp->me.x+xoff,y);
	sp->noprevcp = true;
	SplineMake(other,sp,sp->next->order2);
	sp = other;
    }
return( sp );
}

static SplinePoint *StemMoveTopEndCarefully(SplinePoint *sp,SplineSet *oldss,
	SplineSet *ss,DStemInfo *d, int is_start) {
    SplinePoint *other = is_start ? ss->first : ss->last;

    if ( is_start ) {
	if ( sp->me.y>other->me.y &&
		(( sp->noprevcp && other->me.y<sp->prev->from->me.y) ||
		 (!sp->noprevcp && other->me.y<sp->prevcp.y)) ) {
	    /* We need to move sp up, but we can't because it turns down */
	    /*  So instead, move "other" down to sp */
	    extended ts[3];
	    /* Well, we might be able to move it up a little... */
	    if ( sp->prev->from->me.x==sp->me.x ) {
		SplinePoint *newsp = sp->prev->from;
		SplineFree(sp->prev);
		SplinePointFree(sp);
		if ( sp==oldss->first ) {
		    oldss->first = oldss->last = newsp;
		    oldss->start_offset = 0;
		}
		sp=newsp;
	    }
	    CubicSolve(&other->next->splines[1],sp->me.y,ts);
	    if ( ts[0]!=-1 ) {
		SplinePoint *newend = SplineBisect(other->next,ts[0]);
		SplineFree(newend->prev);
		SplinePointFree(other);
		newend->prev = NULL;
		newend->nextcp.x += sp->me.x-newend->me.x;
		if ( newend->next->order2 && !newend->nonextcp )
		    newend->next->to->prevcp = newend->nextcp;
		newend->me.x = sp->me.x;
		ss->first = newend;
		ss->start_offset = 0;
return( sp );
	    }
	}
    } else {
	if ( sp->me.y>other->me.y &&
		(( sp->nonextcp && other->me.y<sp->next->to->me.y) ||
		 (!sp->nonextcp && other->me.y<sp->nextcp.y)) ) {
	    extended ts[3];
	    if ( sp->next->to->me.x==sp->me.x ) {
		SplinePoint *newsp = sp->next->to;
		SplineFree(sp->next);
		SplinePointFree(sp);
		if ( sp==oldss->first ) {
		    oldss->first = oldss->last = newsp;
		    oldss->start_offset = 0;
		}
		sp=newsp;
	    }
	    CubicSolve(&other->prev->splines[1],sp->me.y,ts);
	    if ( ts[0]!=-1 ) {
		SplinePoint *newend = SplineBisect(other->prev,ts[0]);
		SplineFree(newend->next);
		SplinePointFree(other);
		newend->next = NULL;
		newend->prevcp.x += sp->me.x-newend->me.x;
		if ( newend->prev->order2 && !newend->noprevcp )
		    newend->prev->from->nextcp = newend->prevcp;
		newend->me.x = sp->me.x;
		ss->last = newend;
return( sp );
	    }
	}
    }
    if ( d==NULL )
return( StemMoveTopEndTo(sp,other->me.y,is_start));
    else
return( StemMoveDTopEndTo(sp,other->me.y,d,is_start));
}

static void SplineNextSplice(SplinePoint *start,SplinePoint *newstuff) {
    start->next = newstuff->next;
    start->next->from = start;
    start->nextcp = newstuff->nextcp;
    start->nonextcp = newstuff->nonextcp;
    if ( start->me.x!=newstuff->me.x || start->me.y!=newstuff->me.y ) {
	double xdiff = start->me.x-newstuff->me.x, ydiff = start->me.y-newstuff->me.y;
	SplinePoint *nsp = start->next->to;
	if ( start->next->order2 ) {
	    if ( !nsp->noprevcp ) {
		start->nextcp.x += xdiff/2;
		start->nextcp.y += ydiff/2;
		nsp->prevcp = start->nextcp;
	    } else {
		start->nextcp.x += xdiff;
		start->nextcp.y += ydiff;
	    }
	} else {
	    start->nextcp.x += xdiff;
	    start->nextcp.y += ydiff;
	    nsp->prevcp.x += xdiff/2;
	    nsp->prevcp.y += ydiff/2;
	    nsp->me.x += xdiff/2;
	    nsp->me.y += ydiff/2;
	    nsp->nextcp.x += xdiff/2;
	    nsp->nextcp.y += ydiff/2;
	    SplineRefigure(nsp->next);
	}
	SplineRefigure(nsp->prev);
    }
    SplinePointFree(newstuff);
}

static void SplinePrevSplice(SplinePoint *end,SplinePoint *newstuff) {
    end->prev = newstuff->prev;
    end->prev->to = end;
    end->prevcp = newstuff->prevcp;
    end->noprevcp = newstuff->noprevcp;
    if ( end->me.x!=newstuff->me.x || end->me.y!=newstuff->me.y ) {
	double xdiff = end->me.x-newstuff->me.x, ydiff = end->me.y-newstuff->me.y;
	SplinePoint *psp = end->prev->from;
	if ( end->prev->order2 ) {
	    if ( !psp->noprevcp ) {
		end->prevcp.x += xdiff/2;
		end->prevcp.y += ydiff/2;
		psp->nextcp = end->prevcp;
	    } else {
		end->nextcp.x += xdiff;
		end->nextcp.y += ydiff;
	    }
	} else {
	    end->nextcp.x += xdiff;
	    end->nextcp.y += ydiff;
	    psp->prevcp.x += xdiff/2;
	    psp->prevcp.y += ydiff/2;
	    psp->me.x += xdiff/2;
	    psp->me.y += ydiff/2;
	    psp->nextcp.x += xdiff/2;
	    psp->nextcp.y += ydiff/2;
	    SplineRefigure(psp->prev);
	}
	SplineRefigure(psp->next);
    }
    SplinePointFree(newstuff);
}

static void ReSerifBottomStem(SplineChar *sc,int layer,StemInfo *h,ItalicInfo *ii) {
    SplinePoint *start, *end;
    SplineSet *ss, *oldss;

    if ( h==NULL )
return;
    FindBottomSerifOnStem(sc,layer,h,0,ii,&start,&end,&oldss);
    if ( start==NULL || end==NULL || start==end )
return;
    SerifRemove(start,end,oldss);

    ss = MakeBottomItalicSerif(start->me.x-end->me.x,end->me.x,ii,0);
    start = StemMoveBottomEndCarefully(start,oldss,ss,NULL,true );
    end   = StemMoveBottomEndCarefully(end  ,oldss,ss,NULL,false);

    SplineNextSplice(start,ss->first);
    SplinePrevSplice(end,ss->last);
    chunkfree(ss,sizeof(*ss));
}

static void ReSerifBottomDStem(SplineChar *sc,int layer,DStemInfo *d,ItalicInfo *ii) {
    SplinePoint *start, *end;
    SplineSet *ss, *oldss;
    double stemwidth;
    int seriftype;

    if ( d==NULL )
return;
    FindBottomSerifOnDStem(sc,layer,d,0,ii,&start,&end,&oldss);
    if ( start==NULL || end==NULL || start==end )
return;
    SerifRemove(start,end,oldss);

    stemwidth = (d->right.x-d->left.x)*d->unit.y - (d->right.y-d->left.y)*d->unit.x;
    if ( stemwidth<0 ) stemwidth = -stemwidth;
    if ( d->unit.x*d->unit.y<0 )
	seriftype = 0;
    else if ( stemwidth<boldleftfacingitalicserif.stemwidth*ii->emsize/1000.0+5 )
	seriftype = 1;
    else
	seriftype = 0;
    ss = MakeItalicDSerif(d,stemwidth, end->me.x,ii,seriftype,false);

    start = StemMoveBottomEndCarefully(start,oldss,ss,d,true );
    end   = StemMoveBottomEndCarefully(end  ,oldss,ss,d,false);

    SplineNextSplice(start,ss->first);
    SplinePrevSplice(end,ss->last);
    chunkfree(ss,sizeof(*ss));
}

static void ReSerifXHeightDStem(SplineChar *sc,int layer,DStemInfo *d,ItalicInfo *ii) {
    SplinePoint *start, *end;
    SplineSet *ss, *oldss;
    double stemwidth;
    int seriftype;

    if ( d==NULL )
return;
    FindTopSerifOnDStem(sc,layer,d,ii->x_height,ii,&start,&end,&oldss);
    if ( start==NULL || end==NULL || start==end )
return;
    SerifRemove(start,end,oldss);

    stemwidth = (d->right.x-d->left.x)*d->unit.y - (d->right.y-d->left.y)*d->unit.x;
    if ( stemwidth<0 ) stemwidth = -stemwidth;
    if ( d->unit.x*d->unit.y<0 )
	seriftype = 0;
    else if ( stemwidth<boldleftfacingitalicserif.stemwidth*ii->emsize/1000.0+5 )
	seriftype = 1;
    else
	seriftype = 0;
    ss = MakeItalicDSerif(d,stemwidth, end->me.x,ii,seriftype,1);

    start = StemMoveTopEndCarefully(start,oldss,ss,d,true );
    end   = StemMoveTopEndCarefully(end  ,oldss,ss,d,false);

    SplineNextSplice(start,ss->first);
    SplinePrevSplice(end,ss->last);
    chunkfree(ss,sizeof(*ss));
}

static int NearBottomRightSide(DStemInfo *d,DBounds *b,ItalicInfo *ii) {
    double x;

    x = d->left.x - d->left.y*d->unit.x/d->unit.y;
    if ( x+1.5*ii->serif_extent+30>b->maxx )
return( true );
    x = d->right.x - d->right.y*d->unit.x/d->unit.y;
    if ( x+1.5*ii->serif_extent+30>b->maxx )
return( true );

return( false );
}

static int NearBottomLeftSide(DStemInfo *d,DBounds *b,ItalicInfo *ii) {
    double x;

    x = d->left.x - d->left.y*d->unit.x/d->unit.y;
    if ( x-1.5*ii->serif_extent-30<b->minx )
return( true );
    x = d->right.x - d->right.y*d->unit.x/d->unit.y;
    if ( x-1.5*ii->serif_extent-30<b->minx )
return( true );

return( false );
}

static int NearXHeightRightSide(DStemInfo *d,DBounds *b,ItalicInfo *ii) {
    double x;

    x = d->left.x - (d->left.y-ii->x_height)*d->unit.x/d->unit.y;
    if ( x+1.5*ii->serif_extent+30>b->maxx )
return( true );
    x = d->right.x - (d->right.y-ii->x_height)*d->unit.x/d->unit.y;
    if ( x+1.5*ii->serif_extent+30>b->maxx )
return( true );

return( false );
}

static int NearXHeightLeftSide(DStemInfo *d,DBounds *b,ItalicInfo *ii) {
    double x;

    x = d->left.x - (d->left.y-ii->x_height)*d->unit.x/d->unit.y;
    if ( x-1.5*ii->serif_extent-30<b->minx )
return( true );
    x = d->right.x - (d->right.y-ii->x_height)*d->unit.x/d->unit.y;
    if ( x-1.5*ii->serif_extent-30<b->minx )
return( true );

return( false );
}

static void AddBottomItalicSerifs(SplineChar *sc,int layer,ItalicInfo *ii) {
    StemInfo *h;
    int j, cnt;
    /* If a glyph has multiple stems then only the last one gets turned to */
    /*  the italic serif. The others get deserifed. Note that if the glyph is */
    /*  "k" then the "last stem" is actually a diagonal stem rather than a */
    /*  vertical one */
    /* "r", on the other hand, just doesn't get a serif */
    /* Hmm better than saying the last stem, let's say stem at the right edge */
    DBounds b;

    SplineCharLayerFindBounds(sc,layer,&b);
    for ( h=sc->vstem, cnt=0; h!=NULL; h=h->next )
	if ( h->tobeused )
	    ++cnt;
    for ( j=0, h=sc->vstem; h!=NULL; h=h->next ) if ( h->tobeused ) {
	if ( j==cnt-1 && h->start+h->width+ii->serif_extent+20>b.maxx )
	    ReSerifBottomStem(sc,layer,h,ii);
	else
	    DeSerifBottomStem(sc,layer,h,ii,0,NULL,NULL);
	++j;
    }
}

static void DeSerifTopStem(SplineChar *sc,int layer,StemInfo *h,ItalicInfo *ii,
	double y) {
    SplinePoint *start, *end;
    SplineSet *ss;

    if ( h==NULL )
return;
    FindTopSerifOnStem(sc,layer,h,y,ii,&start,&end,&ss);
    if ( start==NULL || end==NULL || start==end )
return;
    SerifRemove(start,end,ss);

    start = StemMoveBottomEndTo(start,y,true);
    end = StemMoveBottomEndTo(end,y,false);
    start->nonextcp = end->noprevcp = true;
    SplineMake(start,end,sc->layers[layer].order2);
    start->pointtype = end->pointtype = pt_corner;
}

static void ReSerifTopStem(SplineChar *sc,int layer,StemInfo *h,ItalicInfo *ii) {
    SplinePoint *start=NULL, *end=NULL;
    SplineSet *ss, *oldss;
    int at_xh = true;

    if ( h==NULL )
return;
    if ( ii->transform_top_xh_serifs )
	FindTopSerifOnStem(sc,layer,h,ii->x_height,ii,&start,&end,&oldss);
    if ( start==NULL && ii->transform_top_as_serifs ) {
	FindTopSerifOnStem(sc,layer,h,ii->ascender_height,ii,&start,&end,&oldss);
	at_xh = false;
    }
    if ( start==NULL || end==NULL || start==end || !IsLeftHalfSerif(start,end,h))
return;
    SerifRemove(start,end,oldss);

    ss = MakeTopItalicSerif(start->me.x-end->me.x,end->me.x,ii,at_xh);
    start = StemMoveTopEndCarefully(start,oldss,ss,NULL,true );
    end   = StemMoveTopEndCarefully(end  ,oldss,ss,NULL,false);

    SplineNextSplice(start,ss->first);
    SplinePrevSplice(end,ss->last);
    chunkfree(ss,sizeof(*ss));
}

static void AddTopItalicSerifs(SplineChar *sc,int layer,ItalicInfo *ii) {
    StemInfo *h;
    /* The leftmost stem of certain lower case letters can go from a half */
    /* serif to an italic serif. Sometimes it is just stems at the x-height */
    /* sometimes ascenders too, often none */
    DBounds b;

    SplineCharLayerFindBounds(sc,layer,&b);
    for ( h=sc->vstem; h!=NULL; h=h->next ) if ( h->tobeused ) {
	if ( h->start-ii->serif_extent-20<b.minx || sc->unicodeenc=='j' || sc->unicodeenc==0x237 )
	    ReSerifTopStem(sc,layer,h,ii);
    break;
    }
    if ( sc->unicodeenc=='u' || sc->unicodeenc==0x436 ) {
	if ( h!=NULL && sc->unicodeenc=='u' )
	    h=h->next;
	while ( h!=NULL ) {
	    if ( h->tobeused ) {
		DeSerifTopStem(sc,layer,h,ii,ii->x_height);
	break;
	    }
	    h = h->next;
	}
    }
}

static void AddDiagonalItalicSerifs(SplineChar *sc,int layer,ItalicInfo *ii) {
    DStemInfo *d;
    DBounds b;

    SplineCharLayerFindBounds(sc,layer,&b);
    for ( d=sc->dstem; d!=NULL; d=d->next ) {
      /* Serifs on diagonal stems at baseline */
	if ( d->unit.x*d->unit.y<0 && NearBottomRightSide(d,&b,ii))
	    ReSerifBottomDStem(sc,layer,d,ii);
	else if ( d->unit.x*d->unit.y>0 && NearBottomLeftSide(d,&b,ii))
	    ReSerifBottomDStem(sc,layer,d,ii);

      /* Serifs on diagonal stems at xheight */
	if ( d->unit.x*d->unit.y>0 && NearXHeightRightSide(d,&b,ii))
	    ReSerifXHeightDStem(sc,layer,d,ii);
	else if ( d->unit.x*d->unit.y<0 && NearXHeightLeftSide(d,&b,ii))
	    ReSerifXHeightDStem(sc,layer,d,ii);
    }
}

static int FFCopyTrans(ItalicInfo *ii,real *transform,
	SplinePoint **ff_start1,SplinePoint **ff_end1, SplinePoint **ff_start2, SplinePoint **ff_end2) {
    SplinePoint *sp, *last, *cur;
    int touches;

    last = NULL;
    for ( sp = ii->ff_start1; ; sp=sp->next->to ) {
	cur = chunkalloc(sizeof(SplinePoint));
	*cur = *sp;
	cur->hintmask = NULL;
	cur->me.x = transform[0]*sp->me.x + transform[2]*sp->me.y + transform[4];
	cur->me.y = transform[1]*sp->me.x + transform[3]*sp->me.y + transform[5];
	cur->nextcp.x = transform[0]*sp->nextcp.x + transform[2]*sp->nextcp.y + transform[4];
	cur->nextcp.y = transform[1]*sp->nextcp.x + transform[3]*sp->nextcp.y + transform[5];
	cur->prevcp.x = transform[0]*sp->prevcp.x + transform[2]*sp->prevcp.y + transform[4];
	cur->prevcp.y = transform[1]*sp->prevcp.x + transform[3]*sp->prevcp.y + transform[5];
	if ( last==NULL )
	    *ff_start1 = cur;
	else
	    SplineMake(last,cur,sp->prev->order2);
	last = cur;
	if ( sp==ii->ff_end1 || sp==ii->ff_end2 )
    break;
    }
    if ( sp==ii->ff_end1 ) {
	touches = false;
	*ff_end1 = last;
    } else {
	touches = true;
	*ff_end2 = last;
    }

    last = NULL;
    for ( sp = ii->ff_start2; ; sp=sp->next->to ) {
	cur = chunkalloc(sizeof(SplinePoint));
	*cur = *sp;
	cur->hintmask = NULL;
	cur->me.x = transform[0]*sp->me.x + transform[2]*sp->me.y + transform[4];
	cur->me.y = transform[1]*sp->me.x + transform[3]*sp->me.y + transform[5];
	cur->nextcp.x = transform[0]*sp->nextcp.x + transform[2]*sp->nextcp.y + transform[4];
	cur->nextcp.y = transform[1]*sp->nextcp.x + transform[3]*sp->nextcp.y + transform[5];
	cur->prevcp.x = transform[0]*sp->prevcp.x + transform[2]*sp->prevcp.y + transform[4];
	cur->prevcp.y = transform[1]*sp->prevcp.x + transform[3]*sp->prevcp.y + transform[5];
	if ( last==NULL )
	    *ff_start2 = cur;
	else
	    SplineMake(last,cur,sp->prev->order2);
	last = cur;
	if ( sp==ii->ff_end1 || sp==ii->ff_end2 )
    break;
    }
    if ( sp==ii->ff_end1 ) {
	*ff_end1 = last;
    } else {
	*ff_end2 = last;
    }
return( touches );
}

static void FigureFFTop(ItalicInfo *ii) {
    SplineChar *ff;
    StemInfo *h;
    SplinePoint *bests[2][2], *sp;
    SplineSet *ss;
    double sdiff, ediff;
    DBounds b;
    int i;
    real trans[6];

    if ( ii->ff_start1!=NULL )
return;

    ff = SFGetChar(ii->sf,0xfb00,NULL);			/* unicode ff ligature */
    if ( ff==NULL )
	ff = SFGetChar(ii->sf,-1, "f_f");
    if ( ff==NULL )
	ff = SFGetChar(ii->sf,-1, "longs_longs");		/* Long s */
    if ( ff==NULL )
return;
    if ( autohint_before_generate && (ff->changedsincelasthinted || ff->vstem==NULL) &&
	    !ff->manualhints )
	SplineCharAutoHint(ff,ii->layer,NULL);
    FigureGoodStems(ff->vstem);

    memset(bests,0,sizeof(bests));
    for ( ss=ff->layers[ii->layer].splines; ss!=NULL; ss=ss->next ) {
	if ( ss->first->prev==NULL )
    continue;
	for ( sp=ss->first; ; ) {
	    if ( sp->me.y>.9*ii->x_height ) {
		for ( h=ff->vstem, i=0; h!=NULL && i<2; h=h->next ) if ( h->tobeused ) {
		    if ( (sdiff = sp->me.x-h->start)<0 ) sdiff = -sdiff;
		    if ( (ediff = sp->me.x-h->start-h->width)<0 ) ediff = -ediff;
		    if ( sdiff<3 && (bests[i][0]==NULL || sp->me.y>bests[i][0]->me.y )) {
			bests[i][0] = sp;
		    } else if ( ediff<3 && (bests[i][1]==NULL || sp->me.y>bests[i][1]->me.y )) {
			bests[i][1] = sp;
		    }
		    ++i;
		}
	    }
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }

    if ( bests[0][0]==NULL || bests[0][1]==NULL || bests[1][0]==NULL || bests[1][1]==NULL )
return;

    ii->ff_start1 = bests[0][0]; ii->ff_end1 = bests[0][1];
    ii->ff_start2 = bests[1][0]; ii->ff_end2 = bests[1][1];
    SplineCharLayerFindBounds(ff,ii->layer,&b);
    ii->ff_height = b.maxy - ii->ff_start1->me.y;

    /* Now we need to save a copy of this because we might modify the ff glyph */
    /*  before we do the "ffi" glyph (we will if things are in unicode ordering) */
    /*  and then we'd try to put a pre-slanted thing on the bottom of ffi and */
    /*  cause all kinds of havoc */
    memset(trans,0,sizeof(trans));
    trans[0] = trans[3] = 1;
    FFCopyTrans(ii,trans,&bests[0][0],&bests[0][1],&bests[1][0],&bests[1][1]);
    ii->ff_start1 = bests[0][0]; ii->ff_end1 = bests[0][1];
    ii->ff_start2 = bests[1][0]; ii->ff_end2 = bests[1][1];
}

static void FFBottomFromTop(SplineChar *sc,int layer,ItalicInfo *ii) {
    StemInfo *h;
    SplinePoint *start[2], *end[2], *f_start[2], *f_end[2];
    SplineSet *ss[2];
    real transform[6];
    int cnt;
    double bottom_y = 0;
    int touches;

    FigureFFTop(ii);
    if ( ii->ff_start1==NULL )
return;
    cnt=0;
    for ( h=sc->vstem; h!=NULL && cnt<2; h=h->next ) {
	if ( !h->tobeused )
    continue;
	FindBottomSerifOnStem(sc,layer,h,bottom_y,ii,&start[cnt],&end[cnt],&ss[cnt]);
	if ( start[cnt]==NULL )
    continue;
	++cnt;
    }

    if ( cnt!=2 )
return;

    SerifRemove(start[0],end[0],ss[0]);
    SerifRemove(start[1],end[1],ss[1]);

    memset(transform,0,sizeof(transform));
    transform[0] = transform[3] = -1;
    transform[4] = start[0]->me.x + ii->ff_start2->me.x;
    transform[5] = ii->ff_start1->me.y + ii->pq_depth + ii->ff_height;
    touches = FFCopyTrans(ii,transform,&f_start[0],&f_end[0],&f_start[1],&f_end[1]);
    /* Now there are several cases to consider */
    /* First: Did the tops of the two "f"s touch one another or were they two */
    /*  distinct stems with no connection. If they touch then start1->end2 is */
    /*  one path and start2->end1 (counterclockwise) is the other. If they do */
    /*  not touch then start1->end1 and start2->end2. */
    /* If they do not touch then things are pretty simple. We remove serifs  */
    /*  with one shape from the bottom of two stems and we replace them with */
    /*  serifs of a different shape, but the contour structure does not change*/
    /* But if they touch... things are more complicated. And we have more cases*/
    /* Suppose we are applying things to two "longs"es. Do the two longses */
    /*  touch (forming one contour) or are they distinct (forming two) */
    /* If they formed two contours then we will join them with our touching */
    /*  f-tails and we will need to remove one of the two SplineSet structs */
    /* If they form a single contour then we will change that single contour */
    /*  into two, an inner and and outer contour, and we need to add an SS */
    /*  structure. */
    start[0] = StemMoveBottomEndTo(start[0],f_start[1]->me.y,true);
    end[0] = StemMoveBottomEndTo(end[0],f_end[1]->me.y,false);
    SplineNextSplice(start[0],f_start[1]);
    SplinePrevSplice(end[0],f_end[1]);
    start[1] = StemMoveBottomEndTo(start[1],f_start[0]->me.y,true);
    end[1] = StemMoveBottomEndTo(end[1],f_end[0]->me.y,false);
    SplineNextSplice(start[1],f_start[0]);
    SplinePrevSplice(end[1],f_end[0]);
    if ( touches ) {
	if ( ss[0]==ss[1] ) {
	    ss[0]->first = ss[0]->last = start[0];
	    ss[0]->start_offset = 0;
	    ss[1] = chunkalloc(sizeof(SplineSet));
	    ss[1]->next = ss[0]->next;
	    ss[0]->next = ss[1];
	    ss[1]->first = ss[1]->last = start[1];
	    ss[1]->start_offset = 0;
	} else {
	    SplineSet *spl, *prev;
	    for ( prev=NULL, spl=sc->layers[layer].splines; spl!=ss[1]; spl=spl->next )
		prev = spl;
	    if ( prev==NULL )
		sc->layers[layer].splines = ss[1]->next;
	    else
		prev->next = ss[1]->next;
	    chunkfree(ss[1],sizeof(SplineSet));
	}
    }

    SplineSetRefigure(sc->layers[layer].splines);
}

static void FCopyTrans(ItalicInfo *ii,real *transform,SplinePoint **f_start,SplinePoint **f_end) {
    SplinePoint *sp, *last, *cur;

    last = NULL;
    for ( sp = ii->f_start; ; sp=sp->next->to ) {
	cur = chunkalloc(sizeof(SplinePoint));
	*cur = *sp;
	cur->hintmask = NULL;
	cur->me.x = transform[0]*sp->me.x + transform[2]*sp->me.y + transform[4];
	cur->me.y = transform[1]*sp->me.x + transform[3]*sp->me.y + transform[5];
	cur->nextcp.x = transform[0]*sp->nextcp.x + transform[2]*sp->nextcp.y + transform[4];
	cur->nextcp.y = transform[1]*sp->nextcp.x + transform[3]*sp->nextcp.y + transform[5];
	cur->prevcp.x = transform[0]*sp->prevcp.x + transform[2]*sp->prevcp.y + transform[4];
	cur->prevcp.y = transform[1]*sp->prevcp.x + transform[3]*sp->prevcp.y + transform[5];
	if ( last==NULL )
	    *f_start = cur;
	else
	    SplineMake(last,cur,sp->prev->order2);
	last = cur;
	if ( sp==ii->f_end )
    break;
    }
    *f_end = last;
}

static void FigureFTop(ItalicInfo *ii) {
    SplineChar *f;
    StemInfo *h;
    SplinePoint *beste, *bests, *sp;
    SplineSet *ss;
    double bestediff, sdiff, ediff;
    DBounds b;
    real trans[6];

    if ( ii->f_start!=NULL )
return;

    f = SFGetChar(ii->sf,'f',NULL);
    if ( f==NULL )
	f = SFGetChar(ii->sf,0x17f,NULL);		/* Long s */
    if ( f==NULL )
return;
    if ( autohint_before_generate && (f->changedsincelasthinted || f->vstem==NULL) &&
	    !f->manualhints )
	SplineCharAutoHint(f,ii->layer,NULL);
    FigureGoodStems(f->vstem);

    for ( h=f->vstem; h!=NULL; h=h->next ) if ( h->tobeused ) {
	for ( ss=f->layers[ii->layer].splines; ss!=NULL; ss=ss->next ) {
	    bests = beste = NULL;
	    bestediff = 10;
	    for ( sp=ss->first; ; ) {
		if ( sp->me.y>.9*ii->x_height ) {
		    if ( (sdiff = sp->me.x-h->start)<0 ) sdiff = -sdiff;
		    if ( (ediff = sp->me.x-h->start-h->width)<0 ) ediff = -ediff;
		    if ( sdiff<3 && (bests==NULL || sp->me.y>bests->me.y ))
			bests = sp;
		    else if ( ediff<3 && (beste==NULL || sp->me.y>beste->me.y )) {
			bestediff = ediff;
			beste = sp;
		    }
		}
		if ( sp->next==NULL ) {
		    bests = beste = NULL;
	    break;
		}
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	    if ( bests!=NULL && beste!=NULL ) {
		if ( bests->next->to->me.y > bests->me.y ) {
		    ii->f_start = bests;
		    ii->f_end   = beste;
		} else {
		    ii->f_start = beste;
		    ii->f_end   = bests;
		}
		SplineCharLayerFindBounds(f,ii->layer,&b);
		ii->f_height = b.maxy - ii->f_start->me.y;
    /* Now we need to save a copy of this because we might modify the f glyph */
    /*  before we do the "fi" glyph (we will if things are in unicode ordering) */
    /*  and then we'd try to put a pre-slanted thing on the bottom of fi and */
    /*  cause all kinds of havoc */
		memset(trans,0,sizeof(trans));
		trans[0] = trans[3] = 1;
		FCopyTrans(ii,trans,&ii->f_start,&ii->f_end);
return;
	    }
	}
    }
}

static void FBottomFromTop(SplineChar *sc,int layer,ItalicInfo *ii) {
    StemInfo *h;
    SplinePoint *start, *end, *f_start, *f_end;
    SplineSet *ss;
    real transform[6];
    int cnt;
    double bottom_y = 0;
    DBounds b;

    if ( sc->unicodeenc==0x444 ) {
	cnt = 1;	/* cyrillic phi is only sometimes like an f. If we get here it is */
	SplineCharLayerFindBounds(sc,layer,&b);
	bottom_y = b.miny;
    } else
	cnt = LikeAnF(sc);
    if ( cnt==2 ) {
	FFBottomFromTop(sc,layer,ii);
return;
    } else if ( cnt!=1 )
return;		/* I don't think the combination fff ever happens except in hex dumps */
    FigureFTop(ii);
    if ( ii->f_start==NULL || ii->f_end==NULL )
return;
    for ( h=sc->vstem; h!=NULL; h=h->next ) {
	if ( !h->tobeused )
    continue;
	FindBottomSerifOnStem(sc,layer,h,bottom_y,ii,&start,&end,&ss);
	if ( start==NULL )
    continue;
	SerifRemove(start,end,ss);
	memset(transform,0,sizeof(transform));
	transform[0] = transform[3] = -1;
	transform[4] = start->me.x + ii->f_start->me.x;
	transform[5] = ii->f_start->me.y + ii->pq_depth + ii->f_height;
	FCopyTrans(ii,transform,&f_start,&f_end);
	start = StemMoveBottomEndTo(start,f_start->me.y,true);
	end = StemMoveBottomEndTo(end,f_end->me.y,false);
	SplineNextSplice(start,f_start);
	SplinePrevSplice(end,f_end);
    break;
    }
}

static void FBottomGrows(SplineChar *sc,int layer,ItalicInfo *ii) {
    int cnt;
    int i;
    StemInfo *h;
    SplinePoint *start, *end, *sp;

    if ( sc->unicodeenc==0x444 )
	cnt = 1;	/* cyrillic phi is only sometimes like an f. If we get here it is */
    else
	cnt = LikeAnF(sc);

    for ( h=sc->vstem, i=0; i<cnt && h!=NULL; h=h->next ) {
	if ( !h->tobeused )
    continue;
	DeSerifBottomStem(sc,layer,h,ii,0,&start,&end);
	if ( start!=NULL ) {
	    /* The stem currently ends at the baseline, move it down to the */
	    /*  pq_descent line */
	    if ( !start->noprevcp ) {
		sp = SplinePointCreate(start->me.x,start->me.y+ii->pq_depth);
		sp->next = start->next;
		sp->next->from = sp;
		start->nonextcp = true;
		SplineMake(start,sp,sc->layers[layer].order2);
		start = sp;
	    } else {
		start->me.y = start->nextcp.y = start->prevcp.y += ii->pq_depth;
		SplineRefigure(start->prev);
	    }
	    if ( !end->nonextcp ) {
		sp = SplinePointCreate(end->me.x,end->me.y+ii->pq_depth);
		sp->prev = start->prev;
		sp->prev->to = sp;
		end->noprevcp = true;
		SplineMake(sp,end,sc->layers[layer].order2);
		end = sp;
	    } else {
		end->me.y = end->nextcp.y = end->prevcp.y += ii->pq_depth;
		SplineRefigure(end->next);
	    }
	    if ( start->next->to == end )
		SplineRefigure(start->next);
	    else {
		SplinePoint *mid = start->next->to;
		mid->me.y = mid->nextcp.y = mid->prevcp.y += ii->pq_depth;
		SplineRefigure(mid->next);
		SplineRefigure(mid->prev);
	    }
	    ++i;
	}
    }
}

static void FBottomTransform(SplineChar *sc,int layer,ItalicInfo *ii) {
    if ( ii->f_rotate_top )
	FBottomFromTop(sc,layer,ii);
    else
	FBottomGrows(sc,layer,ii);
}

static void CyrilicPhiTop(SplineChar *sc,int layer,ItalicInfo *ii) {
    StemInfo *h;
    SplinePoint *start, *end, *f_start, *f_end;
    SplineSet *ss;
    real transform[6];
    DBounds b;

    FigureFTop(ii);
    if ( ii->f_start==NULL || ii->f_end==NULL )
return;
    SplineCharLayerFindBounds(sc,layer,&b);
    for ( h=sc->vstem; h!=NULL; h=h->next ) {
	if ( !h->tobeused )
    continue;
	FindTopSerifOnStem(sc,layer,h,b.maxy,ii,&start,&end,&ss);
	if ( start==NULL )
    continue;
	SerifRemove(start,end,ss);
	memset(transform,0,sizeof(transform));
	transform[0] = transform[3] = 1;
	transform[4] = start->me.x - ii->f_start->me.x;
	FCopyTrans(ii,transform,&f_start,&f_end);
	start = StemMoveTopEndTo(start,f_start->me.y,true);
	end = StemMoveTopEndTo(end,f_end->me.y,false);
	SplineNextSplice(start,f_start);
	SplinePrevSplice(end,f_end);
    break;
    }
}

static void ItalReplaceWithReferenceTo(SplineChar *sc,int layer, int uni) {
    SplineChar *replacement = SFGetChar(sc->parent,uni,NULL);
    RefChar *newref;

    if ( replacement==NULL )
return;
    SCClearLayer(sc,layer);

    sc->width = replacement->width;

    newref = RefCharCreate();
    free(newref->layers);
    newref->transform[0] = newref->transform[3] = 1;
    newref->layers = NULL;
    newref->layer_cnt = 0;
    newref->sc = replacement;
    sc->layers[layer].refs = newref;
    SCReinstanciateRefChar(sc,newref,layer);
    SCMakeDependent(sc,replacement);
}

static void ItalReplaceWithRotated(SplineChar *sc,int layer, int uni, ItalicInfo *ii) {
    SplineChar *replacement = SFGetChar(sc->parent,uni,NULL);
    RefChar *newref;
    DBounds b;

    if ( replacement==NULL )
return;
    SplineCharLayerFindBounds(replacement,layer,&b);

    SCClearLayer(sc,layer);

    sc->width = replacement->width;

    newref = RefCharCreate();
    free(newref->layers);
    newref->transform[0] = newref->transform[3] = -1;
    newref->transform[4] = replacement->width;
    /* I want the old xheight to be at the baseline */
    newref->transform[5] = ii->x_height;
    newref->layers = NULL;
    newref->layer_cnt = 0;
    newref->sc = replacement;
    sc->layers[layer].refs = newref;
    SCReinstanciateRefChar(sc,newref,layer);
    SCMakeDependent(sc,replacement);
}

static void ItalReplaceWithSmallCaps(SplineChar *sc,int layer, int uni, ItalicInfo *ii) {
    SplineChar *uc = SFGetChar(sc->parent,uni,NULL);
    struct smallcaps small;
    struct genericchange genchange;

    if ( uc==NULL )
return;

    SmallCapsFindConstants(&small, sc->parent, layer);
    if ( uc->ticked ) {
	small.italic_angle = ii->italic_angle;
	small.tan_ia       = ii->tan_ia;
    }

    SCClearLayer(sc,layer);

    memset(&genchange,0,sizeof(genchange));
    genchange.small = &small;
    genchange.gc = gc_smallcaps;
    genchange.extension_for_letters = "sc";
    genchange.extension_for_symbols = "taboldstyle";

    genchange.stem_width_scale  = small.lc_stem_width / small.uc_stem_width;
    genchange.stem_height_scale = genchange.stem_width_scale;
    genchange.v_scale	   = small.xheight / small.capheight;
    genchange.hcounter_scale    = genchange.v_scale;
    genchange.lsb_scale = genchange.rsb_scale = genchange.v_scale;

    ChangeGlyph( sc,uc,layer,&genchange );
}

static void Ital_a_From_d(SplineChar *sc,int layer, ItalicInfo *ii) {
    SplineChar *d = SFGetChar(sc->parent,'d',NULL);
    SplineSet *d_ss, *ss, *sses[4];
    Spline *s, *first, *splines[4], *left, *right;
    SplinePoint *start, *end, *ltemp, *rtemp;
    int scnt, left_is_start;
    double stemwidth, drop, min;
    extended ts[3];

    if ( d==NULL )
return;
    d_ss = SplinePointListCopy(LayerAllSplines(&d->layers[layer]));
    LayerUnAllSplines(&d->layers[ly_fore]);

    if ( d->ticked ) {
	real deskew[6];
	memset(deskew,0,sizeof(deskew));
	deskew[0] = deskew[3] = 1;
	deskew[2] = ii->tan_ia;
	SplinePointListTransform(d_ss,deskew,tpt_AllPoints);
    }

    scnt = 0;
    for ( ss=d_ss; ss!=NULL; ss=ss->next ) {
	first = NULL;
	for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	    if ( first==NULL ) first = s;
	    if (( s->to->me.y<ii->x_height && s->from->me.y>ii->x_height+50) ||
		    ( s->from->me.y<ii->x_height && s->to->me.y>ii->x_height+50)) {
		if ( scnt>=2 ) {
		    SplinePointListsFree(d_ss);
return;
		}
		splines[scnt] = s;
		sses[scnt++] = ss;
	    }
	}
    }
    if ( scnt!=2 || sses[0]!=sses[1] ) {
	SplinePointListsFree(d_ss);
return;
    }
    if ( splines[0]->from->me.x<splines[1]->from->me.x ) {
	left = splines[0];
	right = splines[1];
    } else {
	left = splines[1];
	right = splines[0];
    }
    stemwidth = right->from->me.x - left->from->me.x;
    drop = stemwidth*ii->tan_ia;
    if ( left->from->me.y<left->to->me.y ) {
	min = left->from->me.y;
	left_is_start = true;
    } else {
	min = left->to->me.y;
	left_is_start = false;
    }
    if ( ii->x_height+drop < min ) {
	ltemp = left->from;
    } else {
	CubicSolve(&left->splines[1],ii->x_height+drop,ts);
	ltemp = SplineBisect(left,ts[0]);
    }
    CubicSolve(&right->splines[1],ii->x_height,ts);
    rtemp = SplineBisect(right,ts[0]);
    if ( left_is_start ) {
	start = ltemp;
	end   = rtemp;
    } else {
	start = rtemp;
	end   = ltemp;
    }
    SerifRemove(start,end,sses[0]);
    start->nonextcp = end->noprevcp = true;
    SplineMake(start,end,sc->layers[layer].order2);
 if ( sc->layers[layer].order2 )
  SSCPValidate(sses[0]);

    SCClearLayer(sc,layer);
    sc->layers[layer].splines = d_ss;
    sc->width = d->width;
    SplineCharAutoHint(sc,layer,NULL);
    FigureGoodStems(sc->vstem);
}

static void _SCChangeXHeight(SplineChar *sc,int layer,struct xheightinfo *xi) {
    int l;
    struct position_maps pmaps[7];
    struct fixed_maps fix;
    DBounds b;
    extern int no_windowing_ui;
    int nwi = no_windowing_ui;

    SplineCharLayerFindBounds(sc,layer,&b);
    no_windowing_ui = true;		/* Turn off undoes */

    l=0;
    fix.maps = pmaps;
    fix.maps[l].current = b.miny;
    fix.maps[l++].desired = b.miny;
    if ( b.miny<-xi->xheight_current/4 ) {
	fix.maps[l].current = 0;
	fix.maps[l++].desired = 0;
	if ( xi->serif_height!=0 ) {
	    fix.maps[l].current = xi->serif_height;
	    fix.maps[l++].desired = xi->serif_height;
	}
    }
    if ( xi->serif_height!=0 && b.maxy>=xi->xheight_current ) {
	fix.maps[l].current = xi->xheight_current-xi->serif_height;
	fix.maps[l++].desired = xi->xheight_desired-xi->serif_height;
    }
    if ( b.maxy>=xi->xheight_current ) {
	fix.maps[l].current = xi->xheight_current;
	fix.maps[l++].desired = xi->xheight_desired;
    }
    if ( b.maxy>=xi->xheight_desired ) {
	fix.maps[l].current = b.maxy;
	fix.maps[l++].desired = b.maxy;
    } else {
	fix.maps[l].current = b.maxy;
	fix.maps[l++].desired = b.maxy + xi->xheight_desired - xi->xheight_current;
    }
    fix.cnt = l;
    LowerCaseRemoveSpace(sc->layers[layer].splines,sc->anchor,sc->hstem,1,&fix);
    if ( xi->xheight_current!=0 ) {
	double counter_len = SCFindCounterLen(sc->vstem,b.minx,b.maxx);
	double remove_x = counter_len - xi->xheight_desired*counter_len/xi->xheight_current;
	sc->width -= SmallCapsRemoveSpace(sc->layers[layer].splines,sc->anchor,sc->vstem,0,remove_x,b.minx,b.maxx);
    }
    SplineSetRefigure(sc->layers[layer].splines);
    no_windowing_ui = nwi;
}

static void SCMakeItalic(SplineChar *sc,int layer,ItalicInfo *ii) {
    real skew[6], refpos[6];;
    RefChar *ref;
    extern int no_windowing_ui;
    int nwi = no_windowing_ui;
    int letter_case;

    SCPreserveLayer(sc,layer,true);
    no_windowing_ui = true;		/* Turn off undoes */

    if ( autohint_before_generate && (sc->changedsincelasthinted || sc->vstem==NULL ) &&
	    !sc->manualhints )
	SplineCharAutoHint(sc,layer,NULL);
    else if ( sc->dstem==NULL &&
	    (sc->unicodeenc=='k' || sc->unicodeenc=='v' || sc->unicodeenc=='w' ||
	     sc->unicodeenc=='x' || sc->unicodeenc=='y' || sc->unicodeenc==0x436 ||
	     sc->unicodeenc==0x43a || sc->unicodeenc==0x443 || sc->unicodeenc==0x445 ||
	     sc->unicodeenc==0x3ba || sc->unicodeenc==0x3bb || sc->unicodeenc==0x3bd ||
	     sc->unicodeenc==0x3c7 ))
	SplineCharAutoHint(sc,layer,NULL);	/* I will need diagonal stems */
    letter_case = FigureCase(sc);
    /* Small caps letters look like uppercase letters so should be transformed like them */
    if ( letter_case==cs_lc ) {
	FigureGoodStems(sc->vstem);

	if ( ii->a_from_d && sc->unicodeenc=='a' )
	    Ital_a_From_d(sc,layer,ii);
	if ( ii->a_from_d && sc->unicodeenc==0x430 )
	    ItalReplaceWithReferenceTo(sc,layer,'a');
	if ( ii->cyrl_i   && sc->unicodeenc==0x438 )
	    ItalReplaceWithReferenceTo(sc,layer,'u');
	else if ( ii->cyrl_pi  && sc->unicodeenc==0x43f )
	    ItalReplaceWithReferenceTo(sc,layer,'n');
	else if ( ii->cyrl_te  && sc->unicodeenc==0x442 )
	    ItalReplaceWithReferenceTo(sc,layer,'m');
	else if ( ii->cyrl_dje  && sc->unicodeenc==0x448 )
	    ItalReplaceWithRotated(sc,layer,'m',ii);
	else if ( ii->cyrl_dje  && sc->unicodeenc==0x452 )
	    ItalReplaceWithSmallCaps(sc,layer,'T',ii);
	if ( ii->cyrl_dzhe && sc->unicodeenc==0x45f )
	    ItalReplaceWithReferenceTo(sc,layer,'u');

	if (( ii->f_rotate_top || ii->f_long_tail) &&
		( LikeAnF(sc) || (sc->unicodeenc==0x444 && ii->cyrl_phi)))
	    FBottomTransform(sc,layer,ii);
	else if ( LikePQ(sc) && ii->pq_deserif )
	    DeSerifDescender(sc,layer,ii);
	else if ( ii->transform_bottom_serifs )
	    AddBottomItalicSerifs(sc,layer,ii);

	if ( sc->unicodeenc==0x444 && ii->cyrl_phi )
	    CyrilicPhiTop(sc,layer,ii);
	else if ( ii->transform_top_xh_serifs || ii->transform_top_as_serifs )
	    AddTopItalicSerifs(sc,layer,ii);

	if ( ii->transform_diagon_serifs )
	    AddDiagonalItalicSerifs(sc,layer,ii);
    }

    /* But small caps letters have the same stem sizes as lc so stems should be transformed like lc */
    ItalicCompress(sc,layer,letter_case==cs_lc? &ii->lc :
			    letter_case==cs_uc? &ii->uc :
			    letter_case==cs_smallcaps? &ii->lc :
						&ii->neither);
    if (( letter_case==cs_lc || letter_case==cs_smallcaps ) &&
	    sc->layers[layer].splines!=NULL &&
	    ii->xheight_percent>0 && ii->xheight_percent!=1.0 ) {
	struct xheightinfo xi;
	memset(&xi,0,sizeof(xi));
	xi.xheight_current = ii->x_height;
	xi.xheight_desired = ii->xheight_percent*ii->x_height;
	_SCChangeXHeight(sc,layer,&xi);
    }

    memset(skew,0,sizeof(skew));
    skew[0] = skew[3] = 1;
    skew[2] = -ii->tan_ia;

    SplinePointListTransform( sc->layers[layer].splines, skew, tpt_AllPoints );
    for ( ref=sc->layers[layer].refs; ref!=NULL; ref = ref->next ) {
	memset(refpos,0,sizeof(refpos));
	refpos[0] = refpos[3] = 1;
	refpos[4] = -ii->tan_ia*ref->transform[5];
	ref->transform[4] += refpos[4];
	SplinePointListTransform(ref->layers[0].splines,refpos,tpt_AllPoints);
    }

    StemInfosFree(sc->hstem);  sc->hstem = NULL;
    StemInfosFree(sc->vstem);  sc->vstem = NULL;
    DStemInfosFree(sc->dstem); sc->dstem = NULL;
    no_windowing_ui = nwi;
    SCRound2Int(sc,layer, 1.0);		/* This calls SCCharChangedUpdate(sc,layer); */
}

static int IsSelected(FontViewBase *fv,SplineChar *sc) {
    int enc = fv->map->backmap[sc->orig_pos];
    if ( enc==-1 )
return( false );

return( fv->selected[enc]);
}

static int FVMakeAllItalic(FontViewBase *fv,SplineChar *sc,int layer,ItalicInfo *ii) {
    RefChar *ref;

    sc->ticked = true;
    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	if ( !ref->sc->ticked && IsSelected(fv,ref->sc)) {
	    if ( !FVMakeAllItalic(fv,ref->sc,layer,ii))
return( false );
	}
    }
    SCMakeItalic(sc,layer,ii);
return( ff_progress_next());
}

static double SerifExtent(SplineChar *sc,int layer,int is_bottom) {
    StemInfo *h;
    SplineSet *ss;
    SplinePoint *sp, *start, *end;
    double min=0, max=0;
    ItalicInfo tempii;

    if ( sc==NULL )
return( 0 );
    memset(&tempii,0,sizeof(tempii));
    tempii.serif_extent = 1000;

    if ( autohint_before_generate && (sc->changedsincelasthinted || sc->vstem==NULL) &&
	    !sc->manualhints )
	SplineCharAutoHint(sc,layer,NULL);
    FigureGoodStems(sc->vstem);
    for ( h=sc->vstem; h!=NULL; h=h->next ) if ( h->tobeused ) {
	if ( is_bottom )
	    FindBottomSerifOnStem(sc,layer,h,0,&tempii,&start,&end,&ss);
	else {
	    DBounds b;
	    SplineCharLayerFindBounds(sc,layer,&b);
	    FindTopSerifOnStem(sc,layer,h,b.maxy,&tempii,&start,&end,&ss);
	}
	if ( start==NULL )
    continue;
	for ( sp=start; sp!=end; sp=sp->next->to ) {
	    if ( sp->me.x - (h->start+h->width) > max )
		max = sp->me.x - (h->start+h->width);
	    else if ( h->start - sp->me.x > min )
		min = h->start - sp->me.x;
	}
	if ( min>max )
return( min );
	if ( max!=0 )
return( max );
    }
return( 0 );
}

static double SCSerifHeight(SplineChar *sc,int layer) {
    StemInfo *h;
    SplineSet *ss;
    SplinePoint *sp, *nsp, *start, *end;
    ItalicInfo tempii;

    if ( sc==NULL )
return( 0 );
    memset(&tempii,0,sizeof(tempii));
    tempii.serif_extent = 1000;

    if ( autohint_before_generate && (sc->changedsincelasthinted || sc->vstem==NULL) &&
	    !sc->manualhints )
	SplineCharAutoHint(sc,layer,NULL);
    FigureGoodStems(sc->vstem);
    for ( h=sc->vstem; h!=NULL; h=h->next ) if ( h->tobeused ) {
	FindBottomSerifOnStem(sc,layer,h,0,&tempii,&start,&end,&ss);
	if ( start==NULL )
    continue;
	for ( sp=start; sp!=end; sp=nsp ) {
	    nsp = sp->next->to;
	    if ( sp->me.y>5 && sp->me.y>=nsp->me.y-1 && sp->me.y<=nsp->me.y+1 )
return( sp->me.y );
	}
    }
return( 0 );
}

static void InitItalicConstants(SplineFont *sf, int layer, ItalicInfo *ii) {
    int i,cnt;
    double val;

    ii->tan_ia = tan(ii->italic_angle * 3.1415926535897932/180.0 );

    ii->x_height	  = SFXHeight  (sf,layer,false);
    ii->ascender_height   = SFAscender (sf,layer,false);
    ii->pq_depth	  = SFDescender(sf,layer,false);

    for ( i=cnt=0; lc_botserif_str[i]!=0; ++i ) {
	val = SerifExtent(SFGetChar(sf,lc_botserif_str[i],NULL),layer,true);
	if ( val>ii->serif_extent )
	    ii->serif_extent = val;
    }
    for ( i=cnt=0; lc_topserif_str[i]!=0; ++i ) {
	val = SerifExtent(SFGetChar(sf,lc_topserif_str[i],NULL),layer,false);
	if ( val>ii->serif_extent )
	    ii->serif_extent = val;
    }

    ii->emsize = sf->ascent+sf->descent;
    ii->order2 = sf->layers[layer].order2;
    ii->sf = sf;
    ii->layer = layer;
}

static void StuffFree(SplinePoint *from, SplinePoint *to1, SplinePoint *to2) {
    SplinePoint *mid, *spnext;

    if ( from==NULL )
return;

    for ( mid=from; mid!=to1 && mid!=to2; mid=spnext ) {
	spnext = mid->next->to;
	SplinePointFree(mid);
	SplineFree(spnext->prev);
    }
    SplinePointFree(mid);
}

static void ItalicInfoFreeContents(ItalicInfo *ii) {
    StuffFree(ii->f_start,ii->f_end,NULL);
    StuffFree(ii->ff_start1,ii->ff_end1,ii->ff_end2);
    StuffFree(ii->ff_start2,ii->ff_end1,ii->ff_end2);
    memset(&ii->tan_ia,0,sizeof(ItalicInfo) - (((char *) &ii->tan_ia) - ((char *) ii)));
}

void MakeItalic(FontViewBase *fv,CharViewBase *cv, ItalicInfo *ii) {
    int cnt, enc, gid;
    SplineChar *sc;
    SplineFont *sf = fv!=NULL ? fv->sf : cv->sc->parent;
    int layer = fv!=NULL ? fv->active_layer : CVLayer(cv);
    extern int detect_diagonal_stems;
    int dds = detect_diagonal_stems;

    detect_diagonal_stems = true;
    InitItalicConstants(sf,layer,ii);

    if ( cv!=NULL )
	SCMakeItalic(cv->sc,layer,ii);
    else {
	cnt=0;

	for ( enc=0; enc<fv->map->enccount; ++enc ) {
	    if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL ) {
		++cnt;
		sc->ticked = false;
	    }
	}
	if ( cnt!=0 ) {
	    ff_progress_start_indicator(10,_("Italic"),
		_("Italic Conversion"),NULL,cnt,1);

	    for ( enc=0; enc<fv->map->enccount; ++enc ) {
		if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] &&
			(sc=sf->glyphs[gid])!=NULL && !sc->ticked ) {
		    if ( !FVMakeAllItalic(fv,sc,layer,ii))
	    break;
		}
	    }
	    ff_progress_end_indicator();
	}
    }
    detect_diagonal_stems = dds;
    ItalicInfoFreeContents(ii);
}

/* ************************************************************************** */
/* ***************************** Change X-Height **************************** */
/* ************************************************************************** */

void InitXHeightInfo(SplineFont *sf, int layer, struct xheightinfo *xi) {
    int i, j, cnt, besti;
    double val;
    const int MW=100;
    struct widths { double width, total; } widths[MW];

    memset(xi,0,sizeof(*xi));
    xi->xheight_current = SFXHeight(sf,layer,false);
    for ( i=cnt=0; lc_botserif_str[i]!=0; ++i ) {
	val = SCSerifHeight(SFGetChar(sf,lc_botserif_str[i],NULL),layer);
	if ( val!=0 ) {
	    for ( j=0; j<cnt; ++j ) {
		if ( widths[j].width==val ) {
		    ++widths[j].total;
	    break;
		}
	    }
	    if ( j==cnt && j<MW ) {
		widths[j].width = val;
		widths[j].total = 1;
	    }
	}
    }

    besti = -1;
    for ( j=0 ; j<cnt; ++j ) {
	if ( besti==-1 )
	    besti = j;
	else if ( widths[j].total >= widths[besti].total )
	    besti = j;
    }
    if ( besti!=-1 )
	xi->serif_height = widths[besti].total;
}

static void SCChangeXHeight(SplineChar *sc,int layer,struct xheightinfo *xi) {
    const unichar_t *alts;

    if ( sc->layers[layer].refs!=NULL &&
			((alts = SFGetAlternate(sc->parent,sc->unicodeenc,sc,true))!=NULL &&
			 alts[1]!=0 ))
	SCBuildComposit(sc->parent,sc,layer,NULL,true);
    else {
	SCPreserveLayer(sc,layer,true);
	_SCChangeXHeight(sc,layer,xi);
	SCCharChangedUpdate(sc,layer);
    }
}

static int FVChangeXHeight(FontViewBase *fv,SplineChar *sc,int layer,struct xheightinfo *xi) {
    RefChar *ref;

    sc->ticked = true;
    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	if ( !ref->sc->ticked && IsSelected(fv,ref->sc)) {
	    if ( !FVChangeXHeight(fv,ref->sc,layer,xi))
return( false );
	}
    }
    SCChangeXHeight(sc,layer,xi);
return( ff_progress_next());
}

void ChangeXHeight(FontViewBase *fv,CharViewBase *cv, struct xheightinfo *xi) {
    int cnt, enc, gid;
    SplineChar *sc;
    SplineFont *sf = fv!=NULL ? fv->sf : cv->sc->parent;
    int layer = fv!=NULL ? fv->active_layer : CVLayer(cv);
    extern int detect_diagonal_stems;
    int dds = detect_diagonal_stems;

    detect_diagonal_stems = true;

    if ( cv!=NULL )
	SCChangeXHeight(cv->sc,layer,xi);
    else {
	cnt=0;

	for ( enc=0; enc<fv->map->enccount; ++enc ) {
	    if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL ) {
		++cnt;
		sc->ticked = false;
	    }
	}
	if ( cnt!=0 ) {
	    ff_progress_start_indicator(10,_("Change X-Height"),
		_("Change X-Height"),NULL,cnt,1);

	    for ( enc=0; enc<fv->map->enccount; ++enc ) {
		if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] &&
			(sc=sf->glyphs[gid])!=NULL && !sc->ticked ) {
		    if ( !FVChangeXHeight(fv,sc,layer,xi))
	    break;
		}
	    }
	    ff_progress_end_indicator();
	}
    }
    detect_diagonal_stems = dds;
}
