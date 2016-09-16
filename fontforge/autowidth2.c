/* Copyright (C) 2009-2012 by George Williams */
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

#define DENOM_FACTOR_OF_EMSIZE	50.0

#include "autowidth2.h"
#include "edgelist2.h"

static int aw2_bbox_separation(AW_Glyph *g1, AW_Glyph *g2, AW_Data *all) {
    int j;
    int imin_y, imax_y;
    real tot, cnt;
    real denom;
    /* the goal is to give a weighted average that expresses the visual */
    /*  separation between two glyphs when they are placed so their bounding */
    /*  boxes are adjacent. The separation between two rectangles would be 0 */
    /*  While the separation between "T" and "o" would be fairly large */
    /* The trick is to guess a good weighting function. My guess is that */
    /*  things that look close are more important than those which look far */
    /*  So "T" and "O" should be dominated by the crossbar of the "T"... */
#if !defined(_NO_PYTHON)

    if ( PyFF_GlyphSeparationHook!=NULL )
return( PyFF_GlyphSeparation(g1,g2,all) );
#endif

    imin_y = g2->imin_y > g1->imin_y ? g2->imin_y : g1->imin_y;
    imax_y = g2->imax_y < g1->imax_y ? g2->imax_y : g1->imax_y;
    if ( imax_y < imin_y )		/* no overlap. ie grave and "a" */
return( 0 );
    denom = all->denom;
    tot = cnt = 0;
    for ( j=imin_y ; j<imax_y; ++j ) {
	if ( g2->left[j-g2->imin_y] < 32767 && g1->right[j-g1->imin_y] > -32767 ) {
	    /* beware of gaps such as those in "i" or "aaccute" */
	    real sep = g2->left[j-g2->imin_y] - g1->right[j-g1->imin_y];
	    real weight = 1.0/(sep + denom);
	    weight *= weight;

	    tot += weight*sep;
	    cnt += weight;
	}
    }
    if ( cnt!=0 )
	tot /= cnt;
return( rint( tot ) );
}

static void aw2_figure_lsb(int right_index, AW_Data *all) {
    int i;
    AW_Glyph *me, *other;
    int lsb, tot;
    int *vpt = all->visual_separation + right_index;

    lsb=0;
    me = &all->glyphs[right_index];
    for ( i=0; i<all->gcnt; ++i ) {
	other = &all->glyphs[i];
	tot = all->desired_separation - vpt[i*all->gcnt] - other->rsb;
	if ( tot < all->min_sidebearing )
	    tot = all->min_sidebearing;
	else if ( tot > all->max_sidebearing )
	    tot = all->max_sidebearing;
	lsb += tot;
    }
    if ( i!=0 )
	lsb = (lsb + i/2)/i;
    lsb = rint((3 * lsb + me->lsb)/4.0);
    me->nlsb = lsb;
}

static void aw2_figure_rsb(int left_index, AW_Data *all) {
    int i;
    AW_Glyph *me, *other;
    int rsb, tot;
    int *vpt = all->visual_separation + left_index*all->gcnt;

    rsb=0;
    me = &all->glyphs[left_index];
    for ( i=0; i<all->gcnt; ++i ) {
	other = &all->glyphs[i];
	tot = all->desired_separation - vpt[i] - other->lsb;
	if ( tot < all->min_sidebearing )
	    tot = all->min_sidebearing;
	else if ( tot > all->max_sidebearing )
	    tot = all->max_sidebearing;
	rsb += tot;
    }
    if ( i!=0 )
	rsb = (rsb + i/2)/i;
    rsb = rint((3 * rsb + me->rsb)/4.0);
    me->nrsb = rsb;
}

static void aw2_figure_all_sidebearing(AW_Data *all) {
    int i,j;
    AW_Glyph *me, *other;
    real transform[6], half;
    int width, changed;
    uint8 *rsel = calloc(all->fv->map->enccount,sizeof(char));
    real denom = (all->sf->ascent + all->sf->descent)/DENOM_FACTOR_OF_EMSIZE;
    int ldiff, rdiff;

    all->denom = denom;
    all->visual_separation = malloc(all->gcnt*all->gcnt*sizeof(int));
    for ( i=0; i<all->gcnt; ++i ) {
	int *vpt = all->visual_separation + i*all->gcnt;
	me = &all->glyphs[i];
	for ( j=0; j<all->gcnt; ++j ) {
	    other = &all->glyphs[j];
	    vpt[j] = aw2_bbox_separation(me,other,all);
	}
    }

    half = all->desired_separation/2;
    for ( i=0; i<all->gcnt; ++i ) {
	me = &all->glyphs[i];	
	me->lsb = me->rsb = half;
    }

    for ( j=0; j<all->loop_cnt; ++j ) {
	for ( i=0; i<all->gcnt; ++i )
	    aw2_figure_lsb(i,all);
	for ( i=0; i<all->gcnt; ++i )
	    aw2_figure_rsb(i,all);
	for ( i=0; i<all->gcnt; ++i ) {
	    AW_Glyph *me = &all->glyphs[i];
	    me->rsb = me->nrsb;
	    me->lsb = me->nlsb;
	}
    }
    free(all->visual_separation); all->visual_separation = NULL;

    if ( all->normalize ) {
	/* This is the dummy flat edge we added. We want the separation between */
	/*  two adjacent flat edges to be desired_separation */
	me = &all->glyphs[all->gcnt-1];
	if ( me->lsb+me->rsb != all->desired_separation && me->sc==NULL ) {
	    if ( me->lsb+me->rsb!=0 ) {
		ldiff = (all->desired_separation-(me->lsb+me->rsb)) * me->lsb/(me->lsb+me->rsb);
	    } else {
		ldiff = all->desired_separation/2;
	    }
	    rdiff = (all->desired_separation-(me->lsb+me->rsb)) - ldiff;
	    for ( i=0; (me = &all->glyphs[i])->sc!=NULL; ++i ) {
		me->lsb += ldiff;
		me->rsb += rdiff;
	    }
	}
    }

    transform[0] = transform[3] = 1.0;
    transform[1] = transform[2] = transform[5] = 0;
    for ( i=0; (me = &all->glyphs[i])->sc!=NULL; ++i ) {
	changed = 0;
	if ( me->lsb != me->bb.minx ) {
	    transform[4] = me->lsb-me->bb.minx;
	    FVTrans(all->fv,me->sc,transform,rsel,false);
	    changed = true;
	}
	width = me->lsb + me->rsb + rint(me->bb.maxx - me->bb.minx);
	if ( me->sc->width != width ) {
	    SCPreserveWidth(me->sc);
	    SCSynchronizeWidth(me->sc,width,me->sc->width,all->fv);
	    changed = true;
	}
	if ( changed )
	    SCCharChangedUpdate(me->sc,ly_none);
    }
    free(rsel);
}

static int ak2_figure_kern(AW_Glyph *g1, AW_Glyph *g2, AW_Data *all) {
    int sep = aw2_bbox_separation(g1,g2,all);
    sep += g2->bb.minx + g1->sc->width - g1->bb.maxx;
return( all->desired_separation - sep );
}

static int ak2_figure_touch(AW_Glyph *g1, AW_Glyph *g2, AW_Data *all) {
    int j;
    int imin_y, imax_y;
    real smallest, tot;

    imin_y = g2->imin_y > g1->imin_y ? g2->imin_y : g1->imin_y;
    imax_y = g2->imax_y < g1->imax_y ? g2->imax_y : g1->imax_y;
    if ( imax_y < imin_y )		/* no overlap. ie grave and "a" */
return( - (g2->bb.minx + g1->sc->width - g1->bb.maxx) );
    smallest = 0x7fff;
    for ( j=imin_y ; j<imax_y; ++j ) {
	tot = g2->left[j-g2->imin_y] - g1->right[j-g1->imin_y];
	if ( tot<smallest )
	    smallest = tot;
    }
    if ( smallest == 0x7fff )	/* Overlaps only in gaps, "i" and something between the base and the dot */
return( - (g2->bb.minx + g1->sc->width - g1->bb.maxx) );

    smallest += g2->bb.minx + g1->sc->width - g1->bb.maxx;
return( all->desired_separation - smallest );
}

static int ak2_figure_kernclass(int *class1, int *class2, AW_Data *all) {
    int h,i;
    real subtot, tot, cnt2;

    tot = cnt2 = 0;
    for ( h=0; class1[h]!=-1; ++h ) {
	AW_Glyph *g1 = &all->glyphs[class1[h]];
	for ( i = 0; class2[i]!=-1; ++i ) {
	    AW_Glyph *g2 = &all->glyphs[class2[i]];
	    subtot = aw2_bbox_separation(g1,g2,all);
	    tot += g2->bb.minx + g1->sc->width - g1->bb.maxx + subtot;
	    ++cnt2;
	}
    }
    if ( cnt2==0 )
return( 0 );

    if ( cnt2!=0 )
	tot /= cnt2;
return( all->desired_separation - tot );
}

static int ak2_figure_touchclass(int *class1, int *class2, AW_Data *all) {
    int h,i,j;
    int imin_y, imax_y;
    real smallest, smaller, tot;

    smallest = 0x7fff;
    for ( h=0; class1[h]!=-1; ++h ) {
	AW_Glyph *g1 = &all->glyphs[class1[h]];
	for ( i = 0; class2[i]!=-1; ++i ) {
	    AW_Glyph *g2 = &all->glyphs[class2[i]];
	    imin_y = g2->imin_y > g1->imin_y ? g2->imin_y : g1->imin_y;
	    imax_y = g2->imax_y < g1->imax_y ? g2->imax_y : g1->imax_y;
	    if ( imax_y < imin_y ) {		/* no overlap. ie grave and "a" */
		if ( smallest < - (g2->bb.minx + g1->sc->width - g1->bb.maxx) )
		    smallest = - (g2->bb.minx + g1->sc->width - g1->bb.maxx);
	continue;
	    }
	    smaller = 0x7fff;
	    for ( j=imin_y ; j<imax_y; ++j ) {
		tot = g2->left[j-g2->imin_y] - g1->right[j-g1->imin_y];
		if ( tot<smaller )
		    smaller = tot;
	    }
	    if ( smaller == 0x7fff ) {
		if ( smallest < - (g2->bb.minx + g1->sc->width - g1->bb.maxx) )
		    smallest = - (g2->bb.minx + g1->sc->width - g1->bb.maxx);
	continue;	/* Overlaps only in gaps */
	    }
	    smaller += g2->bb.minx + g1->sc->width - g1->bb.maxx;
	    if ( smaller<smallest )
		smallest = smaller;
	}
    }
    if ( smallest==0x7fff )
return( 0 );

return( all->desired_separation - smallest );
}

static double MonotonicFindY(Monotonic *m,double test,double old_t) {
    double tstart, tend, t;

    tstart = m->tstart; tend = m->tend;
    if ( old_t!=-1 ) {
	if ( m->yup )
	    tstart = old_t;
	else
	    tend = old_t;
    }
    t = IterateSplineSolve(&m->s->splines[1],tstart,tend,test);
return( t );
}

static void aw2_findedges(AW_Glyph *me, AW_Data *all) {
    Monotonic *ms, *m;
    real ytop, ybottom;
    real x, xmin, xmax;
    int i;
    double t;
    Spline1D *msp;
    SplineSet *base;

    me->imin_y = floor(me->bb.miny/all->sub_height);
    me->imax_y = ceil (me->bb.maxy/all->sub_height);
    me->left = malloc((me->imax_y-me->imin_y+1)*sizeof(short));
    me->right = malloc((me->imax_y-me->imin_y+1)*sizeof(short));

    base = LayerAllSplines(&me->sc->layers[all->layer]);
    ms = SSsToMContours(base,over_remove);	/* over_remove is an arcane way of saying: Look at all contours, not just selected ones */
    LayerUnAllSplines(&me->sc->layers[all->layer]);

    ytop = me->imin_y*all->sub_height;
    for ( m=ms; m!=NULL; m=m->linked ) {
	m->t = -1;
	if ( m->b.miny<=ytop )		/* can't be less than, but... */
	    m->t = MonotonicFindY(m,ytop,-1);
    }
    for ( i=me->imin_y; i<=me->imax_y; ++i ) {
	ybottom = ytop; ytop += all->sub_height;
	xmin = 1e10; xmax = -1e10;
	for ( m=ms; m!=NULL; m=m->linked ) {
	    if ( m->b.maxy<ybottom || m->b.miny>ytop || m->b.maxy==m->b.miny )
	continue;
	    if ( (t = m->t)==-1 )
		t = MonotonicFindY(m,m->b.miny,-1);
	    msp = &m->s->splines[0];
	    if ( t!=-1 ) {
		x = ((msp->a*t + msp->b)*t+msp->c)*t+msp->d;
		if ( x>xmax ) xmax = x;
		if ( x<xmin ) xmin = x;
	    }
	    if ( ytop<m->b.maxy )
		t = m->t = MonotonicFindY(m,ytop,t);
	    else {
		m->t = -1;
		t = MonotonicFindY(m,m->b.maxy,t);
	    }
	    if ( t!=-1 ) {
		x = ((msp->a*t + msp->b)*t+msp->c)*t+msp->d;
		if ( x>xmax ) xmax = x;
		if ( x<xmin ) xmin = x;
	    }
	}
	if ( xmin>1e9 ) {
	    /* Glyph might have a gap (as "i" or ":" do) */
	    me->left[i-me->imin_y] = 32767 /* floor((me->bb.maxx - me->bb.minx)/2)*/;
	    me->right[i-me->imin_y] = -32767 /* floor(-(me->bb.maxx - me->bb.minx)/2)*/;
	} else {
	    me->left[i-me->imin_y] = floor(xmin - me->bb.minx);
	    me->right[i-me->imin_y] = floor(xmax - me->bb.maxx);	/* This is always non-positive, so floor will give the bigger absolute value */
	}
    }
    FreeMonotonics(ms);
}

static void aw2_dummyedges(AW_Glyph *flat,AW_Data *all) {
    int i;
    int imin_y = 32000, imax_y = -32000;

    if ( all!=NULL ) {
	for ( i=0; i<all->gcnt; ++i ) {
	    AW_Glyph *test = &all->glyphs[i];
	    if ( test->imin_y < imin_y ) imin_y = test->imin_y;
	    if ( test->imax_y > imax_y ) imax_y = test->imax_y;
	}
	if ( imin_y == 32000 ) {
	    imin_y = floor(-all->sf->descent/all->sub_height);
	    imax_y = ceil ( all->sf->ascent /all->sub_height);
	}
	flat->imin_y = imin_y; flat->imax_y = imax_y;
    }
    flat->left = calloc((flat->imax_y-flat->imin_y+1),sizeof(short));
    flat->right = calloc((flat->imax_y-flat->imin_y+1),sizeof(short));
}

static void AWGlyphFree( AW_Glyph *me) {
    free(me->left);
    free(me->right);
#if !defined(_NO_PYTHON)
    FFPy_AWGlyphFree(me);
#endif
}

static void aw2_handlescript(AW_Data *all) {
    int i;
    AW_Glyph *me;

    for ( i=0; (me = &all->glyphs[i])->sc!=NULL; ++i )
	aw2_findedges(me,all);
    aw2_dummyedges(me,all); ++all->gcnt;
    aw2_figure_all_sidebearing(all);
    for ( i=0; i<all->gcnt; ++i )
	AWGlyphFree( &all->glyphs[i] );
}

SplineChar ***GlyphClassesFromNames(SplineFont *sf,char **classnames,
	int class_cnt ) {
    SplineChar ***classes = calloc(class_cnt,sizeof(SplineChar **));
    int i, pass, clen;
    char *end, ch, *pt, *cn;
    SplineChar *sc;

    for ( i=0; i<class_cnt; ++i ) {
	cn = copy(classnames[i]==NULL ? "" : classnames[i]);
	for ( pass=0; pass<2; ++pass ) {
	    clen = 0;
	    for ( pt = cn; *pt; pt = end+1 ) {
		while ( *pt==' ' ) ++pt;
		if ( *pt=='\0' )
	    break;
		end = strchr(pt,' ');
		if ( end==NULL )
		    end = pt+strlen(pt);
		ch = *end;
		if ( pass ) {
		    *end = '\0';
		    sc = SFGetChar(sf,-1,pt);
		    if ( sc!=NULL ) {
			classes[i][clen++] = sc;
		    }
		    *end = ch;
		} else
		    ++clen;
		if ( ch=='\0' )
	    break;
	    }
	    if ( pass )
		classes[i][clen] = NULL;
	    else
		classes[i] = malloc((clen+1)*sizeof(SplineChar *));
	}
	if ( cn != NULL ) free( cn ) ; cn = NULL ;
    }
return( classes );
}

void AutoWidth2(FontViewBase *fv,int separation,int min_side,int max_side,
	int chunk_height, int loop_cnt) {
    struct scriptlist {
	uint32 script;
	AW_Glyph *glyphs;
	int gcnt;
    } *scripts;
    int scnt, smax;
    int enc, gid, s, i;
    SplineFont *sf = fv->sf;
    SplineChar *sc;
    AW_Data all;

    if ( chunk_height <= 0 )
	chunk_height = (sf->ascent + sf->descent)/100;
    if ( loop_cnt <= 0 )
	loop_cnt = 4;

    scnt = 0; smax = 10;
    scripts = malloc(smax*sizeof(struct scriptlist));

    for ( gid=0; gid<sf->glyphcnt; ++gid ) {
	if ( (sc = sf->glyphs[gid])!=NULL )
	    sc->ticked = false;
    }
    for ( enc=0; enc<fv->map->enccount; ++enc ) {
	if ( fv->selected[enc] && (gid=fv->map->map[enc])!=-1 &&
		(sc = sf->glyphs[gid])!=NULL && ! sc->ticked &&
		HasUseMyMetrics(sc,fv->active_layer)==NULL ) {
		/* If Use My Metrics is set, then we can't change the width (which we grab from a refchar) */
	    uint32 script;
	    script = SCScriptFromUnicode(sc);
	    for ( s=0; s<scnt; ++s ) {
		if ( scripts[s].script == script )
	    break;
	    }
	    if ( s==scnt ) {
		if ( scnt>=smax )
		    scripts = realloc(scripts,(smax+=10)*sizeof(struct scriptlist));
		memset(&scripts[scnt],0,sizeof(struct scriptlist));
		scripts[scnt].script = script;
		scripts[scnt].glyphs = calloc(sf->glyphcnt+1,sizeof(AW_Glyph));
		++scnt;
	    }
	    i = scripts[s].gcnt;
	    scripts[s].glyphs[i].sc = sc;
	    SplineCharLayerFindBounds(sc,fv->active_layer,&scripts[s].glyphs[i].bb);
	    if ( scripts[s].glyphs[i].bb.minx<-16000 || scripts[s].glyphs[i].bb.maxx>16000 ||
		    scripts[s].glyphs[i].bb.miny<-16000 || scripts[s].glyphs[i].bb.maxy>16000 )
		ff_post_notice(_("Glyph too big"),_("%s has a bounding box which is too big for this algorithm to work. Ignored."),sc->name);
	    else
		++scripts[s].gcnt;
	}
    }

    memset(&all,0,sizeof(all));
    all.sf = sf;
    all.fv = fv;
    all.layer = fv->active_layer;
    all.normalize = true;

    all.sub_height = chunk_height;
    all.loop_cnt = loop_cnt;
    all.desired_separation = separation;
    all.min_sidebearing = min_side;
    all.max_sidebearing = max_side;

    for ( s=0; s<scnt; ++s ) {
	memset(&scripts[s].glyphs[scripts[s].gcnt], 0, sizeof(AW_Glyph));
	all.glyphs = scripts[s].glyphs;
	all.gcnt   = scripts[s].gcnt;
	aw2_handlescript(&all);
	free(all.glyphs);
    }
    free(scripts);
#if !defined(_NO_PYTHON)
    FFPy_AWDataFree(&all);
#endif		/* PYTHON */
}

void GuessOpticalOffset(SplineChar *sc,int layer,real *_loff, real *_roff,
	int chunk_height ) {
    /* Given a glyph make a guess as to where the eye thinks the left/right */
    /*  edges of the glyph are to be found */
    /* On an "I" with no serifs, the edges are the right and left side bearings*/
    /* But on an "O" the edge appears to be a bit closer to the center than the side bearing */
    /* The left edge will always appear to be a non-negative (x) distance from the lsb */
    /* the right edge will always appear to be a non-positve distance from the rsb */
    SplineFont *sf = sc->parent;
    AW_Data all;
    AW_Glyph glyph, edge;
    real loff, roff;
    RefChar *r = HasUseMyMetrics(sc,layer);

    if ( r!=NULL )
	    sc = r->sc;

    if ( chunk_height <= 0 )
	chunk_height = (sf->ascent + sf->descent)/200;

    memset(&all,0,sizeof(all));
    memset(&glyph,0,sizeof(glyph));
    memset(&edge,0,sizeof(edge));
    all.layer = layer;
    all.sub_height = chunk_height;
    all.sf = sf;
    all.denom = (sf->ascent + sf->descent)/DENOM_FACTOR_OF_EMSIZE;

    glyph.sc = sc;
    SplineCharLayerFindBounds(sc,layer,&glyph.bb);
    if ( glyph.bb.minx<-16000 || glyph.bb.maxx>16000 ||
	    glyph.bb.miny<-16000 || glyph.bb.maxy>16000 ) {
	ff_post_notice(_("Glyph too big"),_("%s has a bounding box which is too big for this algorithm to work. Ignored."),sc->name);
	*_loff = glyph.bb.minx;
	*_roff = sc->width - glyph.bb.maxx;
    } else {
	aw2_findedges(&glyph, &all);
	edge.imin_y = glyph.imin_y; edge.imax_y = glyph.imax_y;
	aw2_dummyedges(&edge,NULL);
	loff = aw2_bbox_separation(&edge,&glyph,&all);
	roff = aw2_bbox_separation(&glyph,&edge,&all);
	*_loff = glyph.bb.minx + loff;
	*_roff = sc->width - (glyph.bb.maxx - roff);
	AWGlyphFree( &glyph );
	AWGlyphFree( &edge );
    }
#if !defined(_NO_PYTHON)
    FFPy_AWDataFree(&all);
#endif		/* PYTHON */
}

void AutoKern2(SplineFont *sf, int layer,SplineChar **left,SplineChar **right,
	struct lookup_subtable *into,
	int separation, int min_kern, int from_closest_approach, int only_closer,
	int chunk_height,
	void (*addkp)(void *data,SplineChar *left,SplineChar *r,int off),
	void *data) {
    AW_Data all;
    AW_Glyph *glyphs;
    int i,cnt,k, kern;
    SplineChar *sc;
    KernPair *last, *kp, *next;
    int is_l2r = !(into->lookup->lookup_flags & pst_r2l);
    /* Normally, kerning is based on some sort of average distance between */
    /*  two glyphs, but sometimes it is useful to kern so much that the glyphs*/
    /*  just touch. If that is desired then set from_closest_approach. */

    if ( chunk_height <= 0 )
	chunk_height = (sf->ascent + sf->descent)/200;
    if ( separation==0 && !from_closest_approach ) {
	if ( into->separation==0 && !into->kerning_by_touch ) {
	    into->separation = sf->width_separation;
	    if ( sf->width_separation==0 )
		into->separation = 15*(sf->ascent+sf->descent)/100;
	    separation = into->separation;
	} else {
	    separation = into->separation;
	    from_closest_approach = into->kerning_by_touch;
	    min_kern = into->minkern;
	    only_closer = into->onlyCloser;
	}
    }

    memset(&all,0,sizeof(all));
    all.layer = layer;
    all.sub_height = chunk_height;
    all.desired_separation = separation;
    all.sf = sf;
    all.denom = (sf->ascent + sf->descent)/DENOM_FACTOR_OF_EMSIZE;

    /* ticked means a left glyph, ticked2 a right (a glyph can be both) */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL )
	sc->ticked = sc->ticked2 = false;
    for ( i=0; (sc = left[i])!=NULL; ++i )
	sc->ticked = true;
    for ( i=0; (sc = right[i])!=NULL; ++i )
	sc->ticked2 = true;
    for ( cnt=i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL )
	if ( sc->ticked || sc->ticked2 )
	    ++cnt;

    glyphs = calloc(cnt+1,sizeof(AW_Glyph));
    for ( cnt=i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	if ( sc->ticked || sc->ticked2 ) {
	    SplineCharLayerFindBounds(sc,layer,&glyphs[cnt].bb);
	    if ( glyphs[cnt].bb.minx<-16000 || glyphs[cnt].bb.maxx>16000 ||
		    glyphs[cnt].bb.miny<-16000 || glyphs[cnt].bb.maxy>16000 )
		ff_post_notice(_("Glyph too big"),_("%s has a bounding box which is too big for this algorithm to work. Ignored."),sc->name);
	    else {
		glyphs[cnt].sc = sc;
		aw2_findedges(&glyphs[cnt++], &all);
	    }
	}
    }

    /* remove all current kern pairs in this subtable which include the specified glyphs */
    if ( addkp==NULL ) {
	for ( i=0; (sc = left[i])!=NULL; ++i ) {
	    for ( last=NULL, kp=sc->kerns; kp!=NULL; kp = next ) {
		next = kp->next;
		if ( kp->subtable==into && kp->sc->ticked2 ) {
		    if ( last==NULL )
			sc->kerns = next;
		    else
			last->next = next;
		    kp->next = NULL;
		    KernPairsFree(kp);
		} else
		    last = kp;
	    }
	}
    }

    all.glyphs = glyphs;
    all.gcnt = cnt;
    for ( i=0; i<cnt; ++i ) {
	AW_Glyph *g1 = &glyphs[i];
	if ( !g1->sc->ticked )
    continue;
	for ( k=0; k<cnt; ++k ) {
	    AW_Glyph *g2 = &glyphs[k];
	    if ( !g2->sc->ticked2 )
	continue;
	    if ( from_closest_approach )
		kern = rint( ak2_figure_touch(g1,g2,&all));
	    else {
		kern = rint( ak2_figure_kern(g1,g2,&all));
		if ( kern<min_kern && kern>-min_kern )
		    kern = 0;
	    }
	    if ( only_closer && kern>0 )
		kern=0;
	    if ( kern!=0 ) {
		if ( addkp==NULL ) {
		    kp = chunkalloc(sizeof(KernPair));
		    kp->subtable = into;
		    kp->off = kern;
		    if ( is_l2r ) {
			kp->sc = g2->sc;
			kp->next = g1->sc->kerns;
			g1->sc->kerns = kp;
		    } else {
			kp->sc = g1->sc;
			kp->next = g2->sc->kerns;
			g2->sc->kerns = kp;
		    }
		} else
		    (*addkp)(data,g1->sc,g2->sc,kern);
	    }
	}
    }
    for ( i=0; i<cnt; ++i )
	AWGlyphFree( &glyphs[i] );
    free(glyphs);
#if !defined(_NO_PYTHON)
    FFPy_AWDataFree(&all);
#endif		/* PYTHON */
}

void AutoKern2NewClass(SplineFont *sf,int layer,char **leftnames, char **rightnames,
	int lcnt, int rcnt,
	void (*kcAddOffset)(void *data,int left_index, int right_index,int offset), void *data,
	int separation, int min_kern, int from_closest_approach, int only_closer,
	int chunk_height) {
    AW_Data all;
    AW_Glyph *glyphs;
    int i,cnt,k, kern;
    SplineChar ***left = GlyphClassesFromNames(sf,leftnames,lcnt);
    SplineChar ***right = GlyphClassesFromNames(sf,rightnames,rcnt);
    int **ileft = malloc(lcnt*sizeof(int*));
    int **iright = malloc(rcnt*sizeof(int*));
    SplineChar **class, *sc;

    if ( chunk_height <= 0 )
	chunk_height = (sf->ascent + sf->descent)/200;

    memset(&all,0,sizeof(all));
    all.layer = layer;
    all.sub_height = chunk_height;
    all.desired_separation = separation;
    all.sf = sf;
    all.denom = (sf->ascent + sf->descent)/DENOM_FACTOR_OF_EMSIZE;

    /* ticked means a left glyph, ticked2 a right (a glyph can be both) */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	sc->ticked = sc->ticked2 = false;
	sc->ttf_glyph = -1;
    }
    for ( i=0; i<lcnt; ++i )
	for ( class = left[i], k=0; (sc = class[k])!=NULL; ++k )
	    sc->ticked = true;
    for ( i=0; i<rcnt; ++i )
	for ( class = right[i], k=0; (sc = class[k])!=NULL; ++k )
	    sc->ticked2 = true;
    for ( cnt=i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL )
	if ( sc->ticked || sc->ticked2 )
	    ++cnt;

    glyphs = calloc(cnt+1,sizeof(AW_Glyph));
    for ( cnt=i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	if ( sc->ticked || sc->ticked2 ) {
	    SplineCharLayerFindBounds(sc,layer,&glyphs[cnt].bb);
	    if ( glyphs[cnt].bb.minx<-16000 || glyphs[cnt].bb.maxx>16000 ||
		    glyphs[cnt].bb.miny<-16000 || glyphs[cnt].bb.maxy>16000 ) {
		ff_post_notice(_("Glyph too big"),_("%s has a bounding box which is too big for this algorithm to work. Ignored."),sc->name);
		sc->ticked = sc->ticked2 = false;
	    } else {
		glyphs[cnt].sc = sc;
		sc->ttf_glyph = cnt;
		aw2_findedges(&glyphs[cnt++], &all);
	    }
	}
    }

    for ( i=0; i<lcnt; ++i ) {
	for ( class = left[i], k=0; (sc = class[k])!=NULL; ++k );
	ileft[i] = malloc((k+1)*sizeof(int));
	for ( class = left[i], k=0; (sc = class[k])!=NULL; ++k )
	    ileft[i][k] = sc->ttf_glyph;
	ileft[i][k] = -1;
    }
    for ( i=0; i<rcnt; ++i ) {
	for ( class = right[i], k=0; (sc = class[k])!=NULL; ++k );
	iright[i] = malloc((k+1)*sizeof(int));
	for ( class = right[i], k=0; (sc = class[k])!=NULL; ++k )
	    iright[i][k] = sc->ttf_glyph;
	iright[i][k] = -1;
    }

    all.glyphs = glyphs;
    all.gcnt = cnt;
    for ( i=0; i<lcnt; ++i ) {
	for ( k=0; k<rcnt; ++k ) {
	    if ( from_closest_approach )
		kern = rint( ak2_figure_touchclass(ileft[i],iright[k],&all));
	    else {
		kern = rint( ak2_figure_kernclass(ileft[i],iright[k],&all));
		if ( kern<min_kern && kern>-min_kern )
		    kern = 0;
	    }
	    if ( kern>0 && only_closer )
		kern = 0;
	    (*kcAddOffset)(data,i, k, kern);
	}
    }

    for ( i=0; i<lcnt; ++i ) {
	free(ileft[i]);
	free(left[i]);
    }
    free(ileft); free(left);
    for ( i=0; i<rcnt; ++i ) {
	free(iright[i]);
	free(right[i]);
    }
    free(iright); free(right);
    free(glyphs);
}

static void kc2AddOffset(void *data,int left_index, int right_index,int offset) {
    KernClass *kc = data;

    if ( kc->subtable->lookup->lookup_flags & pst_r2l ) {
	kc->offsets[right_index*kc->first_cnt+left_index] = offset;
    } else
	kc->offsets[left_index*kc->second_cnt+right_index] = offset;
}

void AutoKern2BuildClasses(SplineFont *sf,int layer,
	SplineChar **leftglyphs,SplineChar **rightglyphs,
	struct lookup_subtable *sub,
	int separation, int min_kern, int touching, int only_closer,
	int autokern,
	real good_enough) {
    AW_Data all;
    AW_Glyph *glyphs, *me, *other;
    int chunk_height;
    int i,j,k,cnt,lcnt,rcnt, lclasscnt,rclasscnt;
    int len;
    int *lused, *rused;
    char **lclassnames, **rclassnames, *str;
    int *visual_separation;
    KernClass *kc = sub->kc;
    SplineChar *sc;

    if ( kc==NULL )
return;
    // free(kc->firsts); free(kc->seconds); // I think that this forgets to free the contained strings.
    if (kc->firsts != NULL) {
      int tmppos;
      for (tmppos = 0; tmppos < kc->first_cnt; tmppos++)
        if (kc->firsts[tmppos] != NULL) { free(kc->firsts[tmppos]); kc->firsts[tmppos] = NULL; }
      free(kc->firsts); kc->firsts = NULL;
    }
    if (kc->seconds != NULL) {
      int tmppos;
      for (tmppos = 0; tmppos < kc->second_cnt; tmppos++)
        if (kc->seconds[tmppos] != NULL) { free(kc->seconds[tmppos]); kc->seconds[tmppos] = NULL; }
      free(kc->seconds); kc->seconds = NULL;
    }
    free(kc->offsets);
    free(kc->adjusts);

    // Group kerning.
    // Specifically, we drop the group kerning stuff if the classes are getting redefined.
    if (kc->firsts_names != NULL) {
      int tmppos;
      for (tmppos = 0; tmppos < kc->first_cnt; tmppos++)
        if (kc->firsts_names[tmppos] != NULL) { free(kc->firsts_names[tmppos]); kc->firsts_names[tmppos] = NULL; }
      free(kc->firsts_names); kc->firsts_names = NULL;
    }
    if (kc->seconds_names != NULL) {
      int tmppos;
      for (tmppos = 0; tmppos < kc->second_cnt; tmppos++)
        if (kc->seconds_names[tmppos] != NULL) { free(kc->seconds_names[tmppos]); kc->seconds_names[tmppos] = NULL; }
      free(kc->seconds_names); kc->seconds_names = NULL;
    }
    if (kc->firsts_flags != NULL) { free(kc->firsts_flags); kc->firsts_flags = NULL; }
    if (kc->seconds_flags != NULL) { free(kc->seconds_flags); kc->seconds_flags = NULL; }
    if (kc->offsets_flags != NULL) { free(kc->offsets_flags); kc->offsets_flags = NULL; }

    if ( good_enough==-1 )
	good_enough = (sf->ascent+sf->descent)/100.0;

    if ( separation==0 && !touching ) {
	/* Use default values. Generate them if they don't exist */
	if ( sub->separation==0 && !sub->kerning_by_touch ) {
	    sub->separation = sf->width_separation;
	    if ( sf->width_separation==0 )
		sub->separation = 15*(sf->ascent+sf->descent)/100;
	    separation = sub->separation;
	    autokern = true;
	} else {
	    separation = sub->separation;
	    touching = sub->kerning_by_touch;
	    min_kern = sub->minkern;
	    only_closer = sub->onlyCloser;
	    autokern = !sub->dontautokern;
	}
    }
    sub->separation = separation;
    sub->minkern = min_kern;
    sub->kerning_by_touch = touching;
    sub->onlyCloser = only_closer;
    sub->dontautokern = !autokern;
    chunk_height = (sf->ascent + sf->descent)/200;

    memset(&all,0,sizeof(all));
    all.layer = layer;
    all.sub_height = chunk_height;
    all.desired_separation = separation;
    all.sf = sf;
    all.denom = (sf->ascent + sf->descent)/DENOM_FACTOR_OF_EMSIZE;

    /* ticked means a left glyph, ticked2 a right (a glyph can be both) */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	sc->ticked = sc->ticked2 = false;
	sc->ttf_glyph = -1;
    }
    for ( lcnt=0; (sc=leftglyphs[lcnt])!=NULL; ++lcnt )
	sc->ticked = true;
    for ( rcnt=0; (sc=rightglyphs[rcnt])!=NULL; ++rcnt )
	sc->ticked2 = true;
    for ( cnt=i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL )
	if ( sc->ticked || sc->ticked2 )
	    ++cnt;

    glyphs = calloc(cnt+1,sizeof(AW_Glyph));
    for ( cnt=i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	if ( sc->ticked || sc->ticked2 ) {
	    SplineCharLayerFindBounds(sc,layer,&glyphs[cnt].bb);
	    if ( glyphs[cnt].bb.minx<-16000 || glyphs[cnt].bb.maxx>16000 ||
		    glyphs[cnt].bb.miny<-16000 || glyphs[cnt].bb.maxy>16000 ) {
		ff_post_notice(_("Glyph too big"),_("%s has a bounding box which is too big for this algorithm to work. Ignored."),sc->name);
		sc->ticked = sc->ticked2 = false;
	    } else {
		glyphs[cnt].sc = sc;
		sc->ttf_glyph = cnt;
		aw2_findedges(&glyphs[cnt++], &all);
	    }
	}
    }

    all.glyphs = glyphs;
    all.gcnt = cnt;
    visual_separation = malloc(lcnt*rcnt*sizeof(int));
    for ( i=0; i<lcnt; ++i ) {
	int *vpt = visual_separation + i*rcnt;
	SplineChar *lsc = leftglyphs[i];
	if ( lsc->ticked ) {
	    me = &all.glyphs[lsc->ttf_glyph];
	    for ( j=0; j<rcnt; ++j ) {
		SplineChar *rsc = rightglyphs[j];
		if ( rsc->ticked2 ) {
		    other = &all.glyphs[rsc->ttf_glyph];
		    vpt[j] = aw2_bbox_separation(me,other,&all);
		} else
		    vpt[j] = 0;
	    }
	} else {
	    for ( j=0; j<rcnt; ++j )
		vpt[j] = 0;
	}
    }
    for ( i=0; i<cnt; ++i )
	AWGlyphFree( &glyphs[i] );
#if !defined(_NO_PYTHON)
    FFPy_AWDataFree(&all);
    free(glyphs); glyphs = all.glyphs = NULL;
#endif		/* PYTHON */
    glyphs = all.glyphs = NULL;

    good_enough *= good_enough;

    lclassnames = malloc((lcnt+2) * sizeof(char *));
    lclasscnt = 1;
    lclassnames[0] = NULL;
    lused = calloc(lcnt,sizeof(int));
    for ( i=0; i<lcnt; ++i ) if ( leftglyphs[i]->ticked && !lused[i]) {
	lused[i] = lclasscnt;
	len = strlen(leftglyphs[i]->name)+1;
	for ( j=i+1; j<lcnt; ++j ) if ( leftglyphs[j]->ticked && !lused[j] ) {
	    long totdiff = 0, diff;
	    int *vpt1 = visual_separation + i*rcnt;
	    int *vpt2 = visual_separation + j*rcnt;
	    for ( k=0; k<rcnt; ++k ) {
		diff = vpt1[k] - vpt2[k];
		totdiff += diff*diff;
	    }
	    if ( totdiff/((double) rcnt) < good_enough ) {
		lused[j] = lclasscnt;
		len += strlen(leftglyphs[j]->name)+1;
	    }
	}
	lclassnames[lclasscnt] = str = malloc(len+1);
	for ( j=i; j<lcnt; ++j ) if ( lused[j]==lclasscnt ) {
	    strcpy(str,leftglyphs[j]->name);
	    str += strlen(str);
	    *str++ = ' ';
	}
	str[-1] = '\0';
	++lclasscnt;
    }
    lclassnames[lclasscnt] = NULL;
    kc = sub->kc;
    kc->firsts = realloc(lclassnames,(lclasscnt+1)*sizeof(char *));
    kc->first_cnt = lclasscnt;
    free(lused); lused = NULL;

    rclassnames = malloc((rcnt+2) * sizeof(char *));
    rclasscnt = 1;
    rclassnames[0] = NULL;
    rused = calloc(rcnt,sizeof(int));
    for ( i=0; i<rcnt; ++i ) if ( rightglyphs[i]->ticked2 && !rused[i]) {
	rused[i] = rclasscnt;
	len = strlen(rightglyphs[i]->name)+1;
	for ( j=i+1; j<rcnt; ++j ) if ( rightglyphs[j]->ticked2 && !rused[j] ) {
	    long totdiff = 0, diff;
	    int *vpt1 = visual_separation + i;
	    int *vpt2 = visual_separation + j;
	    for ( k=0; k<lcnt; ++k ) {
		diff = vpt1[k*rcnt] - vpt2[k*rcnt];
		totdiff += diff*diff;
	    }
	    if ( totdiff/((double) lcnt) < good_enough ) {
		rused[j] = rclasscnt;
		len += strlen(rightglyphs[j]->name)+1;
	    }
	}
	rclassnames[rclasscnt] = str = malloc(len+1);
	for ( j=i; j<rcnt; ++j ) if ( rused[j]==rclasscnt ) {
	    strcpy(str,rightglyphs[j]->name);
	    str += strlen(str);
	    *str++ = ' ';
	}
	str[-1] = '\0';
	++rclasscnt;
    }
    rclassnames[rclasscnt] = NULL;
    kc->seconds = realloc(rclassnames,(rclasscnt+1)*sizeof(char *));
    kc->second_cnt = rclasscnt;
    free(rused);
    free(visual_separation);
	
    kc->offsets = calloc(lclasscnt*rclasscnt,sizeof(int16));;
    kc->adjusts = calloc(lclasscnt*rclasscnt,sizeof(DeviceTable));

    if ( autokern )
	AutoKern2NewClass(sf,layer,kc->firsts, kc->seconds,
		kc->first_cnt, kc->second_cnt, 
		kc2AddOffset, kc,
		separation,min_kern,touching,only_closer,chunk_height);

    if ( sub->lookup->lookup_flags & pst_r2l ) {
	char **temp = kc->seconds;
	int tempsize = kc->second_cnt;
	kc->seconds = kc->firsts;
	kc->second_cnt = kc->first_cnt;
	kc->firsts = temp;
	kc->first_cnt = tempsize;
    }
}
