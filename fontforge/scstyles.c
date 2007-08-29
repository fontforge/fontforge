/* Copyright (C) 2007 by George Williams */
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <ustring.h>
#include <utype.h>
#include <gkeysym.h>
#include <math.h>

/* Code for various stylistic changes embolden/thin, condense/extend, oblique */

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

    ptmoves[cnt].newpos = ptmoves[0].newpos;
    for ( i=0; i<cnt; ++i ) {
	sp = ptmoves[i].sp;
	nsp = ptmoves[i+1].sp;
	if ( sp->nonextcp )
	    sp->nextcp = ptmoves[i].newpos;
	if ( nsp->noprevcp )
	    nsp->prevcp = ptmoves[i+1].newpos;
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
    }
    for ( i=0; i<cnt; ++i )
	ptmoves[i].sp->me = ptmoves[i].newpos;
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
	SplinePointListTransform(ss_expanded,transform,true);
	if ( layer==ly_fore )
	    SCSynchronizeLBearing(sc,transform[4]);
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

static int InHintConflictsZone(StemInfo *stems,StemInfo *me,double contains_y,
	double off) {
    StemInfo *h;

    for ( h=stems; h!=NULL; h=h->next ) if ( h!=me && h->hasconflicts ) {
	 if ( ((h->start <= contains_y && h->start+h->width>=contains_y) ||
		  (h->start <= contains_y-off && h->start+h->width>=contains_y-off )) &&
		 h->start<=me->start+me->width && h->start+h->width>=me->start )
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
		found = true;
	break;
	    }
	    sp = sp->prev->from;
	    if ( sp==ss->first )
	break;
	}
	if ( !found )
	    ss->first = ss->last = sp->prev->from;		/* Often true */
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
    ptmoves = galloc((ccnt+1)*sizeof(struct ptmoves));
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
	    if ( sign ) {
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
    ptmoves = galloc((ccnt+1)*sizeof(struct ptmoves));
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
	    if ( sp->me.y>=zones->top_bound || (h!=NULL && othery+zones->stroke_width/2>=zones->top_bound))
		sign = -1;
	    else if ( sp->me.y<=zones->bottom_bound || (h!=NULL && othery-zones->stroke_width/2<=zones->bottom_bound))
		sign = 1;
	    else if ( (origsp!=NULL &&
			    (InHintAroundZone(sc->hstem,origsp->me.y,zones->top_bound) ||
			     InHintAroundZone(sc->hstem,origsp->me.y,zones->bottom_bound)) ) ||
		( h!=NULL && h->hasconflicts &&
			(InHintConflictsZone(sc->hstem,h,zones->top_bound,zones->stroke_width/2) ||
			 InHintConflictsZone(sc->hstem,h,zones->bottom_bound,-zones->stroke_width/2))) )
		/* It's not on the hint, so we want to interpolate it even if */
		/*  it's in an area we'd normally move (tahoma "s" has two */
		/*  points which fall on the baseline but aren't on the bottom */
		/*  hint */
		/* Do Nothing */;
	    else if ( h!=NULL ) {
		/* Point on a hint. This one not in the zones, so it is fixed */
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
		    /* If it isn't between them, then average the movements */
		    /* this case should not happen */
		    ptmoves[j].newpos.y += (ptmoves[n].newpos.y-nsp->me.y +
					    ptmoves[p].newpos.y-psp->me.y)/2;
		    ptmoves[j].newpos.x += (ptmoves[n].newpos.x-nsp->me.x +
					    ptmoves[p].newpos.x-psp->me.x)/2;
		    /* LogError("Hmm. This shouldn't happen, but probably doesn't matter.\n" ); */
		} else {
		    double ydiff;
		    ydiff = nsp->me.y - psp->me.y;		/* Not zero, by above test */
		    ptmoves[j].newpos.y += (ptmoves[p].newpos.y-psp->me.y)
					    + (sp->me.y-psp->me.y)*(ptmoves[n].newpos.y-nsp->me.y-(ptmoves[p].newpos.y-psp->me.y))/ydiff;
		    /* Note we even interpolate the x direction depending on */
		    /*  y position */
		    ptmoves[j].newpos.x += (ptmoves[p].newpos.x-psp->me.x)
					    + (sp->me.y-psp->me.y)*(ptmoves[n].newpos.x-nsp->me.x-(ptmoves[p].newpos.x-psp->me.x))/ydiff;
		}
		ptmoves[j].touched = true;
	    }
	}
	InterpolateControlPointsAndSet(ptmoves,cnt);
    }
    free(ptmoves);

    CorrectLeftSideBearing(ss_expanded,sc,layer);
return( ss_expanded );
}

static void LCG_EmboldenWidthHook(SplineChar *sc, struct lcg_zones *zones,
	DBounds *old, DBounds *new) {
    SCSynchronizeWidth(sc,sc->width+rint(zones->stroke_width),sc->width,NULL);
}

static void NumberLayerPoints(SplineSet *ss) {
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
}
	    
static void SCEmbolden(SplineChar *sc, struct lcg_zones *zones, int layer) {
    StrokeInfo si;
    SplineSet *temp;
    DBounds old, new;

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
    si.removeoverlapifneeded = zones->removeoverlap;
    si.toobigwarn = true;

    if ( (layer==-2 || layer==ly_fore) && zones->wants_hints &&
	    sc->hstem == NULL && sc->vstem==NULL && sc->dstem==NULL ) {
	_SplineCharAutoHint(sc,zones->bd,NULL,false);
    }

    if ( layer==-2 ) {
	SCPreserveState(sc,false);
	SplineCharFindBounds(sc,&old);
	for ( layer = ly_fore; layer<sc->layer_cnt; ++layer ) {
	    NumberLayerPoints(sc->layers[layer].splines);
	    temp = SSStroke(sc->layers[layer].splines,&si,sc);
	    if ( zones->embolden_hook!=NULL )
		temp = (zones->embolden_hook)(temp,zones,sc,layer);
	    SplinePointListsFree( sc->layers[layer].splines );
	    sc->layers[layer].splines = temp;
	}
	SplineCharFindBounds(sc,&new);
	if ( zones->embolden_width!=NULL )
	    (zones->embolden_width)(sc,zones,&old,&new);
    } else if ( layer>=0 ) {
	SCPreserveLayer(sc,layer,false);
	NumberLayerPoints(sc->layers[layer].splines);
	SplineSetFindBounds(sc->layers[layer].splines,&old);
	temp = SSStroke(sc->layers[layer].splines,&si,sc);
	if ( zones->embolden_hook!=NULL )
	    temp = (zones->embolden_hook)(temp,zones,sc,layer);
	SplineSetFindBounds(temp,&new);
	if ( zones->embolden_width!=NULL && layer==ly_fore )
	    (zones->embolden_width)(sc,zones,&old,&new);
	SplinePointListsFree( sc->layers[layer].splines );
	sc->layers[layer].splines = temp;
    }

    if ( layer==-2 || layer==ly_fore ) {
	/* Hints will be inccorrect (misleading) after these transformations */
	StemInfosFree(sc->vstem); sc->vstem=NULL;
	StemInfosFree(sc->hstem); sc->hstem=NULL;
	DStemInfosFree(sc->dstem); sc->dstem=NULL;
	SCOutOfDateBackground(sc);
    }
    SCCharChangedUpdate(sc);
}

static struct {
    uint32 script;
    SplineSet *(*embolden_hook)(SplineSet *,struct lcg_zones *,SplineChar *, int layer);
    void (*embolden_width)(SplineChar *sc, struct lcg_zones *,DBounds *old, DBounds *new);
} script_hooks[] = {
    { CHR('l','a','t','n'), LCG_HintedEmboldenHook, LCG_EmboldenWidthHook },
    { CHR('c','y','r','l'), LCG_HintedEmboldenHook, LCG_EmboldenWidthHook },
    { CHR('g','r','e','k'), LCG_HintedEmboldenHook, LCG_EmboldenWidthHook },
	/* Hebrew probably works too */
    { CHR('h','e','b','r'), LCG_HintedEmboldenHook, LCG_EmboldenWidthHook },
    0
};

static struct {
    unichar_t from, to;
    SplineSet *(*embolden_hook)(SplineSet *,struct lcg_zones *,SplineChar *, int layer);
    void (*embolden_width)(SplineChar *sc, struct lcg_zones *,DBounds *old, DBounds *new);
} char_hooks[] = {
    { '0','9', LCG_HintedEmboldenHook, LCG_EmboldenWidthHook },
    { '$','%', LCG_HintedEmboldenHook, LCG_EmboldenWidthHook },
    { '$','%', LCG_HintedEmboldenHook, LCG_EmboldenWidthHook },
    0
};

static void ZoneInit(SplineFont *sf, struct lcg_zones *zones,enum embolden_type type) {

    if ( type == embolden_lcg ) {
	zones->embolden_hook = LCG_HintedEmboldenHook;
	zones->embolden_width = LCG_EmboldenWidthHook;
    } else if ( type == embolden_custom ) {
	zones->embolden_hook = LCG_EmboldenHook;
	zones->embolden_width = LCG_EmboldenWidthHook;
    } else {
	zones->embolden_hook = NULL;
	zones->embolden_width = NULL;
    }
}

static void PerGlyphInit(SplineChar *sc, struct lcg_zones *zones,
	enum embolden_type type, BlueData *bd) {
    int j;
    SplineChar *hebrew;

    if ( type == embolden_auto ) {
	zones->embolden_hook = NULL;
	zones->embolden_width = NULL;
	for ( j=0; char_hooks[j].from!=0; ++j ) {
	    if ( sc->unicodeenc>=char_hooks[j].from && sc->unicodeenc<=char_hooks[j].to ) {
		zones->embolden_hook = char_hooks[j].embolden_hook;
		zones->embolden_width = char_hooks[j].embolden_width;
	break;
	    }
	}
	if ( zones->embolden_hook == NULL ) {
	    uint32 script = SCScriptFromUnicode(sc);
	    for ( j=0; script_hooks[j].script!=0; ++j ) {
		if ( script==script_hooks[j].script ) {
		    zones->embolden_hook = script_hooks[j].embolden_hook;
		    zones->embolden_width = script_hooks[j].embolden_width;
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
	    zones->bottom_zone = bd->xheight>0 ? bd->xheight/3 :
			    bd->caph>0 ? bd->caph/3 :
			    (sc->parent->ascent/4);
	    zones->top_zone = bd->xheight>0 ? 2*bd->xheight/3 :
			    bd->caph>0 ? 2*bd->caph/3 :
			    (sc->parent->ascent/2);
	    zones->top_bound = bd->xheight>0 ? bd->xheight :
			    bd->caph>0 ? bd->caph :
			    (sc->parent->ascent/2);
	} else {
	    /* The default behavior is to treat everything unknown like a */
	    /*  capital latin letter. */
	    zones->bottom_zone = bd->caph>0 ? bd->caph/3 :
			    (sc->parent->ascent/4);
	    zones->top_zone = bd->caph>0 ? 2*bd->caph/3 :
			    (sc->parent->ascent/2);
	    zones->top_bound = bd->caph>0?bd->caph:4*sc->parent->ascent/5;
	}
    }
    zones->wants_hints = zones->embolden_hook == LCG_HintedEmboldenHook;
    zones->bd = bd;
}

void FVEmbolden(FontView *fv,enum embolden_type type,struct lcg_zones *zones) {
    int i, gid;
    SplineChar *sc;
    BlueData bd;

    ZoneInit(fv->sf,zones,type);
    QuickBlues(fv->sf, &bd);

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL ) {
	PerGlyphInit(sc,zones,type, &bd);
	SCEmbolden(sc, zones, -2);		/* -2 => all foreground layers */
    }
}

static void CVEmbolden(CharView *cv,enum embolden_type type,struct lcg_zones *zones) {
    SplineChar *sc = cv->sc;
    BlueData bd;

    if ( cv->drawmode == dm_grid )
return;

    ZoneInit(sc->parent,zones,type);
    QuickBlues(sc->parent, &bd);

    PerGlyphInit(sc,zones,type, &bd);
    SCEmbolden(sc, zones, CVLayer(cv));
}

typedef struct styledlg {
    int done;
    GWindow gw;
    CharView *cv;
    FontView *fv;
    SplineFont *sf;
} StyleDlg;

#define CID_EmBdWidth	1001
#define CID_LCG		1002
#define CID_CJK		1003
#define CID_Auto	1004
#define CID_Custom	1005
#define CID_TopZone	1006
#define CID_BottomZone	1007
#define CID_CleanupSelfIntersect	1008

static SplineFont *lastsf = NULL;
static enum embolden_type last_type = embolden_auto;
static struct lcg_zones last_zones;
static int last_width;
static int last_overlap = false;

static int Embolden_OK(GGadget *g, GEvent *e) {
    enum embolden_type type;
    struct lcg_zones zones;
    int err = false;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow ew = GGadgetGetWindow(g);
	StyleDlg *ed = GDrawGetUserData(ew);
	memset(&zones,0,sizeof(zones));
	err = false;
	zones.stroke_width = GetReal8(ew,CID_EmBdWidth,_("Embolden by"),&err);
	type = GGadgetIsChecked( GWidgetGetControl(ew,CID_LCG)) ? embolden_lcg :
		GGadgetIsChecked( GWidgetGetControl(ew,CID_CJK)) ? embolden_cjk :
		GGadgetIsChecked( GWidgetGetControl(ew,CID_Auto)) ? embolden_auto :
			embolden_custom;
	if ( type == embolden_custom ) {
	    zones.top_zone = GetReal8(ew,CID_TopZone,_("Top Zone"),&err);
	    zones.bottom_zone = GetReal8(ew,CID_BottomZone,_("Bottom Zone"),&err);
	}
	if ( err )
return( true );

	lastsf = ed->sf;
	last_type = type;
	last_width = zones.stroke_width;
	last_overlap = zones.removeoverlap = GGadgetIsChecked( GWidgetGetControl(ew,CID_CleanupSelfIntersect));
	if ( type == embolden_custom )
	    last_zones = zones;

	if ( ed->fv!=NULL )
	    FVEmbolden(ed->fv, type, &zones);
	else
	    CVEmbolden(ed->cv, type, &zones);
	ed->done = true;
    }
return( true );
}

static int Embolden_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	StyleDlg *ed = GDrawGetUserData(GGadgetGetWindow(g));
	ed->done = true;
    }
return( true );
}

static int Embolden_Radio(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StyleDlg *ed = GDrawGetUserData(GGadgetGetWindow(g));
	if (GGadgetIsChecked( GWidgetGetControl(ed->gw,CID_Custom)) ) {
	    GGadgetSetEnabled(GWidgetGetControl(ed->gw,CID_TopZone),true);
	    GGadgetSetEnabled(GWidgetGetControl(ed->gw,CID_BottomZone),true);
	} else {
	    GGadgetSetEnabled(GWidgetGetControl(ed->gw,CID_TopZone),false);
	    GGadgetSetEnabled(GWidgetGetControl(ed->gw,CID_BottomZone),false);
	}
    }
return( true );
}

static int embolden_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	StyleDlg *ed = GDrawGetUserData(gw);
	ed->done = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("elementmenu.html#Style");
return( true );
	}
return( false );
    }
return( true );
}

void EmboldenDlg(FontView *fv, CharView *cv) {
    StyleDlg ed;
    SplineFont *sf = fv!=NULL ? fv->sf : cv->sc->parent;
    BlueData bd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[15], boxes[6], *barray[8], *rarray[6], *hvarray[24];
    GTextInfo label[15];
    int k;
    char topzone[40], botzone[40], emb_width[40];

    QuickBlues(sf, &bd);

    memset(&ed,0,sizeof(ed));
    ed.fv = fv;
    ed.cv = cv;
    ed.sf = sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Embolden/Thin...");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    ed.gw = gw = GDrawCreateTopWindow(NULL,&pos,embolden_e_h,&ed,&wattrs);


    k=0;

    memset(gcd,0,sizeof(gcd));
    memset(boxes,0,sizeof(boxes));
    memset(label,0,sizeof(label));
    label[k].text = (unichar_t *) _("Embolden by:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[0] = &gcd[k-1];

    sprintf( emb_width, "%d", sf==lastsf ? last_width : (sf->ascent+sf->descent)/20 );
    label[k].text = (unichar_t *) emb_width;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_EmBdWidth;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[1] = &gcd[k-1]; hvarray[2] = NULL;

    label[k].text = (unichar_t *) _("_LCG");
    gcd[k].gd.popup_msg = (unichar_t *) _("Embolden as appropriate for Latin, Cyrillic and Greek scripts");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_LCG;
    gcd[k].gd.handle_controlevent = Embolden_Radio;
    gcd[k++].creator = GRadioCreate;
    rarray[0] = &gcd[k-1];

    label[k].text = (unichar_t *) _("_CJK");
    gcd[k].gd.popup_msg = (unichar_t *) _("Embolden as appropriate for Chinese, Japanese, Korean scripts");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_CJK;
    gcd[k].gd.handle_controlevent = Embolden_Radio;
    gcd[k++].creator = GRadioCreate;
    rarray[1] = &gcd[k-1];

    label[k].text = (unichar_t *) _("_Auto");
    gcd[k].gd.popup_msg = (unichar_t *) _("Choose the appropriate method depending on the glyph's script");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_Auto;
    gcd[k].gd.handle_controlevent = Embolden_Radio;
    gcd[k++].creator = GRadioCreate;
    rarray[2] = &gcd[k-1];

    label[k].text = (unichar_t *) _("C_ustom");
    gcd[k].gd.popup_msg = (unichar_t *) _("User controls the emboldening with the next two fields");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.cid = CID_Custom;
    gcd[k].gd.handle_controlevent = Embolden_Radio;
    gcd[k++].creator = GRadioCreate;
    rarray[3] = &gcd[k-1]; rarray[4] = GCD_Glue; rarray[5] = NULL;

    if ( lastsf==sf )
	gcd[k-4 + embolden_auto].gd.flags |= gg_cb_on;
    else
	gcd[k-4 + last_type].gd.flags |= gg_cb_on;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = rarray;
    boxes[3].creator = GHBoxCreate;
    hvarray[3] = &boxes[3]; hvarray[4] = GCD_ColSpan; hvarray[5] = NULL;

    label[k].text = (unichar_t *) _("_Top zone:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[6] = &gcd[k-1];

    sprintf( topzone, "%d", lastsf==sf && last_type==embolden_custom ? last_zones.top_zone :
			(int) rint(bd.xheight>0 ? 2*bd.xheight/3 :
			bd.caph>0 ? 2*bd.caph/3 :
			(sf->ascent/2)) );
    label[k].text = (unichar_t *) topzone;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_TopZone;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[7] = &gcd[k-1]; hvarray[8] = NULL;

    label[k].text = (unichar_t *) _("_Bottom zone:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[9] = &gcd[k-1];

    sprintf( botzone, "%d", lastsf==sf && last_type==embolden_custom ? last_zones.bottom_zone :
			(int) rint(bd.xheight>0 ? bd.xheight/3 :
			bd.caph>0 ? bd.caph/3 :
			(sf->ascent/4)) );
    label[k].text = (unichar_t *) botzone;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_BottomZone;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[10] = &gcd[k-1]; hvarray[11] = NULL;

    label[k].text = (unichar_t *) _("Cleanup Self Intersect");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup | (last_overlap?gg_cb_on:0);
    gcd[k].gd.cid = CID_CleanupSelfIntersect;
    gcd[k].gd.popup_msg = (unichar_t *) _("When FontForge detects that an expanded stroke will self-intersect,\nthen setting this option will cause it to try to make things nice\nby removing the intersections");
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[12] = &gcd[k-1]; hvarray[13] = GCD_ColSpan; hvarray[14] = NULL;

    hvarray[15] = GCD_Glue; hvarray[16] = GCD_Glue; hvarray[17] = NULL;

    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Embolden_OK;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[k-1]; barray[2] = GCD_Glue;

    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Embolden_Cancel;
    gcd[k].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[k]; barray[5] = GCD_Glue;
    barray[6] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = barray;
    boxes[4].creator = GHBoxCreate;
    hvarray[18] = &boxes[4]; hvarray[19] = GCD_ColSpan; hvarray[20] = NULL;
    hvarray[21] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(gw,true);

    while ( !ed.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

/* ************************************************************************** */
/* ***************************** Condense/Extend **************************** */
/* ************************************************************************** */

/* We need to look at the counters. There are two types of counters: */
/*  the left and right side bearings */
/*  internal counters                */
/* If the glyph is nicely hinted with vertical stems then all we need to */
/*  do is look at the hints. Complications "B" which has slightly different */
/*  counters top and bottom. */
/* I'm going to assume that LCG glyphs have at most two counter zones, one */
/*  near the bottom (baseline), one near the top */
/* However many glyphs have diagonal stems: V, A, W, M, K, X, Y */
/*  Many of these have two zones (like "B" above) W, M, K, Y (maybe X) */
/* Find the places where these guys hit the baseline/cap-height (x-height) */
/*  and define these as potential counter boundries. Ignore places where   */
/*  glyphs hit with curves (like O, Q, p).
/* Remember to merge a hint with a top/bottom hit (harder with serifs) */

/* We may still not have a useful counter: 7 3 2 C E T */
/*  Use the left and right sides of the bounding box (do I need to know */
/*  StemSnapV? -- standard stem size -- yes, don't want to expand the stem) */
/* Don't try to make I 1 l i grow, only one stem even if the serifs make the */
/*  bounding box bigger */

/* If the font is italic, then skew it by the italic angle in hopes of getting*/
/*  some real vertical stems, rehint, condense/extend & unskew */

struct counterinfo {
    double c_factor, c_add;		/* For counters */
    double sb_factor, sb_add;		/* For side bearings */
    uint8 correct_italic;

    BlueData bd;
    double stdvw;

    SplineChar *sc;
    int layer;
    DBounds bb;				/* Value before change */
    double top_y, bottom_y, boundry;
    int has_two_zones;
#define TOP_Z	0
#define BOT_Z	1
    int cnts[2];
    int maxes[2];
    struct ci_zones {
	double start, width;
	double moveto, newwidth;	/* Only change width for diagonal stems*/
    } *zones[2];
};

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
	ci->zones[z] = grealloc(ci->zones[z],(ci->maxes[z]+=10)*sizeof(struct ci_zones));
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
		    y += (8-i)*diff;
		    i = 8;
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
	    if ( SpOnEdge(sp,ci->bottom_y,1,ci,BOT_Z))
		/* All Done */;
	    else if ( SpOnEdge(sp,ci->top_y,-1,ci,TOP_Z))
		/* All Done */;
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

static void SCCondenseExtend(struct counterinfo *ci,SplineChar *sc, int layer) {
    SplineSet *ss;
    SplinePoint *sp;
    Spline *s, *first;
    DBounds b;
    int width;
    double offset;
    real transform[6];

    SCPreserveLayer(sc,layer,false);

    if ( ci->correct_italic && sc->parent->italicangle!=0 ) {
	memset(transform,0,sizeof(transform));
	transform[0] = transform[3] = 1;
	transform[2] = -tan( sc->parent->italicangle * 3.1415926535897932/180.0 );
	SplinePointListTransform(sc->layers[layer].splines,transform,true);
	StemInfosFree(sc->vstem); sc->vstem=NULL;
    }
    if ( sc->vstem==NULL )
	_SplineCharAutoHint(sc,&ci->bd,NULL,false);

    PerGlyphFindCounters(ci,sc, layer);

    for ( ss = sc->layers[layer].splines; ss!=NULL; ss = ss->next ) {
	for ( sp=ss->first; ; ) {
	    BPAdjustCE(&sp->nextcp,ci);
	    BPAdjustCE(&sp->prevcp,ci);
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
	SplinePointListTransform(sc->layers[layer].splines,transform,true);
    if ( layer==ly_fore ) {
	width = b.maxx + (sc->width-ci->bb.maxx)*ci->sb_factor/100. + ci->sb_add;
	SCSynchronizeWidth(sc,width,sc->width,NULL);
	offset = (b.maxx-b.minx)/2 - (ci->bb.maxx-ci->bb.minx)/2;
	/* We haven't really changed the left side bearing by offset, but */
	/*  this is the amount (about) by which we need to adjust accents */
	SCSynchronizeLBearing(sc,offset);
    }

    if ( ci->correct_italic && sc->parent->italicangle!=0 ) {
	/* If we unskewed it, we want to skew it now */
	memset(transform,0,sizeof(transform));
	transform[0] = transform[3] = 1;
	transform[2] = tan( sc->parent->italicangle * 3.1415926535897932/180.0 );
	SplinePointListTransform(sc->layers[layer].splines,transform,true);
    }
    
    if ( layer==ly_fore ) {
	/* Hints will be inccorrect (misleading) after these transformations */
	StemInfosFree(sc->vstem); sc->vstem=NULL;
	StemInfosFree(sc->hstem); sc->hstem=NULL;
	DStemInfosFree(sc->dstem); sc->dstem=NULL;
	SCOutOfDateBackground(sc);
    }
    SCCharChangedUpdate(sc);
}

static void FVCondenseExtend(FontView *fv,struct counterinfo *ci) {
    int i, gid;
    SplineChar *sc;

    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] &&
	    (gid = fv->map->map[i])!=-1 && (sc=fv->sf->glyphs[gid])!=NULL ) {
	SCCondenseExtend(ci,sc,ly_fore);
    }

    free( ci->zones[0]);
    free( ci->zones[1]);
}

static void CVCondenseExtend(CharView *cv,struct counterinfo *ci) {
    SplineChar *sc = cv->sc;

    if ( cv->drawmode == dm_grid )
return;

    SCCondenseExtend(ci, sc, CVLayer(cv));

    free( ci->zones[0]);
    free( ci->zones[1]);
}

static void CI_Init(struct counterinfo *ci,SplineFont *sf) {
    char *ret;

    QuickBlues(sf, &ci->bd);

    if ( sf->private!=NULL && (ret=PSDictHasEntry(sf->private,"StdVW"))!=NULL )
	ci->stdvw = strtod(ret,NULL);

    if ( ci->stdvw<=0 )
	ci->stdvw = (sf->ascent+sf->descent)/12.5;
}

#define CID_C_Factor	1001
#define CID_C_Add	1002
#define CID_SB_Factor	1003
#define CID_SB_Add	1004
#define CID_CorrectItalic	1005

static struct counterinfo last_ci = { 90, 0, 90, 0, true };

static int CondenseExtend_OK(GGadget *g, GEvent *e) {
    struct counterinfo ci;
    int err = false;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow ew = GGadgetGetWindow(g);
	StyleDlg *ed = GDrawGetUserData(ew);
	memset(&ci,0,sizeof(ci));
	err = false;
	ci.c_factor  = GetReal8(ew,CID_C_Factor,_("Counter Expansion Factor"),&err);
	ci.c_add     = GetReal8(ew,CID_C_Add,_("Counter Addition"),&err);
	ci.sb_factor = GetReal8(ew,CID_SB_Factor,_("Side Bearing Expansion Factor"),&err);
	ci.sb_add    = GetReal8(ew,CID_SB_Add,_("Side Bearing Addition"),&err);
	ci.correct_italic= GGadgetIsChecked( GWidgetGetControl(ew,CID_CorrectItalic));
	if ( err )
return( true );

	last_ci = ci;

	CI_Init(&ci,ed->sf);
	if ( ed->fv!=NULL )
	    FVCondenseExtend(ed->fv, &ci);
	else
	    CVCondenseExtend(ed->cv, &ci);
	ed->done = true;
    }
return( true );
}

static int CondenseExtend_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	StyleDlg *ed = GDrawGetUserData(GGadgetGetWindow(g));
	ed->done = true;
    }
return( true );
}

void CondenseExtendDlg(FontView *fv, CharView *cv) {
    StyleDlg ed;
    SplineFont *sf = fv!=NULL ? fv->sf : cv->sc->parent;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[15], boxes[6], *barray[8], *hvarray[31];
    GTextInfo label[15];
    int k;
    char c_factor[40], c_add[40], sb_factor[40], sb_add[40];

    memset(&ed,0,sizeof(ed));
    ed.fv = fv;
    ed.cv = cv;
    ed.sf = sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Condense/Extend...");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = 100;
    pos.height = 100;
    ed.gw = gw = GDrawCreateTopWindow(NULL,&pos,embolden_e_h,&ed,&wattrs);


    k=0;

    memset(gcd,0,sizeof(gcd));
    memset(boxes,0,sizeof(boxes));
    memset(label,0,sizeof(label));

    hvarray[0] = GCD_Glue;

    label[k].text = (unichar_t *) _("Scale By");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[1] = &gcd[k-1];
    hvarray[2] = GCD_Glue;

    label[k].text = (unichar_t *) _("Add");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[3] = &gcd[k-1];
    hvarray[4] = NULL;

    label[k].text = (unichar_t *) _("Counters:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[5] = &gcd[k-1];

    sprintf( c_factor, "%g", last_ci.c_factor );
    label[k].text = (unichar_t *) c_factor;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_C_Factor;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[6] = &gcd[k-1];

    label[k].text = (unichar_t *) "% +";
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[7] = &gcd[k-1];

    sprintf( c_add, "%g", last_ci.c_add );
    label[k].text = (unichar_t *) c_add;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_C_Add;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[8] = &gcd[k-1];
    hvarray[9] = NULL;

    label[k].text = (unichar_t *) _("Side Bearings:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[10] = &gcd[k-1];

    sprintf( sb_factor, "%g", last_ci.sb_factor );
    label[k].text = (unichar_t *) sb_factor;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_SB_Factor;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[11] = &gcd[k-1];

    label[k].text = (unichar_t *) "% +";
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+31;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvarray[12] = &gcd[k-1];

    sprintf( sb_add, "%g", last_ci.sb_add );
    label[k].text = (unichar_t *) sb_add;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-3;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k].gd.cid = CID_SB_Add;
    gcd[k++].creator = GNumericFieldCreate;
    hvarray[13] = &gcd[k-1];
    hvarray[14] = NULL;

    label[k].text = (unichar_t *) _("Correct for Italic Angle");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup | (last_ci.correct_italic?gg_cb_on:0);
    gcd[k].gd.cid = CID_CorrectItalic;
    gcd[k].gd.popup_msg = (unichar_t *) _("When FontForge detects that an expanded stroke will self-intersect,\nthen setting this option will cause it to try to make things nice\nby removing the intersections");
    gcd[k++].creator = GCheckBoxCreate;
    hvarray[15] = &gcd[k-1];
    hvarray[16] = hvarray[17] = hvarray[18] = GCD_ColSpan; hvarray[19] = NULL;
    if ( sf->italicangle==0 )
	gcd[k-1].gd.flags = ~gg_enabled;

    hvarray[20] = hvarray[21] = hvarray[22] = hvarray[23] = GCD_ColSpan; hvarray[24] = NULL;

    gcd[k].gd.pos.x = 30-3; gcd[k].gd.pos.y = 5;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = CondenseExtend_OK;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[k-1]; barray[2] = GCD_Glue;

    gcd[k].gd.pos.x = -30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = CondenseExtend_Cancel;
    gcd[k].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[k]; barray[5] = GCD_Glue;
    barray[6] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = barray;
    boxes[3].creator = GHBoxCreate;
    hvarray[25] = &boxes[3];
    hvarray[26] = hvarray[27] = hvarray[28] = GCD_ColSpan; hvarray[29] = NULL;
    hvarray[30] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    GDrawSetVisible(gw,true);

    while ( !ed.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
