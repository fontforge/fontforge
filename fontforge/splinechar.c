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

#include <fontforge-config.h>

#include "cvundoes.h"
#include "dumppfa.h"
#include "ffglib_compat.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "lookups.h"
#include "mem.h"
#include "parsettf.h"
#include "spiro.h"
#include "splineorder2.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "tottf.h"
#include "ttf.h"
#include "ustring.h"
#include "utanvec.h"
#include "utype.h"

#include <locale.h>
#include <math.h>

#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif

int adjustwidth = true;
int adjustlbearing = true;
int allow_utf8_glyphnames = false;
int clear_tt_instructions_when_needed = true;

void SCClearRounds(SplineChar *sc,int layer) {
    SplineSet *ss;
    SplinePoint *sp;

    for ( ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    sp->roundx = sp->roundy = false;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
}

void MDReplace(MinimumDistance *md,SplineSet *old,SplineSet *rpl) {
    /* Replace all the old points with the ones in rpl in the minimum distance hints */
    SplinePoint *osp, *rsp;
    MinimumDistance *test;

    if ( md==NULL )
return;

    while ( old!=NULL && rpl!=NULL ) {
	osp = old->first; rsp = rpl->first;
	while ( 1 ) {
	    for ( test=md; test!=NULL ; test=test->next ) {
		if ( test->sp1==osp )
		    test->sp1 = rsp;
		if ( test->sp2==osp )
		    test->sp2 = rsp;
	    }
	    if ( osp->next==NULL || rsp->next==NULL )
	break;
	    osp = osp->next->to;
	    rsp = rsp->next->to;
	    if ( osp==old->first )
	break;
	}
	old = old->next;
	rpl = rpl->next;
    }
}

RefChar *HasUseMyMetrics(SplineChar *sc,int layer) {
    RefChar *r;

    if ( layer==ly_grid ) layer = ly_fore;

    for ( r=sc->layers[layer].refs; r!=NULL; r=r->next )
	if ( r->use_my_metrics )
return( r );

return( NULL );
}

/* if they changed the width, then change the width on all bitmap chars of */
/*  ours, and if we are a letter, then change the width on all chars linked */
/*  to us which had the same width that we used to have (so if we change the */
/*  width of A, we'll also change that of À and Ä and ... */
void SCSynchronizeWidth(SplineChar *sc,real newwidth, real oldwidth, FontViewBase *flagfv) {
    BDFFont *bdf;
    struct splinecharlist *dlist;
    RefChar *r = HasUseMyMetrics(sc,ly_fore);
    int isprobablybase;

    sc->widthset = true;
    if( r!=NULL ) {
	if ( oldwidth==r->sc->width ) {
	    sc->width = r->sc->width;
return;
	}
	newwidth = r->sc->width;
    }
    if ( newwidth==oldwidth )
return;
    sc->width = newwidth;
    for ( bdf=sc->parent->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	BDFChar *bc = bdf->glyphs[sc->orig_pos];
	if ( bc!=NULL ) {
	    int width = rint(sc->width*bdf->pixelsize / (real) (sc->parent->ascent+sc->parent->descent));
	    if ( bc->width!=width ) {
		/*BCPreserveWidth(bc);*/ /* Bitmaps can't set width, so no undo for it */
		bc->width = width;
		BCCharChangedUpdate(bc);
	    }
	}
    }

    if ( !adjustwidth )
return;

    isprobablybase = isalpha(sc->unicodeenc) && !iscombining(sc->unicodeenc);

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next ) {
	RefChar *metrics = HasUseMyMetrics(dlist->sc,ly_fore);
	if ( metrics!=NULL && metrics->sc!=sc )
    continue;
	else if ( metrics==NULL && !isprobablybase )
    continue;
	if ( dlist->sc->width==oldwidth &&
		(metrics!=NULL || flagfv==NULL ||
		    !flagfv->selected[flagfv->map->backmap[dlist->sc->orig_pos]])) {
	    SCSynchronizeWidth(dlist->sc,newwidth,oldwidth,flagfv);
	    if ( !dlist->sc->changed ) {
		dlist->sc->changed = true;
		FVToggleCharChanged(dlist->sc);
	    }
	    SCUpdateAll(dlist->sc);
	}
    }
}

/* If they change the left bearing of a character, then in all chars */
/*  that depend on it should be adjusted too. */
/* Also all vstem hints */
/* I deliberately don't set undoes in the dependents. The change is not */
/*  in them, after all */
void SCSynchronizeLBearing(SplineChar *sc,real off,int layer) {
    struct splinecharlist *dlist;
    RefChar *ref;
    DStemInfo *d;
    StemInfo *h;
    HintInstance *hi;
    int isprobablybase;

    for ( h=sc->vstem; h !=NULL; h=h->next )
	h->start += off;
    for ( h=sc->hstem; h !=NULL; h=h->next )
	for ( hi = h->where; hi!=NULL; hi=hi->next ) {
	    hi->begin += off;
	    hi->end += off;
	}
    for ( d=sc->dstem; d !=NULL; d=d->next ) {
	d->left.x += off;
	d->right.x += off;
    }

    if ( !adjustlbearing )
return;

    isprobablybase = isalpha(sc->unicodeenc) && !iscombining(sc->unicodeenc);

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next ) {
	RefChar *metrics = HasUseMyMetrics(dlist->sc,layer);
	if ( metrics!=NULL && metrics->sc!=sc )
    continue;
	else if ( metrics==NULL && !isprobablybase )
    continue;
	else if ( metrics==NULL && sc->width!=dlist->sc->width )
    continue;
	SCPreserveLayer(dlist->sc,layer,false);
	SplinePointListShift(dlist->sc->layers[layer].splines,off,tpt_AllPoints);
	for ( ref = dlist->sc->layers[layer].refs; ref!=NULL; ref=ref->next )
		if ( ref->sc!=sc ) {
	    SplinePointListShift(ref->layers[0].splines,off,tpt_AllPoints);
	    ref->transform[4] += off;
	    ref->bb.minx += off; ref->bb.maxx += off;
	}
	SCUpdateAll(dlist->sc);
	SCSynchronizeLBearing(dlist->sc,off,layer);
    }
}

static int _SCRefNumberPoints2(SplineSet **_rss,SplineChar *sc,int pnum,int layer) {
    SplineSet *ss, *rss = *_rss;
    SplinePoint *sp, *rsp;
    RefChar *r;
    int starts_with_cp, startcnt;

    for ( ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next, rss=rss->next ) {
	if ( rss==NULL )		/* Can't happen */
    break;
	starts_with_cp = !ss->first->noprevcp &&
		((ss->first->ttfindex == pnum+1 && ss->first->prev!=NULL &&
		    ss->first->prev->from->nextcpindex==pnum ) ||
		 ((ss->first->ttfindex==0xffff || SPInterpolate( ss->first ))));
	startcnt = pnum;
	if ( starts_with_cp ) ++pnum;
	for ( sp = ss->first, rsp=rss->first; ; ) {
	    if ( sp->ttfindex==0xffff || SPInterpolate( sp ))
		rsp->ttfindex = 0xffff;
	    else
		rsp->ttfindex = pnum++;
	    if ( sp->next==NULL )
	break;
	    if ( sp->next!=NULL && sp->next->to == ss->first ) {
		if ( sp->nonextcp )
		    rsp->nextcpindex = 0xffff;
		else if ( starts_with_cp )
		    rsp->nextcpindex = startcnt;
		else
		    rsp->nextcpindex = pnum++;
	break;
	    }
	    if ( sp->nonextcp )
		rsp->nextcpindex = 0xffff;
	    else
		rsp->nextcpindex = pnum++;
	    if ( sp->next==NULL || rsp->next==NULL )
	break;
	    sp = sp->next->to;
	    rsp = rsp->next->to;
	}
    }

    *_rss = rss;
    for ( r = sc->layers[layer].refs; r!=NULL; r=r->next )
	pnum = _SCRefNumberPoints2(_rss,r->sc,pnum,layer);
return( pnum );
}

static int SCRefNumberPoints2(RefChar *ref,int pnum,int layer) {
    SplineSet *rss;

    rss = ref->layers[0].splines;
return( _SCRefNumberPoints2(&rss,ref->sc,pnum,layer));
}

int SSTtfNumberPoints(SplineSet *ss) {
    int pnum=0;
    SplinePoint *sp;
    int starts_with_cp;

    for ( ; ss!=NULL; ss=ss->next ) {
	starts_with_cp = !ss->first->noprevcp &&
		((ss->first->ttfindex == pnum+1 && ss->first->prev!=NULL &&
		  ss->first->prev->from->nextcpindex==pnum ) ||
		 SPInterpolate( ss->first ));
	if ( starts_with_cp && ss->first->prev!=NULL )
	    ss->first->prev->from->nextcpindex = pnum++;
	for ( sp=ss->first; ; ) {
	    if ( SPInterpolate(sp) )
		sp->ttfindex = 0xffff;
	    else
		sp->ttfindex = pnum++;
	    if ( sp->nonextcp && sp->nextcpindex!=pnum )
		sp->nextcpindex = 0xffff;
	    else if ( !starts_with_cp || (sp->next!=NULL && sp->next->to!=ss->first) )
		sp->nextcpindex = pnum++;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
return( pnum );
}

static int SSPsNumberPoints(SplineChar *sc, SplineSet *splines,int pnum) {
    SplineSet *ss;
    SplinePoint *sp;

    for ( ss = splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    sp->ttfindex = pnum++;
	    sp->nextcpindex = 0xffff;
	    if ( sc->numberpointsbackards ) {
		if ( sp->prev==NULL )
	break;
		if ( !sp->noprevcp || !sp->prev->from->nonextcp )
		    pnum += 2;
		sp = sp->prev->from;
	    } else {
		if ( sp->next==NULL )
	break;
		if ( !sp->nonextcp || !sp->next->to->noprevcp )
		    pnum += 2;
		sp = sp->next->to;
	    }
	    if ( sp==ss->first )
	break;
	}
    }
return( pnum );
}

int SCNumberPoints(SplineChar *sc,int layer) {
    int pnum=0;
    SplineSet *ss;
    SplinePoint *sp;
    RefChar *ref;

    if ( layer<0 || layer>=sc->layer_cnt )
        return( pnum );

    if ( sc->layers[layer].order2 ) {		/* TrueType and its complexities. I ignore svg here */
	if ( sc->layers[layer].refs!=NULL ) {
	    /* if there are references there can't be splines. So if we've got*/
	    /*  splines mark all point numbers on them as meaningless */
	    for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
		for ( sp=ss->first; ; ) {
		    sp->ttfindex = 0xfffe;
		    if ( !sp->nonextcp )
			sp->nextcpindex = 0xfffe;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==ss->first )
		break;
		}
	    }
	    for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next )
		pnum = SCRefNumberPoints2(ref,pnum,layer);
	} else {
	    pnum = SSTtfNumberPoints(sc->layers[layer].splines);
	}
    } else {		/* cubic (PostScript/SVG) splines */
	int first, last;
	if ( sc->parent->multilayer ) {
	    first = ly_fore;
	    last = sc->layer_cnt-1;
	} else
	    first = last = layer;
	for ( layer=first; layer<=last; ++layer ) {
	    for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next )
		pnum = SSPsNumberPoints(sc,ref->layers[0].splines,pnum);
	    pnum = SSPsNumberPoints(sc,sc->layers[layer].splines,pnum);
	}
    }
return( pnum );
}

int SCPointsNumberedProperly(SplineChar *sc,int layer) {
    int pnum=0, skipit;
    SplineSet *ss;
    SplinePoint *sp;
    int starts_with_cp;
    int start_pnum;

    if ( sc->layers[layer].splines!=NULL &&
	    sc->layers[layer].refs!=NULL )
return( false );	/* TrueType can't represent this, so always remove instructions. They can't be meaningful */

    for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	starts_with_cp = (ss->first->ttfindex == pnum+1 || ss->first->ttfindex==0xffff) &&
		!ss->first->noprevcp;
	start_pnum = pnum;
	if ( starts_with_cp ) ++pnum;
	for ( sp=ss->first; ; ) {
	    skipit = SPInterpolate(sp);
	    if ( sp->nonextcp || sp->noprevcp ) skipit = false;
	    if ( sp->ttfindex==0xffff && skipit )
		/* Doesn't count */;
	    else if ( sp->ttfindex!=pnum )
return( false );
	    else
		++pnum;
	    if ( sp->nonextcp && sp->nextcpindex==0xffff )
		/* Doesn't count */;
	    else if ( sp->nextcpindex==pnum )
		++pnum;
	    else if ( sp->nextcpindex==start_pnum && starts_with_cp &&
		    (sp->next!=NULL && sp->next->to==ss->first) )
		/* Ok */;
	    else
return( false );
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
	/* if ( starts_with_cp ) --pnum; */
    }
return( true );
}

void SCClearLayer(SplineChar *sc,int layer) {
    RefChar *refs, *next;

    SplinePointListsFree(sc->layers[layer].splines);
    sc->layers[layer].splines = NULL;
    for ( refs=sc->layers[layer].refs; refs!=NULL; refs = next ) {
	next = refs->next;
	SCRemoveDependent(sc,refs,layer);
    }
    sc->layers[layer].refs = NULL;
    ImageListsFree(sc->layers[layer].images);
    sc->layers[layer].images = NULL;
}

void SCClearContents(SplineChar *sc,int layer) {
    int ly_first, ly_last;

    if ( sc==NULL )
return;
    if ( sc->parent!=NULL && sc->parent->multilayer ) {
	ly_first = ly_fore;
	ly_last = sc->layer_cnt-1;
    } else
	ly_first = ly_last = layer;
    for ( layer = ly_first; layer<=ly_last; ++layer )
	SCClearLayer(sc,layer);
    --layer;

    if ( sc->parent!=NULL &&
	    (sc->parent->multilayer ||
		(!sc->parent->layers[layer].background && SCWasEmpty(sc,layer)))) {
	sc->widthset = false;
	if ( sc->parent!=NULL && sc->width!=0 )
	    sc->vwidth = sc->width = sc->parent->ascent+sc->parent->descent;
	AnchorPointsFree(sc->anchor);
	sc->anchor = NULL;
	StemInfosFree(sc->hstem); sc->hstem = NULL;
	StemInfosFree(sc->vstem); sc->vstem = NULL;
	DStemInfosFree(sc->dstem); sc->dstem = NULL;
	MinimumDistancesFree(sc->md); sc->md = NULL;
	free(sc->ttf_instrs);
	sc->ttf_instrs = NULL;
	sc->ttf_instrs_len = 0;
    }
}

void SCClearAll(SplineChar *sc,int layer) {
    extern int copymetadata;

    if ( sc==NULL )
return;
    if ( sc->layers[layer].splines==NULL && sc->layers[layer].refs==NULL && !sc->widthset &&
	    sc->hstem==NULL && sc->vstem==NULL && sc->anchor==NULL &&
	    !sc->parent->multilayer &&
	    (!copymetadata ||
		(sc->unicodeenc==-1 && strcmp(sc->name,".notdef")==0)))
return;
    SCPreserveLayer(sc,layer,2);
    if ( copymetadata ) {
	sc->unicodeenc = -1;
	free(sc->name);
	sc->name = copy(".notdef");
	PSTFree(sc->possub);
	sc->possub = NULL;
    }
    SCClearContents(sc,layer);
    SCCharChangedUpdate(sc,layer);
}

void SCClearBackground(SplineChar *sc) {

    if ( sc==NULL )
return;
    if ( sc->layers[0].splines==NULL && sc->layers[ly_back].images==NULL &&
	    sc->layers[0].refs==NULL )
return;
    SCPreserveBackground(sc);
    SCClearLayer(sc,ly_back);
    SCCharChangedUpdate(sc,ly_back);
}

void SCCopyLayerToLayer(SplineChar *sc, int from, int to,int doclear) {
    SplinePointList *fore, *temp;
    RefChar *ref, *oldref;

    SCPreserveLayer(sc,to,false);
    if ( doclear )
	SCClearLayer(sc,to);

    fore = SplinePointListCopy(sc->layers[from].splines);
    if ( !sc->layers[from].order2 && sc->layers[to].order2 ) {
	temp = SplineSetsTTFApprox(fore);
	SplinePointListsFree(fore);
	fore = temp;
    } else if ( sc->layers[from].order2 && !sc->layers[to].order2 ) {
	temp = SplineSetsPSApprox(fore);
	SplinePointListsFree(fore);
	fore = temp;
    }
    if ( fore!=NULL ) {
	for ( temp=fore; temp->next!=NULL; temp = temp->next );
	temp->next = sc->layers[to].splines;
	sc->layers[to].splines = fore;
    }

    if ( sc->layers[to].refs==NULL )
	sc->layers[to].refs = ref = RefCharsCopyState(sc,from);
    else {
	for ( oldref = sc->layers[to].refs; oldref->next!=NULL; oldref=oldref->next );
	oldref->next = ref = RefCharsCopyState(sc,from);
    }
    for ( ; ref!=NULL; ref=ref->next ) {
	SCReinstanciateRefChar(sc,ref,to);
	SCMakeDependent(sc,ref->sc);
    }
    SCCharChangedUpdate(sc,to);
}

int BpColinear(BasePoint *first, BasePoint *mid, BasePoint *last) {
    BasePoint dist_f, unit_f, dist_l, unit_l;
    bigreal len, off_l, off_f;

    dist_f.x = first->x - mid->x; dist_f.y = first->y - mid->y;
    len = sqrt( dist_f.x*dist_f.x + dist_f.y*dist_f.y );
    if ( len==0 )
return( false );
    unit_f.x = dist_f.x/len; unit_f.y = dist_f.y/len;

    dist_l.x = last->x - mid->x; dist_l.y = last->y - mid->y;
    len = sqrt( dist_l.x*dist_l.x + dist_l.y*dist_l.y );
    if ( len==0 )
return( false );
    unit_l.x = dist_l.x/len; unit_l.y = dist_l.y/len;

    off_f = dist_l.x*unit_f.y - dist_l.y*unit_f.x;
    off_l = dist_f.x*unit_l.y - dist_f.y*unit_l.x;
    if ( ( off_f<-1.5 || off_f>1.5 ) && ( off_l<-1.5 || off_l>1.5 ))
return( false );

return( true );
}

int BpWithin(BasePoint *first, BasePoint *mid, BasePoint *last) {
    BasePoint dist_mf, unit_mf, dist_lf, unit_lf;
    bigreal len, off_lf, off_mf, len2;

    dist_mf.x = mid->x - first->x; dist_mf.y = mid->y - first->y;
    len = sqrt( dist_mf.x*dist_mf.x + dist_mf.y*dist_mf.y );
    if ( len==0 )
return( true );
    unit_mf.x = dist_mf.x/len; unit_mf.y = dist_mf.y/len;

    dist_lf.x = last->x - first->x; dist_lf.y = last->y - first->y;
    len = sqrt( dist_lf.x*dist_lf.x + dist_lf.y*dist_lf.y );
    if ( len==0 )
return( false );
    unit_lf.x = dist_lf.x/len; unit_lf.y = dist_lf.y/len;

    off_mf = dist_lf.x*unit_mf.y - dist_lf.y*unit_mf.x;
    off_lf = dist_mf.x*unit_lf.y - dist_mf.y*unit_lf.x;
    if ( ( off_mf<-.1 || off_mf>.1 ) && ( off_lf<-.1 || off_lf>.1 ))
return( false );

    len2 = dist_mf.x*unit_lf.x + dist_mf.y*unit_lf.y;
return( len2>=0 && len2<=len );
}

/* This is a fast computable measure for linearity. */
/* First, we norm the spline: Scale and rotate the spline such that */
/* one end lies on (0,0) and the other on (100,0). */
/* Then the maximal y coordinate of the horizontal extrema is returned. */
static bigreal Linearity(Spline *s) {
    if ( s->islinear ) return 0;  
    BasePoint ftunit = BPSub(s->to->me, s->from->me);
    bigreal ftlen = BPNorm(ftunit);
    if ( ftlen==0 ) return -1; /* flag for error, no norming possible */
    ftunit = BPScale(ftunit, 1/ftlen);
    BasePoint nextcpscaled = BPScale(BPSub(s->from->nextcp, s->from->me),100./ftlen);
    if ( s->order2 ) return .5*fabs(BPCross(ftunit, nextcpscaled));
    BasePoint prevcpscaled = BPScale(BPSub(s->to->prevcp, s->from->me),100./ftlen);
    /* we just look a the bezier coefficients of the polynomial in t */
    /* in y dimension (divided by 3): */
    Spline1D ys1d;
    ys1d.d = 0;
    ys1d.c = BPCross(ftunit, nextcpscaled); /* rotate it to horizontal*/
    ys1d.b = BPCross(ftunit, prevcpscaled)-2*ys1d.c;
    ys1d.a = -ys1d.b-ys1d.c;
    extended te1, te2;
    SplineFindExtrema(&ys1d, &te1, &te2);
    if ( te1==-1 && te2==-1 ) return 0;
    if ( te2==-1 ) return 3*fabs(((ys1d.a*te1+ys1d.b)*te1+ys1d.c)*te1);
return 3*fmax(fabs(((ys1d.a*te1+ys1d.b)*te1+ys1d.c)*te1), 
fabs(((ys1d.a*te2+ys1d.b)*te2+ys1d.c)*te2));
}

void SPChangePointType(SplinePoint *sp, int pointtype) {
    BasePoint unitnext, unitprev;
    bigreal nextlen, prevlen;
    int makedflt;
    int oldpointtype = sp->pointtype;

    if ( sp->pointtype==pointtype ) {
	if ( pointtype==pt_curve || pointtype == pt_hvcurve ) {
	    if ( !sp->nextcpdef && sp->next!=NULL && !sp->next->order2 )
		SplineCharDefaultNextCP(sp);
	    if ( !sp->prevcpdef && sp->prev!=NULL && !sp->prev->order2 )
		SplineCharDefaultPrevCP(sp);
	}
return;
    }
    sp->pointtype = pointtype;

    if ( pointtype==pt_corner ) {
	/* Leave control points as they are */;
	sp->nextcpdef = sp->nonextcp;
	sp->prevcpdef = sp->noprevcp;
    } else if ( pointtype==pt_tangent ) {
	if ( sp->next!=NULL && sp->prev!=NULL ) {
		bigreal prevlinearity = Linearity(sp->prev);
		bigreal nextlinearity = Linearity(sp->next);
		if ( prevlinearity>=0 && nextlinearity>=0 ) {
			if ( nextlinearity >= prevlinearity ) { /* make prev linear */
				sp->prev->islinear = true;
				sp->prev->from->nextcp = sp->prev->from->me;
				sp->prevcp = sp->me; 
				if ( sp->prev->from->pointtype!=pt_tangent) 
					sp->prev->from->pointtype = pt_corner;
				SplineRefigure(sp->prev); /* display straight line */
				if ( sp->next->order2 ) {
					BasePoint inter;
					if ( IntersectLines(&inter,&sp->me,&sp->prev->from->me,&sp->nextcp,&sp->next->to->me) ) {
						sp->nextcp = inter;
						sp->next->to->prevcp = inter;
						SplineRefigure(sp->next); /* update curve */
					} /* else just leave things as they are */
				} else {
					unitnext = NormVec(BPSub(sp->me,sp->prev->from->me));
					sp->nextcp = BPAdd(sp->me, BPScale(unitnext,fabs(BPDot(BPSub(sp->nextcp, sp->me), unitnext))));
					SplineRefigure(sp->next); /* update curve */
				}
			} else { /* make next linear */
				sp->next->islinear = true;
				sp->next->to->prevcp = sp->next->to->me;
				sp->nextcp = sp->me; 
				if ( sp->next->to->pointtype!=pt_tangent) 
					sp->next->to->pointtype = pt_corner;
				SplineRefigure(sp->next); /* display straight line */
				if ( sp->prev->order2 ) {
					BasePoint inter;
					if ( IntersectLines(&inter,&sp->me,&sp->next->to->me,&sp->prevcp,&sp->prev->from->me) ) {
						sp->prevcp = inter;
						sp->prev->from->nextcp = inter;
						SplineRefigure(sp->prev); /* update curve */
					} /* else just leave things as they are */
				} else {
					unitprev = NormVec(BPSub(sp->me,sp->next->to->me));
					sp->prevcp = BPAdd(sp->me, BPScale(unitprev,fabs(BPDot(BPSub(sp->prevcp, sp->me), unitprev))));
					SplineRefigure(sp->prev); /* update curve */
				}
			}
		} /* else do nothing - this would not make any sense */
	}
    } else if ( pointtype!=pt_curve
		&& ((BpColinear(&sp->prevcp,&sp->me,&sp->nextcp) ||
	    ( sp->nonextcp ^ sp->noprevcp )) &&
	    ( pointtype!=pt_hvcurve ||
		(sp->nextcp.x == sp->me.x && sp->nextcp.y != sp->me.y ) ||
	      (sp->nextcp.y == sp->me.y && sp->nextcp.x != sp->me.x ) )))
    {
	/* Retain the old control points */
    }
    else
    {
	unitnext.x = sp->nextcp.x-sp->me.x; unitnext.y = sp->nextcp.y-sp->me.y;
	nextlen = sqrt(unitnext.x*unitnext.x + unitnext.y*unitnext.y);
	unitprev.x = sp->prevcp.x-sp->me.x; unitprev.y = sp->prevcp.y-sp->me.y;
	prevlen = sqrt(unitprev.x*unitprev.x + unitprev.y*unitprev.y);
	makedflt=true;
	if ( nextlen!=0 && prevlen!=0 ) {
	    unitnext.x /= nextlen; unitnext.y /= nextlen;
	    unitprev.x /= prevlen; unitprev.y /= prevlen;
	    if ( unitnext.x*unitprev.x + unitnext.y*unitprev.y<=-.95 ) {
		/* If the control points are essentially in the same direction*/
		/*  (so valid for a curve) then leave them as is */
		makedflt = false;
	    }
	}
	if ( pointtype==pt_hvcurve &&
		((unitnext.x!=0 && unitnext.y!=0) ||
		 (unitprev.x!=0 && unitprev.y!=0)) ) {
	    BasePoint ncp, pcp;
	    if ( fabs(unitnext.x)+fabs(unitprev.x)>fabs(unitnext.y)+fabs(unitprev.y) ) {
		unitnext.y = unitprev.y = 0;
		unitnext.x = unitnext.x>0 ? 1 : -1;
		unitprev.x = unitprev.x>0 ? 1 : -1;
	    } else {
		unitnext.x = unitprev.x = 0;
		unitnext.y = unitnext.y>0 ? 1 : -1;
		unitprev.y = unitprev.y>0 ? 1 : -1;
	    }
	    ncp.x = sp->me.x + unitnext.x*nextlen;
	    ncp.y = sp->me.y + unitnext.y*nextlen;
	    sp->nextcp = ncp;
	    if ( sp->next!=NULL && sp->next->order2 )
			sp->next->to->prevcp = ncp;
		SplineRefigure(sp->next);
	    pcp.x = sp->me.x + unitprev.x*prevlen;
	    pcp.y = sp->me.y + unitprev.y*prevlen;
	    sp->prevcp = pcp;
	    if ( sp->prev!=NULL && sp->prev->order2 )
			sp->prev->from->nextcp = pcp;
		SplineRefigure(sp->prev);
	    makedflt = false;
	}
	if( pointtype==pt_curve ) { 
		if ( oldpointtype==pt_corner ) { 
			makedflt = false; 	
			if ( sp->prev!=NULL && sp->next!=NULL) {
				if ( prevlen!=0 && nextlen!=0 ) { /* take the average direction */
					sp->nextcp = BPAdd(sp->me, BPScale(BPSub(unitnext, unitprev), .5*nextlen));
					sp->prevcp = BPSub(sp->me, BPScale(BPSub(unitnext, unitprev), .5*prevlen));	
					if ( sp->next->order2 ) { /* sp->nextcp and sp->next->to->prevcp are not necessary equal */
						BasePoint inter; 
						if ( IntersectLines(&inter,&sp->me,&sp->nextcp,&sp->next->to->prevcp,&sp->next->to->me) 
						/* check if inter is on the same side of sp->me as sp->nextcp: */ 
						&& BPDot( BPSub(inter, sp->me), BPSub(sp->nextcp, sp->me) ) >= 0 
						/* check if inter is on the same side of sp->next->to->me as sp->nextcp: */ 
						&& BPDot( BPSub(inter, sp->me), BPSub(sp->next->to->me, sp->me) ) >= 0 ) {
							sp->nextcp = inter;
							sp->next->to->prevcp = inter;
							SplineRefigure(sp->next);
						} else { /* undo things, the user has to interact (no clear solution) */
							sp->nextcp = sp->next->to->prevcp;
						}
					}
					if ( sp->prev->order2 ) { /* sp->prevcp and sp->prev->from->nextcp are not necessary equal */
						BasePoint inter; /* this is suboptimal when the intersection is on the wrong side */
						if ( IntersectLines(&inter,&sp->me,&sp->prevcp,&sp->prev->from->nextcp,&sp->prev->from->me) 
						/* check if inter is on the same side of sp->me as sp->prevcp: */ 
						&& BPDot( BPSub(inter, sp->me), BPSub(sp->prevcp, sp->me) ) >= 0 
						/* check if inter is on the same side of sp->prev->from->me as sp->prevcp: */ 
						&& BPDot( BPSub(inter, sp->me), BPSub(sp->prev->from->me, sp->me) ) >= 0 ) {
							sp->prevcp = inter;
							sp->prev->from->nextcp = inter;
							SplineRefigure(sp->prev);
						} else { /* undo things, the user has to interact (no clear solution) */
							sp->prevcp = sp->prev->from->nextcp;
						}
					}
				} else if ( prevlen!=0 && !sp->next->order2 ) { /* and therefore nextlen==0 */
					sp->nextcp = BPSub(sp->me, BPScale(NormVec(unitprev), 
					NICE_PROPORTION*BPNorm(BPSub(sp->next->to->me, sp->me))));
				} else if ( nextlen!=0 && !sp->prev->order2 ) { /* and therefore prevlen==0 */
					sp->prevcp = BPSub(sp->me, BPScale(NormVec(unitnext), 
					NICE_PROPORTION*BPNorm(BPSub(sp->prev->from->me, sp->me))));
				} else makedflt = true;
			}
		} else if ( oldpointtype==pt_tangent || oldpointtype==pt_hvcurve ) { 
			makedflt = false; 	
		} else makedflt = true; /* original behaviour */
	}
	
	if ( makedflt ) {
	    sp->nextcpdef = sp->prevcpdef = true;
	    if (( sp->prev!=NULL && sp->prev->order2 ) ||
		    (sp->next!=NULL && sp->next->order2)) {
		if ( sp->prev!=NULL )
		    SplineRefigureFixup(sp->prev);
		if ( sp->next!=NULL )
		    SplineRefigureFixup(sp->next);
	    } else {
		SplineCharDefaultPrevCP(sp);
		SplineCharDefaultNextCP(sp);
	    }
	}
    }
    /* Now in order2 splines it is possible to request combinations that are */
    /*  mathematically impossible -- two adjacent hv points often don't work */
    if ( pointtype==pt_hvcurve &&
		!(sp->nextcp.x == sp->me.x && sp->nextcp.y != sp->me.y ) &&
		!(sp->nextcp.y == sp->me.y && sp->nextcp.x != sp->me.x ) )
	sp->pointtype = pt_curve;
}

void SplinePointRound(SplinePoint *sp,real factor) {
    BasePoint noff, poff;

    if ( sp->prev!=NULL && sp->next!=NULL && sp->next->order2 &&
	    sp->ttfindex == 0xffff ) {
	/* For interpolated points we want to round the controls */
	/* and then interpolated based on that */
	sp->nextcp.x = rint(sp->nextcp.x*factor)/factor;
	sp->nextcp.y = rint(sp->nextcp.y*factor)/factor;
	sp->prevcp.x = rint(sp->prevcp.x*factor)/factor;
	sp->prevcp.y = rint(sp->prevcp.y*factor)/factor;
	sp->me.x = (sp->nextcp.x + sp->prevcp.x)/2;
	sp->me.y = (sp->nextcp.y + sp->prevcp.y)/2;
    } else {
	/* For normal points we want to round the distance of the controls */
	/*  from the base */
	noff.x = rint((sp->nextcp.x - sp->me.x)*factor)/factor;
	noff.y = rint((sp->nextcp.y - sp->me.y)*factor)/factor;
	poff.x = rint((sp->prevcp.x - sp->me.x)*factor)/factor;
	poff.y = rint((sp->prevcp.y - sp->me.y)*factor)/factor;

	sp->me.x = rint(sp->me.x*factor)/factor;
	sp->me.y = rint(sp->me.y*factor)/factor;

	sp->nextcp.x = sp->me.x + noff.x;
	sp->nextcp.y = sp->me.y + noff.y;
	sp->prevcp.x = sp->me.x + poff.x;
	sp->prevcp.y = sp->me.y + poff.y;
    }
    if ( sp->next!=NULL && sp->next->order2 )
	sp->next->to->prevcp = sp->nextcp;
    if ( sp->prev!=NULL && sp->prev->order2 )
	sp->prev->from->nextcp = sp->prevcp;
}

static void SpiroRound2Int(spiro_cp *cp,real factor) {
    cp->x = rint(cp->x*factor)/factor;
    cp->y = rint(cp->y*factor)/factor;
}

void SplineSetsRound2Int(SplineSet *spl,real factor, int inspiro, int onlysel) {
    SplinePoint *sp;
    int i;

    for ( ; spl!=NULL; spl=spl->next ) {
	if ( inspiro && spl->spiro_cnt!=0 ) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		if ( !onlysel || SPIRO_SELECTED(&spl->spiros[i]) )
		    SpiroRound2Int(&spl->spiros[i],factor);
	    SSRegenerateFromSpiros(spl);
	} else {
	    SplineSetSpirosClear(spl);
	    for ( sp=spl->first; ; ) {
		if ( sp->selected || !onlysel )
		    SplinePointRound(sp,factor);
		if ( sp->prev!=NULL )
		    SplineRefigure(sp->prev);
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	    if ( spl->first->prev!=NULL )
		SplineRefigure(spl->first->prev);
	}
    }
}

static void SplineSetsChangeCoord(SplineSet *spl,real old, real new,int isy,
	int inspiro) {
    SplinePoint *sp;
    int changed;
    int i;

    for ( ; spl!=NULL; spl=spl->next ) {
	changed = false;
	if ( inspiro ) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i ) {
		if ( isy && RealNear(spl->spiros[i].y,old)) {
		    spl->spiros[i].y = new;
		    changed = true;
		} else if ( !isy && RealNear(spl->spiros[i].x,old)) {
		    spl->spiros[i].x = new;
		    changed = true;
		}
	    }
	/* SSRegenerateFromSpiros will be done in Round2Int */
	} else {
	    for ( sp=spl->first; ; ) {
		if ( isy ) {
		    if ( RealNear(sp->me.y,old) ) {
			if ( RealNear(sp->nextcp.y,old))
			    sp->nextcp.y = new;
			else
			    sp->nextcp.y += new-sp->me.y;
			if ( RealNear(sp->prevcp.y,old))
			    sp->prevcp.y = new;
			else
			    sp->prevcp.y += new-sp->me.y;
			sp->me.y = new;
			changed = true;
			/* we expect to be called before SplineSetRound2Int and will */
			/*  allow it to do any SplineRefigures */
		    }
		} else {
		    if ( RealNear(sp->me.x,old) ) {
			if ( RealNear(sp->nextcp.x,old))
			    sp->nextcp.x = new;
			else
			    sp->nextcp.x += new-sp->me.x;
			if ( RealNear(sp->prevcp.x,old))
			    sp->prevcp.x = new;
			else
			    sp->prevcp.x += new-sp->me.x;
			sp->me.x = new;
			changed = true;
		    }
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	    if ( changed )
		SplineSetSpirosClear(spl);
	}
    }
}

void SCRound2Int(SplineChar *sc,int layer, real factor) {
    RefChar *r;
    AnchorPoint *ap;
    StemInfo *stems;
    real old, new;
    int first, last;

    for ( stems = sc->hstem; stems!=NULL; stems=stems->next ) {
	old = stems->start+stems->width;
	stems->start = rint(stems->start*factor)/factor;
	stems->width = rint(stems->width*factor)/factor;
	new = stems->start+stems->width;
	if ( old!=new )
	    SplineSetsChangeCoord(sc->layers[ly_fore].splines,old,new,true,sc->inspiro && hasspiro());
    }
    for ( stems = sc->vstem; stems!=NULL; stems=stems->next ) {
	old = stems->start+stems->width;
	stems->start = rint(stems->start*factor)/factor;
	stems->width = rint(stems->width*factor)/factor;
	new = stems->start+stems->width;
	if ( old!=new )
	    SplineSetsChangeCoord(sc->layers[ly_fore].splines,old,new,false,sc->inspiro && hasspiro());
    }

    if ( sc->parent->multilayer ) {
	first = ly_fore;
	last = sc->layer_cnt-1;
    } else
	first = last = layer;
    for ( layer = first; layer<=last; ++layer ) {
	SplineSetsRound2Int(sc->layers[layer].splines,factor,sc->inspiro && hasspiro(),false);
	for ( r=sc->layers[layer].refs; r!=NULL; r=r->next ) {
	    r->transform[4] = rint(r->transform[4]*factor)/factor;
	    r->transform[5] = rint(r->transform[5]*factor)/factor;
	    RefCharFindBounds(r);
	}
    }
    if ( sc->parent->multilayer )
	layer = ly_fore;
    else
	--layer;

    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	ap->me.x = rint(ap->me.x*factor)/factor;
	ap->me.y = rint(ap->me.y*factor)/factor;
    }
    SCCharChangedUpdate(sc,layer);
}

void AltUniRemove(SplineChar *sc,int uni) {
    struct altuni *altuni, *prev;

    if ( sc==NULL || uni==-1 )
return;

    if ( sc->unicodeenc==uni ) {
	for ( altuni = sc->altuni; altuni!=NULL; altuni=altuni->next )
	    if ( altuni->fid==0 && altuni->vs==-1 )
	break;
	if ( altuni!=NULL ) {
	    sc->unicodeenc = altuni->unienc;
	    altuni->unienc = uni;
	}
    }

    if ( sc->unicodeenc==uni )
return;
    for ( prev=NULL, altuni=sc->altuni; altuni!=NULL && (altuni->unienc!=uni || altuni->vs==-1 || altuni->fid!=0);
	    prev = altuni, altuni = altuni->next );
    if ( altuni ) {
	if ( prev==NULL )
	    sc->altuni = altuni->next;
	else
	    prev->next = altuni->next;
	altuni->next = NULL;
	AltUniFree(altuni);
    }
}

void AltUniAdd(SplineChar *sc,int uni) {
    struct altuni *altuni;

    if ( sc!=NULL && uni!=-1 && uni!=sc->unicodeenc ) {
	for ( altuni = sc->altuni; altuni!=NULL && (altuni->unienc!=uni ||
						    altuni->vs!=-1 ||
			                            altuni->fid); altuni=altuni->next );
	if ( altuni==NULL ) {
	    altuni = chunkalloc(sizeof(struct altuni));
	    altuni->next = sc->altuni;
	    sc->altuni = altuni;
	    altuni->unienc = uni;
	    altuni->vs = -1;
	    altuni->fid = 0;
	}
    }
}

void AltUniAdd_DontCheckDups(SplineChar *sc,int uni) {
    struct altuni *altuni;

    if ( sc!=NULL && uni!=-1 && uni!=sc->unicodeenc ) {
	altuni = chunkalloc(sizeof(struct altuni));
	altuni->next = sc->altuni;
	sc->altuni = altuni;
	altuni->unienc = uni;
	altuni->vs = -1;
	altuni->fid = 0;
    }
}

void SCOrderAP(SplineChar *sc) {
    int lc=0, cnt=0, out=false, i,j;
    AnchorPoint *ap, **array;
    /* Order so that first ligature index comes first */

    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	if ( ap->lig_index<lc ) out = true;
	if ( ap->lig_index>lc ) lc = ap->lig_index;
	++cnt;
    }
    if ( !out )
return;

    array = malloc(cnt*sizeof(AnchorPoint *));
    for ( i=0, ap=sc->anchor; ap!=NULL; ++i, ap=ap->next )
	array[i] = ap;
    for ( i=0; i<cnt-1; ++i ) {
	for ( j=i+1; j<cnt; ++j ) {
	    if ( array[i]->lig_index>array[j]->lig_index ) {
		ap = array[i];
		array[i] = array[j];
		array[j] = ap;
	    }
	}
    }
    sc->anchor = array[0];
    for ( i=0; i<cnt-1; ++i )
	array[i]->next = array[i+1];
    array[cnt-1]->next = NULL;
    free( array );
}

void UnlinkThisReference(FontViewBase *fv,SplineChar *sc,int layer) {
    /* We are about to clear out sc. But somebody refers to it and that we */
    /*  aren't going to delete. So (if the user asked us to) instantiate sc */
    /*  into all characters which refer to it and which aren't about to be */
    /*  cleared out */
    struct splinecharlist *dep, *dnext;

    for ( dep=sc->dependents; dep!=NULL; dep=dnext ) {
	dnext = dep->next;
	if ( fv==NULL || !fv->selected[fv->map->backmap[dep->sc->orig_pos]]) {
	    SplineChar *dsc = dep->sc;
	    RefChar *rf, *rnext;
	    /* May be more than one reference to us, colon has two refs to period */
	    /*  but only one dlist entry */
	    for ( rf = dsc->layers[layer].refs; rf!=NULL; rf=rnext ) {
		rnext = rf->next;
		if ( rf->sc == sc ) {
		    /* Even if we were to preserve the state there would be no */
		    /*  way to undo the operation until we undid the delete... */
		    SCRefToSplines(dsc,rf,layer);
		    SCUpdateAll(dsc);
		}
	    }
	}
    }
}

static int MultipleNames(const char *name) {
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_Cancel"); buts[2] = NULL;
    if ( ff_ask(_("Multiple"),(const char **) buts,0,1,_("There is already a glyph with name %.40s,\ndo you want to swap names?"), name)==0 )
return( true );

return( false );
}

int SCSetMetaData(SplineChar *sc,const char *name,int unienc,const char *comment) {
    SplineFont *sf = sc->parent;
    int samename=false, sameuni=false;
    struct altuni *alt;

    if ( sf->glyphs[sc->orig_pos]!=sc )
	IError("Bad call to SCSetMetaData");

    for ( alt=sc->altuni; alt!=NULL && (alt->unienc!=unienc || alt->vs!=-1 || alt->fid!=0); alt=alt->next );
    if ( unienc==sc->unicodeenc || alt!=NULL )
	sameuni=true;
    if ( sameuni && strcmp(name,sc->name)==0 ) {
	samename = true;	/* No change, it must be good */
    }
    if ( alt!=NULL || !samename ) {
        int i;
        if (unienc != -1) {
            for (i = 0; i < sf->glyphcnt; ++i)
                if (sf->glyphs[i] != NULL &&
                    sf->glyphs[i]->orig_pos != sc->orig_pos &&
                    sf->glyphs[i]->unicodeenc == unienc) {
                    /* Duplicate encoding is never allowed */
                    ff_post_notice(_("Duplicate encodings"),
                                   _("There is already a glyph with Unicode "
                                     "encoding 0x%1$04x named %2$.40s. The "
                                     "current name and encoding will be kept."),
                                   unienc, sf->glyphs[i]->name);
                    return false;
                }
            /* Proceed to copy encoding and name. */
        } else {
	    /* For unencoded glyphs, check name duplication and propose to swap. */
            int isnotdef = strcmp(name, ".notdef") == 0;
            SplineChar* swap_sc = NULL;
            for (i = 0; i < sf->glyphcnt && swap_sc == NULL; ++i)
                if (!isnotdef && strcmp(name, sf->glyphs[i]->name) == 0) {
                    if (!MultipleNames(name))
                        return false;
                    else
                        swap_sc = sf->glyphs[i];
                }
            if (swap_sc) {
                free(swap_sc->name);
                swap_sc->namechanged = true;
                if (strncmp(sc->name, "uni", 3) == 0 &&
                    swap_sc->unicodeenc != -1) {
                    if (swap_sc->unicodeenc < 0x10000)
                        swap_sc->name =
                            smprintf("uni%04X", swap_sc->unicodeenc);
                    else
                        swap_sc->name = smprintf("u%04X", swap_sc->unicodeenc);
                } else {
                    swap_sc->name = sc->name;
                    sc->name = NULL;
                }
            }
        }
	if ( sc->unicodeenc!=unienc ) {
	    struct splinecharlist *scl;
	    int layer;
	    RefChar *ref;

	    for ( scl=sc->dependents; scl!=NULL; scl=scl->next ) {
		for ( layer=ly_back; layer<scl->sc->layer_cnt; ++layer )
		    for ( ref = scl->sc->layers[layer].refs; ref!=NULL; ref=ref->next )
			if ( ref->sc==sc )
			    ref->unicode_enc = unienc;
	    }
	}
    }
    if ( alt!=NULL )
	alt->unienc = sc->unicodeenc;
    sc->unicodeenc = unienc;
    if ( sc->name==NULL || strcmp(name,sc->name)!=0 ) {
	if ( sc->name!=NULL )
	    SFGlyphRenameFixup(sf,sc->name,name,false);
	free(sc->name);
	sc->name = copy(name);
	sc->namechanged = true;
	GlyphHashFree(sf);
    }
    sf->changed = true;
    if ( samename )
	/* Ok to name it itself */;
    else if ( sameuni && ( unienc>=0xe000 && unienc<=0xf8ff ))
	/* Ok to name things in the private use area */;
    else {
	FontViewBase *fvs;
	for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	    int enc = fvs->map->backmap[sc->orig_pos];
	    if ( enc!=-1 && ((fvs->map->enc->only_1byte && enc<256) ||
			(fvs->map->enc->has_2byte && enc<65535 ))) {
		fvs->map->enc = &custom;
		FVSetTitle(fvs);
	    }
	}
    }
    free(sc->comment); sc->comment = NULL;
    if ( comment!=NULL && *comment!='\0' )
	sc->comment = copy(comment);

    SCRefreshTitles(sc);
return( true );
}

void RevertedGlyphReferenceFixup(SplineChar *sc, SplineFont *sf) {
    RefChar *refs, *prev, *next;
    KernPair *kp, *kprev, *knext;
    SplineFont *cidmaster = sf, *ksf;
    int layer, isv, l;

    for ( layer = 0; layer<sc->layer_cnt; ++layer ) {
	for ( prev=NULL, refs = sc->layers[layer].refs ; refs!=NULL; refs = next ) {
	    next = refs->next;
	    if ( refs->orig_pos<sf->glyphcnt && sf->glyphs[refs->orig_pos]!=NULL ) {
		prev = refs;
		refs->sc = sf->glyphs[refs->orig_pos];
		refs->unicode_enc = refs->sc->unicodeenc;
		SCReinstanciateRefChar(sc,refs,layer);
		SCMakeDependent(sc,refs->sc);
	    } else {
		if ( prev==NULL )
		    sc->layers[layer].refs = next;
		else
		    prev->next = next;
		RefCharFree(refs);
	    }
	}
    }
    /* Fixup kerning pairs as well */
    for ( isv=0; isv<2; ++isv ) {
	for ( kprev = NULL, kp=isv?sc->vkerns : sc->kerns; kp!=NULL; kp=knext ) {
	    int index = (intptr_t) (kp->sc);
	    knext = kp->next;
	    kp->kcid = false;
	    ksf = sf;
	    if ( cidmaster!=sf ) {
		for ( l=0; l<cidmaster->subfontcnt; ++l ) {
		    ksf = cidmaster->subfonts[l];
		    if ( index<ksf->glyphcnt && ksf->glyphs[index]!=NULL )
	    break;
		}
	    }
	    if ( index>=ksf->glyphcnt || ksf->glyphs[index]==NULL ) {
		IError( "Bad kerning information in glyph %s\n", sc->name );
		kp->sc = NULL;
	    } else
		kp->sc = ksf->glyphs[index];
	    if ( kp->sc!=NULL )
		kprev = kp;
	    else{
		if ( kprev!=NULL )
		    kprev->next = knext;
		else if ( isv )
		    sc->vkerns = knext;
		else
		    sc->kerns = knext;
		chunkfree(kp,sizeof(KernPair));
	    }
	}
    }
}

static int CheckBluePair(char *blues, char *others, int bluefuzz,
	int magicpointsize) {
    int bound = 2*bluefuzz+1;
    int bluevals[10+14], cnt, pos=0, maxzoneheight;
    int err = 0;
    char *end;

    if ( others!=NULL ) {
	while ( *others==' ' ) ++others;
	if ( *others=='[' || *others=='{' ) ++others;
	for ( cnt=0; ; ++cnt ) {
	    bigreal temp;
	    while ( *others==' ' ) ++others;
	    if ( *others==']' || *others=='}' )
	break;
	    temp = ff_strtod(others,&end);
	    if ( temp!=rint(temp))
		err |= pds_notintegral;
	    else if ( end==others ) {
		err |= pds_notintegral;
	break;
	    }
	    others = end;
	    if ( cnt>=10 )
		err |= pds_toomany;
	    else
		bluevals[pos++] = temp;
	}
	if ( cnt&1 )
	    err |= pds_odd;
    }

    while ( *blues==' ' ) ++blues;
    if ( *blues=='{' || *blues=='[' ) ++blues;
    for ( cnt=0; ; ++cnt ) {
	bigreal temp;
	while ( *blues==' ' ) ++blues;
	if ( *blues==']' || *blues=='}' )
    break;
	temp = ff_strtod(blues,&end);
	if ( temp!=rint(temp))
	    err |= pds_notintegral;
	else if ( end==blues ) {
	    err |= pds_notintegral;
    break;
	}
	blues = end;
	if ( cnt>=14 )
	    err |= pds_toomany;
	else
	    bluevals[pos++] = temp;
    }
    if ( cnt&1 )
	err |= pds_odd;

    /* Now there is nothing which says that otherblues must all be less than */
    /*  blues. But the examples suggest it. And I shall assume it */

    maxzoneheight = -1;
    for ( cnt=0; cnt<pos; cnt+=2 ) {
	if ( cnt+1<pos && bluevals[cnt]>bluevals[cnt+1] )
	    err |= pds_outoforder;
	else if ( cnt+1<pos && maxzoneheight<bluevals[cnt+1]-bluevals[cnt] )
	    maxzoneheight = bluevals[cnt+1]-bluevals[cnt];
	if ( cnt!=0 && bluevals[cnt-1]>=bluevals[cnt] )
	    err |= pds_outoforder;
	if ( cnt!=0 && bluevals[cnt-1]+bound>bluevals[cnt] )
	    err |= pds_tooclose;
    }

    if ( maxzoneheight>0 && (magicpointsize-.49)*maxzoneheight>=240 )
	err |= pds_toobig;

return( err );
}

static int CheckStdW(struct psdict *dict,char *key ) {
    char *str_val, *end;
    bigreal val;

    if ( (str_val = PSDictHasEntry(dict,key))==NULL )
return( true );
    while ( *str_val==' ' ) ++str_val;
    if ( *str_val!='[' && *str_val!='{' )
return( false );
    ++str_val;

    val = ff_strtod(str_val,&end);
    while ( *end==' ' ) ++end;
    if ( *end!=']' && *end!='}' )
return( false );
    ++end;
    while ( *end==' ' ) ++end;
    if ( *end!='\0' || end==str_val || val<=0 )
return( false );

return( true );
}

static int CheckStemSnap(struct psdict *dict,char *snapkey, char *stdkey ) {
    char *str_val, *end;
    bigreal std_val = -1;
    bigreal stems[12], temp;
    int cnt, found;
    /* At most 12 double values, in order, must include Std?W value, array */

    if ( (str_val = PSDictHasEntry(dict,stdkey))!=NULL ) {
	while ( *str_val==' ' ) ++str_val;
	if ( *str_val=='[' && *str_val!='{' ) ++str_val;
	std_val = ff_strtod(str_val,&end);
    }

    if ( (str_val = PSDictHasEntry(dict,snapkey))==NULL )
return( true );		/* This entry is not required */
    while ( *str_val==' ' ) ++str_val;
    if ( *str_val!='[' && *str_val!='{' )
return( false );
    ++str_val;

    found = false;
    for ( cnt=0; ; ++cnt ) {
	while ( *str_val==' ' ) ++str_val;
	if ( *str_val==']' && *str_val!='}' )
    break;
	temp = ff_strtod(str_val,&end);
	if ( end==str_val )
return( false );
	str_val = end;
	if ( cnt>=12 )
return( false );
	stems[cnt] = temp;
	if ( cnt>0 && stems[cnt-1]>=stems[cnt] )
return( false );
	if ( stems[cnt] == std_val )
	    found = true;
    }
    if ( !found && std_val>0 )
return( -1 );

return( true );
}

int ValidatePrivate(SplineFont *sf) {
    int errs = 0;
    char *blues, *bf, *test, *end;
    int fuzz = 1;
    bigreal bluescale = .039625;
    int magicpointsize;

    if ( sf->private_dict==NULL )
return( pds_missingblue );

    if ( (bf = PSDictHasEntry(sf->private_dict,"BlueFuzz"))!=NULL ) {
	fuzz = strtol(bf,&end,10);
	if ( *end!='\0' || fuzz<0 )
	    errs |= pds_badbluefuzz;
    }

    if ( (test=PSDictHasEntry(sf->private_dict,"BlueScale"))!=NULL ) {
	bluescale = ff_strtod(test,&end);
	if ( *end!='\0' || end==test || bluescale<0 )
	    errs |= pds_badbluescale;
    }
    magicpointsize = rint( bluescale*240 + 0.49 );

    if ( (blues = PSDictHasEntry(sf->private_dict,"BlueValues"))==NULL )
	errs |= pds_missingblue;
    else
	errs |= CheckBluePair(blues,PSDictHasEntry(sf->private_dict,"OtherBlues"),fuzz,magicpointsize);

    if ( (blues = PSDictHasEntry(sf->private_dict,"FamilyBlues"))!=NULL )
	errs |= CheckBluePair(blues,PSDictHasEntry(sf->private_dict,"FamilyOtherBlues"),
		fuzz,magicpointsize)<<pds_shift;


    if ( (test=PSDictHasEntry(sf->private_dict,"BlueShift"))!=NULL ) {
	int val = strtol(test,&end,10);
	if ( *end!='\0' || end==test || val<0 )
	    errs |= pds_badblueshift;
    }

    if ( !CheckStdW(sf->private_dict,"StdHW"))
	errs |= pds_badstdhw;
    if ( !CheckStdW(sf->private_dict,"StdVW"))
	errs |= pds_badstdvw;

    switch ( CheckStemSnap(sf->private_dict,"StemSnapH", "StdHW")) {
      case false:
	errs |= pds_badstemsnaph;
      break;
      case -1:
	errs |= pds_stemsnapnostdh;
      break;
    }
    switch ( CheckStemSnap(sf->private_dict,"StemSnapV", "StdVW")) {
      case false:
	errs |= pds_badstemsnapv;
      break;
      case -1:
	errs |= pds_stemsnapnostdv;
      break;
    }

return( errs );
}

static int SFValidNameList(SplineFont *sf, char *list) {
    char *start, *pt;
    int ch;
    SplineChar *sc;

    for ( start = list ; ; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
return( true );
	for ( pt=start; *pt!=':' && *pt!=' ' && *pt!='\0' ; ++pt );
	ch = *pt;
	if ( ch==' ' || ch=='\0' )
return( -1 );
	if ( sf!=NULL ) {
	    *pt = '\0';
	    sc = SFGetChar(sf,-1,start);
	    *pt = ch;
	    if ( sc==NULL )
return( -1 );
	}
	start = pt;
    }
}

int BPTooFar(BasePoint *bp1, BasePoint *bp2) {
return( bp1->x - bp2->x > 32767 || bp2->x - bp1->x > 32767 ||
	bp1->y - bp2->y > 32767 || bp2->y - bp1->y > 32767 );
}

AnchorClass *SCValidateAnchors(SplineChar *sc) {
    SplineFont *sf = sc->parent;
    AnchorClass *ac;
    AnchorPoint *ap;

    if ( sf==NULL )
return( NULL );
    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;

    for ( ac=sf->anchor; ac!=NULL; ac=ac->next ) {
	ac->ticked = 0;
	if ( ac->subtable ) ac->subtable->ticked = 0;
    }

    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	if ( ap->type==at_basechar || ap->type==at_basemark ) {
	    ac = ap->anchor;
	    ac->ticked = true;
	    if ( ac->subtable ) ac->subtable->ticked = true;
	}
    }

    for ( ac=sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( !ac->ticked && ac->subtable && ac->subtable->ticked )
return( ac );
    }
return( NULL );
}

static int UniMatch(int vs, int uni, SplineChar *sc) {
    struct altuni *alt;

    if ( sc->unicodeenc!=-1 && vs==-1 && uni == sc->unicodeenc )
return( true );
    for ( alt = sc->altuni; alt!=NULL; alt=alt->next )
	if ( alt->vs==vs && alt->unienc==uni )
return( true );

return( false );
}

StemInfo *SCHintOverlapInMask(SplineChar *sc,HintMask *hm) {
    int hi1, hi2, hcnt=0;
    StemInfo *h1, *h2;
    int v;

    for ( v=0; v<2; ++v ) {
	if ( v==0 ) {
	    h1 = sc->hstem;
	    hi1 = 0;
	} else {
	    h1 = sc->vstem;
	    hi1 = hcnt;
	}
	for ( ; h1!=NULL && hi1<HntMax; ++hi1, h1=h1->next ) {
	    if ( hm==NULL || ((*hm)[(hi1>>3)] & (0x80>>(hi1&7))) ) {
		for ( hi2=hi1+1, h2=h1->next; h2!=NULL && hi2<HntMax; ++hi2, h2=h2->next ) {
		    if ( hm==NULL || ((*hm)[(hi2>>3)] & (0x80>>(hi2&7))) ) {
			real start1, end1, start2, end2;
			if ( h1->width>0 ) {
			    start1 = h1->start;
			    end1 = start1+h1->width;
			} else {
			    end1 = h1->start;
			    start1 = end1+h1->width;
			}
			if ( h2->width>0 ) {
			    start2 = h2->start;
			    end2 = start2+h2->width;
			} else {
			    end2 = h2->start;
			    start2 = end2+h2->width;
			}
			if ( end1<start2 || start1>end2 )
			    /* No overlap */;
			else
return( h1 );
		    }
		}
	    }
	}
	hcnt = hi1;
    }
return( NULL );
}

int SCValidate(SplineChar *sc, int layer, int force) {
    SplineSet *ss;
    Spline *s1, *s2, *s, *first;
    SplinePoint *sp;
    RefChar *ref;
    int lastscan= -1;
    int cnt, path_cnt, pt_cnt;
    StemInfo *h;
    SplineSet *base;
    bigreal len2, bound2, x, y;
    extended extrema[4];
    PST *pst;
    struct ttf_table *tab;
    extern int allow_utf8_glyphnames;
    RefChar *r;
    BasePoint lastpt;
    int gid, k;
    SplineFont *cid, *sf;
    SplineChar *othersc;
    struct altuni *alt;

    if ( (sc->layers[layer].validation_state&vs_known) && !force )
  goto end;

    sc->layers[layer].validation_state = 0;

    base = LayerAllSplines(&sc->layers[layer]);

    if ( !allow_utf8_glyphnames ) {
	if ( strlen(sc->name)>31 )
	    sc->layers[layer].validation_state |= vs_badglyphname|vs_known;
	else {
	    char *pt;
	    for ( pt = sc->name; *pt; ++pt ) {
		if (( *pt>='A' && *pt<='Z' ) ||
			(*pt>='a' && *pt<='z' ) ||
			(*pt>='0' && *pt<='9' ) ||
			*pt == '.' || *pt == '_' )
		    /* That's ok */;
		else {
		    sc->layers[layer].validation_state |= vs_badglyphname|vs_known;
	    break;
		}
	    }
	}
    }

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type==pst_substitution &&
		!SCWorthOutputting(SFGetChar(sc->parent,-1,pst->u.subs.variant))) {
	    sc->layers[layer].validation_state |= vs_badglyphname|vs_known;
    break;
	} else if ( pst->type==pst_pair &&
		!SCWorthOutputting(SFGetChar(sc->parent,-1,pst->u.pair.paired))) {
	    sc->layers[layer].validation_state |= vs_badglyphname|vs_known;
    break;
	} else if ( (pst->type==pst_alternate || pst->type==pst_multiple || pst->type==pst_ligature) &&
		!SFValidNameList(sc->parent,pst->u.mult.components)) {
	    sc->layers[layer].validation_state |= vs_badglyphname|vs_known;
    break;
	}
    }
    if ( sc->vert_variants!=NULL && sc->vert_variants->variants != NULL &&
	    !SFValidNameList(sc->parent,sc->vert_variants->variants) )
	sc->layers[layer].validation_state |= vs_badglyphname|vs_known;
    else if ( sc->horiz_variants!=NULL && sc->horiz_variants->variants != NULL &&
	    !SFValidNameList(sc->parent,sc->horiz_variants->variants) )
	sc->layers[layer].validation_state |= vs_badglyphname|vs_known;
    else {
	int i;
	if ( sc->vert_variants!=NULL ) {
	    for ( i=0; i<sc->vert_variants->part_cnt; ++i ) {
		if ( !SCWorthOutputting(SFGetChar(sc->parent,-1,sc->vert_variants->parts[i].component)))
		    sc->layers[layer].validation_state |= vs_badglyphname|vs_known;
	    break;
	    }
	}
	if ( sc->horiz_variants!=NULL ) {
	    for ( i=0; i<sc->horiz_variants->part_cnt; ++i ) {
		if ( !SCWorthOutputting(SFGetChar(sc->parent,-1,sc->horiz_variants->parts[i].component)))
		    sc->layers[layer].validation_state |= vs_badglyphname|vs_known;
	    break;
	    }
	}
    }

    for ( ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	/* TrueType uses single points to move things around so ignore them */
	if ( ss->first->next==NULL )
	    /* Do Nothing */;
	else if ( ss->first->prev==NULL ) {
	    sc->layers[layer].validation_state |= vs_opencontour|vs_known;
    break;
	}
    }

    /* If there's an open contour we can't really tell whether it self-intersects */
    if ( sc->layers[layer].validation_state & vs_opencontour )
	/* sc->layers[layer].validation_state |= vs_selfintersects*/;
    else {
	if ( SplineSetIntersect(base,&s1,&s2) )
	    sc->layers[layer].validation_state |= vs_selfintersects|vs_known;
    }

    /* If there's a self-intersection we are guaranteed that both the self- */
    /*  intersecting contours will be in the wrong direction at some point */
    if ( sc->layers[layer].validation_state & vs_selfintersects )
	/*sc->layers[layer].validation_state |= vs_wrongdirection*/;
    else {
	if ( SplineSetsDetectDir(&base,&lastscan)!=NULL )
	    sc->layers[layer].validation_state |= vs_wrongdirection|vs_known;
    }

    /* Different kind of "wrong direction" */
    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	if ( ref->transform[0]*ref->transform[3]<0 ||
		(ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
	    sc->layers[layer].validation_state |= vs_flippedreferences|vs_known;
    break;
	}
    }

    for ( h=sc->hstem, cnt=0; h!=NULL; h=h->next, ++cnt );
    for ( h=sc->vstem       ; h!=NULL; h=h->next, ++cnt );
    if ( cnt>=96 )
	sc->layers[layer].validation_state |= vs_toomanyhints|vs_known;

    if ( sc->layers[layer].splines!=NULL ) {
	int anyhm=0;
	h=NULL;
	for ( ss=sc->layers[layer].splines; ss!=NULL && h==NULL; ss=ss->next ) {
	    sp = ss->first;
	    do {
		if ( sp->hintmask!=NULL ) {
		    anyhm = true;
		    h = SCHintOverlapInMask(sc,sp->hintmask);
		    if ( h!=NULL )
	    break;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    } while ( sp!=ss->first );
	}
	if ( !anyhm )
	    h = SCHintOverlapInMask(sc,NULL);
	if ( h!=NULL )
	    sc->layers[layer].validation_state |= vs_overlappedhints|vs_known;
    }

    memset(&lastpt,0,sizeof(lastpt));
    for ( ss=sc->layers[layer].splines, pt_cnt=path_cnt=0; ss!=NULL; ss=ss->next, ++path_cnt ) {
	for ( sp=ss->first; ; ) {
	    /* If we're interpolating the point, it won't show up in the truetype */
	    /*  points list and it need not be integral (often it will end in .5) */
	    if ( (!SPInterpolate(sp) && (sp->me.x != rint(sp->me.x) || sp->me.y != rint(sp->me.y))) ||
		    sp->nextcp.x != rint(sp->nextcp.x) || sp->nextcp.y != rint(sp->nextcp.y) ||
		    sp->prevcp.x != rint(sp->prevcp.x) || sp->prevcp.y != rint(sp->prevcp.y))
		sc->layers[layer].validation_state |= vs_nonintegral|vs_known;
	    if ( BPTooFar(&lastpt,&sp->prevcp) ||
		    BPTooFar(&sp->prevcp,&sp->me) ||
		    BPTooFar(&sp->me,&sp->nextcp))
		sc->layers[layer].validation_state |= vs_pointstoofarapart|vs_known;
	    memcpy(&lastpt,&sp->nextcp,sizeof(lastpt));
	    ++pt_cnt;
	    if ( sp->next==NULL )
	break;
	    if ( !sp->next->knownlinear ) {
		if ( sp->next->order2 )
		    ++pt_cnt;
		else
		    pt_cnt += 2;
	    }
	    sp = sp->next->to;
	    if ( sp==ss->first ) {
		memcpy(&lastpt,&sp->me,sizeof(lastpt));
	break;
	    }
	}
    }
    if ( pt_cnt>1500 )
	sc->layers[layer].validation_state |= vs_toomanypoints|vs_known;

    LayerUnAllSplines(&sc->layers[layer]);

    /* Only check the splines in the glyph, not those in refs */
    bound2 = sc->parent->extrema_bound;
    if ( bound2<=0 )
	bound2 = (sc->parent->ascent + sc->parent->descent)/32.0;
    bound2 *= bound2;
    for ( ss=sc->layers[layer].splines, cnt=0; ss!=NULL; ss=ss->next ) {
	first = NULL;
	for ( s=ss->first->next ; s!=NULL && s!=first; s=s->to->next ) {
	    if ( first==NULL )
		first = s;
	    if ( s->acceptableextrema )
	continue;		/* If marked as good, don't check it */
	    /* rough approximation to spline's length */
	    x = (s->to->me.x-s->from->me.x);
	    y = (s->to->me.y-s->from->me.y);
	    len2 = x*x + y*y;
	    /* short splines (serifs) are not required to have points at their extrema */
	    if ( len2>bound2 && Spline2DFindExtrema(s,extrema)>0 ) {
		sc->layers[layer].validation_state |= vs_missingextrema|vs_known;
    goto break_2_loops;
	    }
	}
    }
    break_2_loops:;

    if ( (tab = SFFindTable(sc->parent,CHR('m','a','x','p')))!=NULL && tab->len>=32 ) {
	/* If we have a maxp table then do some truetype checks */
	/* these are only errors for fontlint, we'll fix them up when we */
	/*  generate the font -- but fontlint needs to know this stuff */
	int pt_max = memushort(tab->data,tab->len,3*sizeof(uint16_t));
	int path_max = memushort(tab->data,tab->len,4*sizeof(uint16_t));
	int composit_pt_max = memushort(tab->data,tab->len,5*sizeof(uint16_t));
	int composit_path_max = memushort(tab->data,tab->len,6*sizeof(uint16_t));
	int instr_len_max = memushort(tab->data,tab->len,13*sizeof(uint16_t));
	int num_comp_max = memushort(tab->data,tab->len,14*sizeof(uint16_t));
	int comp_depth_max  = memushort(tab->data,tab->len,15*sizeof(uint16_t));
	int rd, rdtest;

	/* Already figured out two of these */
	if ( sc->layers[layer].splines==NULL ) {
	    if ( pt_cnt>composit_pt_max )
		sc->layers[layer].validation_state |= vs_maxp_toomanycomppoints|vs_known;
	    if ( path_cnt>composit_path_max )
		sc->layers[layer].validation_state |= vs_maxp_toomanycomppaths|vs_known;
	}

	for ( ss=sc->layers[layer].splines, pt_cnt=path_cnt=0; ss!=NULL; ss=ss->next, ++path_cnt ) {
	    for ( sp=ss->first; ; ) {
		++pt_cnt;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	}
	if ( pt_cnt>pt_max )
	    sc->layers[layer].validation_state |= vs_maxp_toomanypoints|vs_known;
	if ( path_cnt>path_max )
	    sc->layers[layer].validation_state |= vs_maxp_toomanypaths|vs_known;

	if ( sc->ttf_instrs_len>instr_len_max )
	    sc->layers[layer].validation_state |= vs_maxp_instrtoolong|vs_known;

	rd = 0;
	for ( r=sc->layers[layer].refs, cnt=0; r!=NULL; r=r->next, ++cnt ) {
	    rdtest = RefDepth(r,layer);
	    if ( rdtest>rd )
		rd = rdtest;
	}
	if ( cnt>num_comp_max )
	    sc->layers[layer].validation_state |= vs_maxp_toomanyrefs|vs_known;
	if ( rd>comp_depth_max )
	    sc->layers[layer].validation_state |= vs_maxp_refstoodeep|vs_known;
    }

    k=0;
    cid = sc->parent;
    if ( cid->cidmaster != NULL )
	cid = cid->cidmaster;
    do {
	sf = cid->subfontcnt==0 ? cid : cid->subfonts[k];
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( (othersc=sf->glyphs[gid])!=NULL ) {
	    if ( othersc==sc )
	continue;
	    if ( strcmp(sc->name,othersc->name)==0 )
		sc->layers[layer].validation_state |= vs_dupname|vs_known;
	    if ( sc->unicodeenc!=-1 && UniMatch(-1,sc->unicodeenc,othersc) )
		sc->layers[layer].validation_state |= vs_dupunicode|vs_known;
	    for ( alt=sc->altuni; alt!=NULL; alt=alt->next )
		if ( UniMatch(alt->vs,alt->unienc,othersc) )
		    sc->layers[layer].validation_state |= vs_dupunicode|vs_known;
	}
	++k;
    } while ( k<cid->subfontcnt );

  end:;
    /* This test is intentionally here and should be done even if the glyph */
    /*  hasn't changed. If the lookup changed it could make the glyph invalid */
    if ( SCValidateAnchors(sc)!=NULL )
	sc->layers[layer].validation_state |= vs_missinganchor;

    sc->layers[layer].validation_state |= vs_known;
    if ( sc->unlink_rm_ovrlp_save_undo )
return( sc->layers[layer].validation_state&~(vs_known|vs_selfintersects) );

return( sc->layers[layer].validation_state&~vs_known );
}

int SFValidate(SplineFont *sf, int layer, int force) {
    int k, gid;
    SplineFont *sub;
    int any = 0;
    SplineChar *sc;
    int cnt=0;

    if ( sf->cidmaster )
	sf = sf->cidmaster;

    if ( !no_windowing_ui ) {
	cnt = 0;
	k = 0;
	do {
	    sub = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	    for ( gid=0; gid<sub->glyphcnt; ++gid ) if ( (sc=sub->glyphs[gid])!=NULL ) {
		if ( force || !(sc->layers[layer].validation_state&vs_known) )
		    ++cnt;
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
	if ( cnt!=0 )
	    ff_progress_start_indicator(10,_("Validating..."),_("Validating..."),0,cnt,1);
    }

    k = 0;
    do {
	sub = sf->subfontcnt==0 ? sf : sf->subfonts[k];
	for ( gid=0; gid<sub->glyphcnt; ++gid ) if ( (sc=sub->glyphs[gid])!=NULL ) {
	    if ( force || !(sc->layers[layer].validation_state&vs_known) ) {
		SCValidate(sc,layer,true);
		if ( !ff_progress_next())
return( -1 );
	    } else if ( SCValidateAnchors(sc)!=NULL )
		sc->layers[layer].validation_state |= vs_missinganchor;

	    if ( sc->unlink_rm_ovrlp_save_undo )
		any |= sc->layers[layer].validation_state&~vs_selfintersects;
	    else
		any |= sc->layers[layer].validation_state;
	}
	++k;
    } while ( k<sf->subfontcnt );
    ff_progress_end_indicator();

    /* a lot of asian ttf files have a bad postscript fontname stored in the */
    /*  name table */
return( any&~vs_known );
}

void SCTickValidationState(SplineChar *sc,int layer) {
    struct splinecharlist *dlist;

    sc->layers[layer].validation_state = vs_unknown;
    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next ) {
	if ( dlist->sc==sc )
	    IError("A glyph may not depend on itself in SCTickValidationState");
	else
	    SCTickValidationState(dlist->sc,layer);
    }
}

int VSMaskFromFormat(SplineFont *sf, int layer, enum fontformat format) {
    if ( format==ff_cid || format==ff_cffcid || format==ff_otfcid || format==ff_otfciddfont )
return( vs_maskcid );
    else if ( format<=ff_cff )
return( vs_maskps );
    else if ( format<=ff_ttfdfont )
return( vs_maskttf );
    else if ( format<=ff_otfdfont )
return( vs_maskps );
    else if ( format==ff_svg )
return( vs_maskttf );
    else if ( format==ff_woff_ttf || format==ff_woff2_ttf )
return( vs_maskttf );
    else if ( format==ff_woff_otf || format==ff_woff2_otf )
return( vs_maskps );
    else
return( sf->subfontcnt!=0 || sf->cidmaster!=NULL ? vs_maskcid :
	sf->layers[layer].order2 ? vs_maskttf : vs_maskps );
}

static SplinePoint *CirclePoint(int which) {
    SplinePoint *sp;
    static struct shapedescrip {
        BasePoint me, prevcp, nextcp; int nocp;
    } ellipse3[] = {
        { { 1, 0 }, { 1, 0.552 }, { 1, -.552 }, false },
        { { 0, -1 }, { 0.552, -1 }, { -0.552, -1 }, false },
        { { -1, 0 }, { -1, -0.552 }, { -1, .552 }, false },
        { { 0, 1 }, { -0.552, 1 }, { 0.552, 1 }, false },
        { { 0, 0 }, { 0, 0 }, { 0, 0 }, 0 }
    };

    sp = SplinePointCreate(ellipse3[which].me.x,ellipse3[which].me.y);
    sp->nextcp = ellipse3[which].nextcp;
    sp->prevcp = ellipse3[which].prevcp;
return( sp );
}

static SplineSet *UnitCircle(int clockwise) {
    SplineSet *spl;
    SplinePoint *sps[5];
    int i;

    spl = chunkalloc(sizeof(SplineSet));
    for ( i=0; i<4; ++i )
	sps[i] = CirclePoint(i&3);
    sps[4] = sps[0];
    for ( i=0; i<4; ++i )
	SplineMake3(sps[i], sps[i+1]);
    spl->first = sps[0]; spl->last = sps[4];
    spl->start_offset = 0;
    if ( !clockwise )
	SplineSetReverse(spl);
return( spl );
}

static int CutCircle(SplineSet *spl,BasePoint *me,int first) {
    Spline *s, *firsts;
    SplinePoint *end;
    extended ts[3];
    int i;
    bigreal best_t = -1;
    Spline *best_s = NULL;
    bigreal best_off = 2;

    firsts = NULL;
    for ( s = spl->first->next ; s!=NULL && s!=firsts; s = s->to->next ) {
	if ( firsts==NULL ) firsts = s;
	CubicSolve(&s->splines[0],me->x,ts);
	for ( i=0; i<3 && ts[i]!=-1; ++i ) {
	    bigreal y = ((s->splines[1].a*ts[i]+s->splines[1].b)*ts[i]+s->splines[1].c)*ts[i]+s->splines[1].d;
	    bigreal off = me->y-y;
	    if ( off<0 ) off = -off;
	    if ( off < best_off ) {
		best_s = s;
		best_t = ts[i];
		best_off = off;
	    }
	}
    }
    if ( best_s==NULL ) {
return(false);
    }

    if ( best_t<.0001 )
	end = best_s->from;
    else if ( best_t>.999 )
	end = best_s->to;
    else
	end = SplineBisect(best_s,best_t);
    if ( first ) {
	spl->first = end;
	spl->last = end;
	spl->start_offset = 0;
    } else {
	spl->last = end;
	s = end->next;
	end->next = NULL;
	while ( s!=NULL ) {
	    end = s->to;
	    SplineFree(s);
	    if ( end==spl->first )
	break;
	    s = end->next;
	    SplinePointFree(end);
	}
    }
return( true );
}

static void PrevSlope(SplinePoint *sp,BasePoint *slope) {
    bigreal len;

    if ( sp->prev==NULL )
	slope->x = slope->y = 0;
    else if ( sp->prev->knownlinear ) {
	slope->x = sp->me.x - sp->prev->from->me.x;
	slope->y = sp->me.y - sp->prev->from->me.y;
    } else if ( !sp->noprevcp ) {
	slope->x = sp->me.x - sp->prevcp.x;
	slope->y = sp->me.x - sp->prevcp.y;
    } else {
	bigreal t = 1.0-1/256.0;
	slope->x = (3*sp->prev->splines[0].a*t+2*sp->prev->splines[0].b)*t+sp->prev->splines[0].c;
	slope->y = (3*sp->prev->splines[1].a*t+2*sp->prev->splines[1].b)*t+sp->prev->splines[1].c;
    }
    len = sqrt(slope->x*slope->x + slope->y*slope->y);
    if ( len==0 )
return;
    slope->x /= len;
    slope->y /= len;
return;
}

static void NextSlope(SplinePoint *sp,BasePoint *slope) {
    bigreal len;

    if ( sp->next==NULL )
	slope->x = slope->y = 0;
    else if ( sp->next->knownlinear ) {
	slope->x = sp->next->to->me.x - sp->me.x;
	slope->y = sp->next->to->me.y - sp->me.y;
    } else if ( !sp->nonextcp ) {
	slope->x = sp->nextcp.x - sp->me.x;
	slope->y = sp->nextcp.y - sp->me.y;
    } else {
	bigreal t = 1/256.0;
	slope->x = (3*sp->next->splines[0].a*t+2*sp->next->splines[0].b)*t+sp->next->splines[0].c;
	slope->y = (3*sp->next->splines[1].a*t+2*sp->next->splines[1].b)*t+sp->next->splines[1].c;
    }
    len = sqrt(slope->x*slope->x + slope->y*slope->y);
    if ( len==0 )
return;
    slope->x /= len;
    slope->y /= len;
return;
}

static int EllipseClockwise(SplinePoint *sp1,SplinePoint *sp2,BasePoint *slope1,
	BasePoint *slope2) {
    /* Build up a temporary shape to see whether the ellipse will be clockwise*/
    /*  or counter */
    SplinePoint *e1, *e2;
    SplineSet *ss;
    int ret;
    bigreal len;

    e1 = SplinePointCreate(sp1->me.x,sp1->me.y);
    e2 = SplinePointCreate(sp2->me.x,sp2->me.y);
    SplineMake3(e2,e1);
    len = sqrt((sp1->me.x-sp2->me.x) * (sp1->me.x-sp2->me.x)  +
	  (sp1->me.y-sp2->me.y) * (sp1->me.y-sp2->me.y));
    e1->nextcp.x = e1->me.x + len*slope1->x;
    e1->nextcp.y = e1->me.y + len*slope1->y;
    e2->prevcp.x = e2->me.x - len*slope2->x;
    e2->prevcp.y = e2->me.y - len*slope2->y;
    SplineMake3(e1,e2);
    ss = chunkalloc(sizeof(SplineSet));
    ss->first = ss->last = e1;
    ss->start_offset = 0;
    ret = SplinePointListIsClockwise(ss);
    SplinePointListFree(ss);
return( ret );
}

static int BuildEllipse(int clockwise,bigreal r1,bigreal r2, bigreal theta,
	BasePoint *center,SplinePoint *sp1,SplinePoint *sp2,CharViewBase *cv,
	int changed, int order2, int ellipse_to_back) {
    SplineSet *spl, *ss=NULL;
    real trans[6];
    bigreal c,s;

    spl = UnitCircle(clockwise);
    memset(trans,0,sizeof(trans));
    trans[0] = r1; trans[3] = r2;
    SplinePointListTransform(spl,trans,tpt_AllPoints);
    c = cos(theta); s = sin(theta);
    trans[0] = trans[3] = c;
    trans[1] = s; trans[2] = -s;
    trans[4] = center->x; trans[5] = center->y;
    SplinePointListTransform(spl,trans,tpt_AllPoints);
    if ( ellipse_to_back && CVLayer(cv)!=ly_back )
	/* Originally this was debugging code... but hey, it's kind of neat */
	/*  and may prove useful if we need to do more debugging. Invoked by */
	/*  holding down the <Alt> modifier when selecting the menu item */
	ss = SplinePointListCopy(spl);
    if ( !CutCircle(spl,&sp1->me,true) || !CutCircle(spl,&sp2->me,false) ) {
	SplinePointListFree(spl);
	SplinePointListFree(ss);
return( false );
    }
    if ( ellipse_to_back && ss!=NULL ) {
	SCPreserveBackground(cv->sc);
	if ( cv->sc->layers[ly_back].order2 )
	    ss = SplineSetsConvertOrder(ss,true);
	ss->next = cv->sc->layers[ly_back].splines;
	cv->sc->layers[ly_back].splines = ss;
    }
    if ( order2 )
	spl = SplineSetsConvertOrder(spl,true);
    if ( !changed )
	CVPreserveState(cv);
    if ( sp1->next!=NULL ) {
	chunkfree(sp1->next,sizeof(Spline));
	sp1->next = sp2->prev = NULL;
    }
    sp1->nextcp = spl->first->nextcp;
    sp1->nonextcp = spl->first->nonextcp;
    sp1->next = spl->first->next;
    sp1->next->from = sp1;
    sp2->prevcp.x = sp2->me.x + (spl->last->prevcp.x-spl->last->me.x);
    sp2->prevcp.y = sp2->me.y + (spl->last->prevcp.y-spl->last->me.y);
    sp2->noprevcp = spl->last->noprevcp;
    sp2->prev = spl->last->prev;
    sp2->prev->to = sp2;
    SplineRefigure(sp1->next); SplineRefigure(sp2->prev);
    SplinePointFree(spl->first);
    SplinePointFree(spl->last);
    spl->first = spl->last = NULL;
    spl->start_offset = 0;
    SplinePointListFree(spl);
return( true );
}

static int MakeEllipseWithAxis(CharViewBase *cv,SplinePoint *sp1,SplinePoint *sp2,int order2,
	int changed, int ellipse_to_back ) {
    BasePoint center;
    bigreal r1,r2, len, dot, theta, c, s, factor, denom;
    BasePoint slope1, slope2, offset, roffset, rslope, temp;
    int clockwise;

    PrevSlope(sp1,&slope1);
    NextSlope(sp2,&slope2);
    if ( slope1.x==0 && slope1.y==0 ) {
	if ( slope2.x==0 && slope2.y==0 ) {
	    /* No direction info. Draw a semicircle with center halfway between*/
	    slope1.y =   sp2->me.x - sp1->me.x ;
	    slope1.x = -(sp2->me.y - sp1->me.y);
	    len = sqrt(slope1.x*slope1.x + slope1.y*slope1.y);
	    slope1.x /= len; slope1.y /= len;
	    slope2.x = -slope1.x;
	    slope2.y = -slope1.y;
	} else {
	    slope1.x = -slope2.y;
	    slope1.y =  slope2.x;
	}
    } else if ( slope2.x==0 && slope2.y==0 ) {
	slope2.x =  slope1.y;
	slope2.y = -slope1.x;
    }
    clockwise = EllipseClockwise(sp1,sp2,&slope1,&slope2);
    dot = slope1.y*slope2.x - slope1.x*slope2.y;
    theta = atan2(-slope1.x,slope1.y);
    if ( !isfinite(theta))
return( false );
    c = cos(theta); s = sin(theta);
    if ( RealNear(dot,0) ) {
	/* The slopes are parallel. Most cases have no solution, but */
	/*  one case gives us a semicircle centered between the two points*/
	if ( slope1.x*slope2.x + slope1.y*slope2.y > 0 )
return( false );		/* Point in different directions */
	temp.x = sp2->me.x - sp1->me.x;
	temp.y = sp2->me.y - sp1->me.y;
	if ( !RealNear(slope1.x*temp.x - slope1.y*temp.y,0) )
return( false );
	center.x = sp1->me.x + temp.x/2;
	center.y = sp1->me.y + temp.y/2;
	len = sqrt(temp.x*temp.x + temp.y*temp.y);
	r1 = r2 = len/2;
	factor = 1;
    } else {
	/* The data we have been given do not specify an unique ellipse*/
	/*  in fact there are an infinite number of them, (I think)    */
	/*  but once we fix the orientation of the axes then there is  */
	/*  only one. So let's say that one axis is normal to sp1,     */
	/* Now in a circle... if we draw a line through the center to  */
	/*  the edge, and we represent the slope of the circle at that */
	/*  point as slope2.y/slope2.x, and the height of the point as */
	/*  offset.y and the difference between the x coord of the pt  */
	/*  and the radius of the circle, then we have two identities  */
	/*    (radius-offset.x)^2 + offset.y^2 = radius^2              */
	/*    => radius = (offset.x^2+offset.y^2)/(2*offset.x)         */
	/*    slope.y/slope.x = (radius-offset.x)/offset.y             */
	/*  Now in an ellipse we must multiply each y value by a factor*/
	/*  Solving for both equations for radius, this gives us an    */
	/*  equation for factor:                                       */
	/*  f = sqrt( dx^2*sx / (sx*dy^2 - 2*sy*dy*dx ))               */
	offset.x = sp2->me.x - sp1->me.x;
	offset.y = sp2->me.y - sp1->me.y;
	roffset.x =  offset.x*c + offset.y*s;
	roffset.y = -offset.x*s + offset.y*c;
	if ( RealNear(roffset.x,0) || RealNear(roffset.y,0))
return( false );
	rslope.x =  slope2.x*c + slope2.y*s;
	rslope.y = -slope2.x*s + slope2.y*c;
	if ( roffset.x<0 ) roffset.x = -roffset.x;
	if ( roffset.y<0 ) roffset.y = -roffset.y;
	if ( rslope.x<0 ) rslope.x = -rslope.x;
	if ( rslope.y<0 ) rslope.y = -rslope.y;
	denom = roffset.y*(rslope.x*roffset.y - 2*roffset.x*rslope.y);
	if ( RealNear(denom,0))
return( false );
	factor = rslope.x*roffset.x*roffset.x/denom;
	if ( factor<0 )
return( false );
	factor = sqrt( factor );
	r1 = (roffset.x*roffset.x+roffset.y*roffset.y*factor*factor)/(2*roffset.x);
	r2 = r1/factor;
	/* And the center is normal to sp1 and r1 away from it */
	if ( clockwise ) {
	    center.x = sp1->me.x + slope1.y*r1;
	    center.y = sp1->me.y - slope1.x*r1;
	} else {
	    center.x = sp1->me.x - slope1.y*r1;
	    center.y = sp1->me.y + slope1.x*r1;
	}
    }
return( BuildEllipse(clockwise,r1,r1/factor,theta,&center,sp1,sp2,cv,changed,
	order2, ellipse_to_back));
}

#define ITER	3
static int EllipseSomewhere(CharViewBase *cv,SplinePoint *sp1,SplinePoint *sp2,
	int order2, int changed, int ellipse_to_back ) {
    BasePoint center;
    bigreal len, dot, rc, rs;
    BasePoint slope1, slope2, temp;
    struct transstate {
	BasePoint me1, me2;
	BasePoint slope1, slope2;
    } centered, rot;
    bigreal bestrot=9999, bestrtest = 1e50, e1sq, e2sq, r1, r2, rtest;
    bigreal low, high, offset=FF_PI/128., rotation;
    int i;
    int clockwise;
    /* First figure out a center. There will be one: */
    /*  Take a line through sp1 but parallel to slope2 */
    /*  Take a line through sp2 but parallel to slope1 */
    /* The intersection of these two lines gives us a center */
    /* First rotate everything by the rotation angle (around the origin -- it */
    /*  will do to cancel out the rotation */

    PrevSlope(sp1,&slope1);
    NextSlope(sp2,&slope2);
    if ( slope1.x==0 && slope1.y==0 ) {
	if ( slope2.x==0 && slope2.y==0 ) {
	    /* No direction info. Draw a semicircle with center halfway between*/
	    slope1.y =   sp2->me.x - sp1->me.x ;
	    slope1.x = -(sp2->me.y - sp1->me.y);
	    len = sqrt(slope1.x*slope1.x + slope1.y*slope1.y);
	    slope1.x /= len; slope1.y /= len;
	    slope2.x = -slope1.x;
	    slope2.y = -slope1.y;
	} else {
	    slope1.x = -slope2.y;
	    slope1.y =  slope2.x;
	}
    } else if ( slope2.x==0 && slope2.y==0 ) {
	slope2.x =  slope1.y;
	slope2.y = -slope1.x;
    }
    clockwise = EllipseClockwise(sp1,sp2,&slope1,&slope2);
    dot = slope1.y*slope2.x - slope1.x*slope2.y;
    if ( RealNear(dot,0)) {
	/* Essentially parallel lines, already handled the only case we can */
return( false );
    }
    center.x = (slope1.x*slope2.x*(sp1->me.y-sp2->me.y) + slope2.x*slope1.y*sp2->me.x - slope1.x*slope2.y*sp1->me.x)/dot;
    if ( slope2.x!=0 )
	center.y = sp1->me.y + slope2.y*(center.x-sp1->me.x)/slope2.x;
    else	/* Can't both be 0, else lines would be parallel */
	center.y = sp2->me.y + slope1.y*(center.x-sp2->me.x)/slope1.x;
    centered.me1.x = sp1->me.x - center.x; centered.me1.y = sp1->me.y - center.y;
    centered.me2.x = sp2->me.x - center.x; centered.me2.y = sp2->me.y - center.y;

    /* now guess the angle of rotation and find one which works */
    /* doubtless there is a better way to do this, but I get lost in */
    /*  algebraic grunge */
    r1 = r2 = 1;
    for ( i=0; i<ITER; ++i ) {
	if ( i==0 ) {
	    low = 0; high = FF_PI; offset = FF_PI/1024;
	} else {
	    low = bestrot-offset; high = bestrot+offset; offset /= 64.;
	}
	bestrot = 9999; bestrtest = 1e50;
	for ( rotation = low; rotation<=high; rotation += offset ) {
	    rc = cos(rotation); rs = sin(rotation);
	    temp.x =  slope1.x*rc + slope1.y*rs;
	    temp.y = -slope1.x*rs + slope1.y*rc;
	    rot.slope1 = temp;
	    temp.x =  slope2.x*rc + slope2.y*rs;
	    temp.y = -slope2.x*rs + slope2.y*rc;
	    rot.slope2 = temp;
	    rot.me1.x =  centered.me1.x*rc + centered.me1.y*rs;
	    rot.me1.y = -centered.me1.x*rs + centered.me1.y*rc;
	    rot.me2.x =  centered.me2.x*rc + centered.me2.y*rs;
	    rot.me2.y = -centered.me2.x*rs + centered.me2.y*rc;

	    /* Now we know that in a circle: */
	    /* (x-cx)/(y-cy) = -sy/sx,  or  sx*(x-cx) = -sy*(y-cy)     */
	    /* on an ellipse that becomes  sx*(x-cx) = -e^2*sy*(y-cy)  */
	    /*  e^2 = -sx*(x-cx)/(sy*(y-cy))                           */
	    if ( rot.slope1.y==0 || rot.slope2.y==0 || rot.me1.y==0 || rot.me2.y==0 ||
		    rot.slope1.x==0 || rot.slope2.x==0 || rot.me1.x==0 || rot.me2.x==0 )
	continue;
	    e1sq = -rot.slope1.x*rot.me1.x/(rot.slope1.y*rot.me1.y);
	    e2sq = -rot.slope2.x*rot.me2.x/(rot.slope2.y*rot.me2.y);
	    if ( e1sq<=0 || e2sq<=0 )
	continue;
	    e1sq = (e1sq+e2sq)/2;
	    r1 = sqrt(rot.me1.x*rot.me1.x + e1sq*rot.me1.y*rot.me1.y);
	    rtest = sqrt(rot.me2.x*rot.me2.x + e1sq*rot.me2.y*rot.me2.y);
	    if ( (rtest -= r1)<0 ) rtest = -rtest;
	    if ( rtest<bestrtest ) {
		bestrtest = rtest;
		bestrot = rotation;
		r2 = r1/sqrt(e1sq);
		/* Since we've got two points with tangents we know:       */
		/* -s1x*(x1-cx)/(s1y*(y1-cy)) = -s2x*(x2-cx)/(s2y*(y2-cy)) */
		/* s1x*(x1-cx)*s2y*(y2-cy) = s2x*(x2-cx)*s1y*(y1-cy)       */
		/* s1x*s2y*(x1-cx)*y2 - s2x*s1y*(x2-cx)*y1 = (s1x*s2y*(x1-cx)-s2x*s1y*(x2-cx))*cy */
		/*      s1x*s2y*(x1-cx)*y2 - s2x*s1y*(x2-cx)*y1            */
		/* cy = ---------------------------------------            */
		/*          s1x*s2y*(x1-cx)-s2x*s1y*(x2-cx)                */
		/* Since the points lie on the curve we know               */
		/*  (x-cx)^2 + e^2*(y-cy)^2 = r^2                          */
		/* Removing r^2....                                        */
		/*  (x1-cx)^2 + e^2*(y1-cy)^2 = (x2-cx)^2 + e^2*(y2-cy)^2  */
		/*  e^2 * [(y1-cy)^2-(y2-cy)^2] = (x2-cx)^2 - (x1-cx)^2    */
		/*        (x2-cx)^2 - (x1-cx)^2                            */
		/*  e^2 = ---------------------                            */
		/*        (y1-cy)^2 - (y2-cy)^2                            */
	    }
	}
	if ( bestrot>9990 )
return( false );
    }
return( BuildEllipse(clockwise,r1,r2,bestrot,&center,sp1,sp2,cv,changed,
	order2, ellipse_to_back));
}

static int MakeShape(CharViewBase *cv,SplinePointList *spl1,SplinePointList *spl2,
	SplinePoint *sp1,SplinePoint *sp2,int order2, int changed, int do_arc,
	int ellipse_to_back ) {
    if ( !do_arc || ( sp1->me.x==sp2->me.x && sp1->me.y==sp2->me.y )) {
	if ( !changed )
	    CVPreserveState(cv);
	sp1->nextcp = sp1->me;
	sp2->prevcp = sp2->me;
	if ( sp1->next==NULL )
	    SplineMake(sp1,sp2,order2);
	else
	    SplineRefigure(sp1->next);
return( true );
    } else {
	/* There are either an infinite number of elliptical solutions or none*/
	/* First search for a solution where one of the points lies on one of */
	/*  the axes of the ellipse. (If there is a circular solution this    */
	/*  will find it as the above statement is true of all points on a    */
	/*  circle). If that fails then try a more general search which will  */
	/*  find something, but may not find an intuitively expected soln.    */
	if ( MakeEllipseWithAxis(cv,sp1,sp2,order2,changed,ellipse_to_back))
return( true );
	/* OK, sp1 wasn't on an axis. How about sp2? */
	SplineSetReverse(spl1);
	if ( spl1!=spl2 )
	    SplineSetReverse(spl2);
	if ( MakeEllipseWithAxis(cv,sp2,sp1,order2,changed,ellipse_to_back))
return( -1 );
	SplineSetReverse(spl1);
	if ( spl1!=spl2 )
	    SplineSetReverse(spl2);
	/* OK, neither was on an axis. Ellipse is rotated by some odd amount */
	if ( EllipseSomewhere(cv,sp1,sp2,order2,changed,ellipse_to_back) )
return( true );

return( false );
    }
}

void _CVMenuMakeLine(CharViewBase *cv,int do_arc,int ellipse_to_back) {
    SplinePointList *spl, *spl1=NULL, *spl2=NULL;
    SplinePoint *sp, *sp1=NULL, *sp2=NULL;
    int changed = false;
    int layer;

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	for ( sp=spl->first; ; ) {
	    if  ( sp->selected ) {
		if ( sp1==NULL ) { sp1 = sp; spl1 = spl; }
		else if ( sp2==NULL ) { sp2 = sp; spl2 = spl; }
		else {
		    sp1 = (SplinePoint *) -1;
	break;
		}
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( sp1 == (SplinePoint *) -1 )
    break;
    }
    if ( sp1!=(SplinePoint *) -1 && sp2!=NULL &&
	    (( sp1->prev==NULL || sp1->next==NULL ) && ( sp2->prev==NULL || sp2->next==NULL ) &&
		!(sp1->next!=NULL && sp1->next->to==sp2) &&
		!(sp1->prev!=NULL && sp1->prev->from==sp2) )) {
	layer = CVLayer(cv);
	CVPreserveState(cv);
	if ( sp1->next!=NULL ) {
	    sp = sp1; sp1 = sp2; sp2 = sp;
	    spl = spl1; spl1 = spl2; spl2 = spl;
	}
	if ( spl1==spl2 ) {
	    /* case of two connected points is handled below */
	    /* This case joins the endpoints of an open-contour */
	    if ( MakeShape(cv,spl1,spl2,sp1,sp2,cv->sc->layers[layer].order2,changed,do_arc,ellipse_to_back)) {
		changed = true;
		spl1->last = spl1->first;
	    }
	} else {
	    if ( sp1->next!=NULL )
		SplineSetReverse(spl1);
	    if ( sp2->prev!=NULL )
		SplineSetReverse(spl2);
	    switch ( MakeShape(cv,spl1,spl2,sp1,sp2,cv->sc->layers[layer].order2,changed,do_arc,ellipse_to_back) ) {
	      case 1:
		spl1->last = spl2->last;
		for ( spl=cv->layerheads[cv->drawmode]->splines;
			spl!=NULL && spl->next!=spl2; spl = spl->next );
		if ( spl!=NULL )
		    spl->next = spl2->next;
		else
		    cv->layerheads[cv->drawmode]->splines = spl2->next;
		chunkfree(spl2,sizeof(*spl2));
		changed = true;
	      break;
	      case -1:
		/* we reversed spl1 and spl2 to get a good match */
		spl2->last = spl1->last;
		SplineSetReverse(spl2);
		for ( spl=cv->layerheads[cv->drawmode]->splines;
			spl!=NULL && spl->next!=spl1; spl = spl->next );
		if ( spl!=NULL )
		    spl->next = spl1->next;
		else
		    cv->layerheads[cv->drawmode]->splines = spl1->next;
		chunkfree(spl1,sizeof(*spl1));
		changed = true;
	      break;
	    }
	}
    } else for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	for ( sp=spl->first; ; ) {
	    if ( sp->selected ) {
		if ( sp->next!=NULL && sp->next->to->selected ) {
		    if ( MakeShape(cv,spl,spl,sp,sp->next->to,sp->next->order2,changed,do_arc,ellipse_to_back))
			changed = true;
		    if ( !changed ) {
			CVPreserveState(cv);
			changed = true;
		    }
		    if (!do_arc) {
			sp->nextcp = sp->me;
			sp->next->to->prevcp = sp->next->to->me;
			sp->next->to->noprevcp = true;
		    }
		    SplineRefigure(sp->next);
		}
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }

    if ( changed )
	CVCharChangedUpdate(cv);
}

void SCClearInstrsOrMark(SplineChar *sc, int layer, int complain) {
    uint8_t *instrs = sc->ttf_instrs==NULL && sc->parent->mm!=NULL && sc->parent->mm->apple ?
		sc->parent->mm->normal->glyphs[sc->orig_pos]->ttf_instrs : sc->ttf_instrs;
    struct splinecharlist *dep;
    SplineSet *ss;
    SplinePoint *sp;
    AnchorPoint *ap;
    int had_ap, had_dep, had_instrs;

    had_ap = had_dep = had_instrs = 0;
    if ( instrs!=NULL ) {
	if ( clear_tt_instructions_when_needed ) {
	    free(sc->ttf_instrs); sc->ttf_instrs = NULL;
	    sc->ttf_instrs_len = 0;
	    SCMarkInstrDlgAsChanged(sc);
	    had_instrs = 1;
	} else {
	    sc->instructions_out_of_date = true;
	    had_instrs = 2;
	}
    }
    for ( dep=sc->dependents; dep!=NULL; dep=dep->next ) {
	RefChar *ref;
	if ( dep->sc->ttf_instrs_len!=0 ) {
	    if ( clear_tt_instructions_when_needed ) {
		free(dep->sc->ttf_instrs); dep->sc->ttf_instrs = NULL;
		dep->sc->ttf_instrs_len = 0;
		SCMarkInstrDlgAsChanged(dep->sc);
		had_instrs = 1;
	    } else {
		dep->sc->instructions_out_of_date = true;
		had_instrs = 2;
	    }
	}
	for ( ref=dep->sc->layers[layer].refs; ref!=NULL && ref->sc!=sc; ref=ref->next );
	for ( ; ref!=NULL ; ref=ref->next ) {
	    if ( ref->point_match ) {
		ref->point_match_out_of_date = true;
		had_dep = true;
	    }
	}
    }
    SCNumberPoints(sc,layer);
    for ( ap=sc->anchor ; ap!=NULL; ap=ap->next ) {
	if ( ap->has_ttf_pt ) {
	    had_ap = true;
	    ap->has_ttf_pt = false;
	    for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
		for ( sp=ss->first; ; ) {
		    if ( sp->me.x==ap->me.x && sp->me.y==ap->me.y && sp->ttfindex!=0xffff ) {
			ap->has_ttf_pt = true;
			ap->ttf_pt_index = sp->ttfindex;
	    goto found;
		    } else if ( sp->nextcp.x==ap->me.x && sp->nextcp.y==ap->me.y && sp->nextcpindex!=0xffff ) {
			ap->has_ttf_pt = true;
			ap->ttf_pt_index = sp->nextcpindex;
	    goto found;
		    }
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==ss->first )
		break;
		}
	    }
	    found: ;
	}
    }
    if ( !complain || no_windowing_ui )
	/* If we're in a script it's annoying (and pointless) to get this message */;
    else if ( sc->complained_about_ptnums )
	/* It's annoying to get the same message over and over again as you edit a glyph */;
    else if ( had_ap || had_dep || had_instrs ) {
	ff_post_notice(_("You changed the point numbering"),
		_("You have just changed the point numbering of glyph %s.%s%s%s"),
			sc->name,
			had_instrs==0 ? "" :
			had_instrs==1 ? _(" Instructions in this glyph (or one that refers to it) have been lost.") :
			                _(" Instructions in this glyph (or one that refers to it) are now out of date."),
			had_dep ? _(" At least one reference to this glyph used point matching. That match is now out of date.")
				: "",
			had_ap ? _(" At least one anchor point used point matching. It may be out of date now.")
				: "" );
	sc->complained_about_ptnums = true;
	if ( had_instrs==2 )
	    FVRefreshAll( sc->parent );
    }
}

void PatternSCBounds(SplineChar *sc, DBounds *b) {
    if ( sc==NULL )
	memset(b,0,sizeof(DBounds));
    else if ( sc->tile_margin!=0 || (sc->tile_bounds.minx==0 && sc->tile_bounds.maxx==0) ) {
	SplineCharFindBounds(sc,b);
	b->minx -= sc->tile_margin; b->miny -= sc->tile_margin;
	b->maxx += sc->tile_margin; b->maxy += sc->tile_margin;
    } else
	*b = sc->tile_bounds;
    if ( b->minx>=b->maxx )
	b->maxx = b->minx+1;
    if ( b->miny>=b->maxy )
	b->maxy = b->miny+1;
}

static void SCUpdateNothing(SplineChar *sc) {
}

static void SCHintsChng(SplineChar *sc) {
    sc->changedsincelasthinted = false;
    if ( !sc->changed ) {
	sc->changed = true;
	sc->parent->changed = true;
    }
}

void instrcheck(SplineChar *sc,int layer) {
    uint8_t *instrs = sc->ttf_instrs==NULL && sc->parent->mm!=NULL && sc->parent->mm->apple ?
		sc->parent->mm->normal->glyphs[sc->orig_pos]->ttf_instrs : sc->ttf_instrs;

    if ( !sc->layers[layer].order2 || sc->layers[layer].background )
return;

    if ( sc->instructions_out_of_date && no_windowing_ui && sc->anchor==NULL )
return;
    if ( instrs==NULL && sc->dependents==NULL && no_windowing_ui && sc->anchor==NULL )
return;
    /* If the points are no longer in order then the instructions are not valid */
    /*  (because they'll refer to the wrong points) and should be removed */
    /* Except that annoys users who don't expect it */
    if ( !SCPointsNumberedProperly(sc,layer)) {
	SCClearInstrsOrMark(sc,layer,true);
    }
}

void TTFPointMatches(SplineChar *sc,int layer,int top) {
    AnchorPoint *ap;
    BasePoint here, there;
    struct splinecharlist *deps;
    RefChar *ref;

    if ( !sc->layers[layer].order2 || sc->layers[layer].background )
return;
    for ( ap=sc->anchor ; ap!=NULL; ap=ap->next ) {
	if ( ap->has_ttf_pt )
	    if ( ttfFindPointInSC(sc,layer,ap->ttf_pt_index,&ap->me,NULL)!=-1 )
		ap->has_ttf_pt = false;
    }
    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
	if ( ref->point_match ) {
	    if ( ttfFindPointInSC(sc,layer,ref->match_pt_base,&there,ref)==-1 &&
		    ttfFindPointInSC(ref->sc,layer,ref->match_pt_ref,&here,NULL)==-1 ) {
		if ( ref->transform[4]!=there.x-here.x ||
			ref->transform[5]!=there.y-here.y ) {
		    ref->transform[4] = there.x-here.x;
		    ref->transform[5] = there.y-here.y;
		    SCReinstanciateRefChar(sc,ref,layer);
		    if ( !top )
			_SCCharChangedUpdate(sc,layer,true);
		}
	    } else
		ref->point_match = false;		/* one of the points no longer exists */
	}
    }
    for ( deps = sc->dependents; deps!=NULL; deps = deps->next )
	TTFPointMatches(deps->sc,layer,false);
}

static void _SCChngNoUpdate(SplineChar *sc,int layer,int changed) {
    SplineFont *sf = sc->parent;

    if ( layer>=sc->layer_cnt ) {
	IError( "Bad layer in _SCChngNoUpdate");
	layer = ly_fore;
    }
    if ( layer>=0 && !sc->layers[layer].background )
	TTFPointMatches(sc,layer,true);
    if ( changed!=-1 ) {
	sc->changed_since_autosave = true;
	SFSetModTime(sf);
	if ( (sc->changed==0) != (changed==0) ) {
	    sc->changed = (changed!=0);
	    if ( changed && (sc->layers[ly_fore].splines!=NULL || sc->layers[ly_fore].refs!=NULL))
		sc->parent->onlybitmaps = false;
	}
	if ( changed && layer>=0 && !sc->layers[layer].background )
	    instrcheck(sc,layer);
	sc->changedsincelasthinted = true;
	sc->changed_since_search = true;
	sf->changed = true;
	sf->changed_since_autosave = true;
	sf->changed_since_xuidchanged = true;
	if ( layer>=0 )
	    SCTickValidationState(sc,layer);
    }
}

static void SCChngNoUpdate(SplineChar *sc,int layer) {
    _SCChngNoUpdate(sc,layer,true);
}

static void SCB_MoreLayers(SplineChar *sc,Layer *old) {
}

static struct sc_interface noui_sc = {
    SCUpdateNothing,
    SCUpdateNothing,
    SCHintsChng,
    SCChngNoUpdate,
    _SCChngNoUpdate,
    SCUpdateNothing,
    SCUpdateNothing,
    SCB_MoreLayers
};

struct sc_interface *sc_interface = &noui_sc;

void FF_SetSCInterface(struct sc_interface *sci) {
    sc_interface = sci;
}

static void CVChngNoUpdate(CharViewBase *cv) {
    _SCChngNoUpdate(cv->sc,CVLayer(cv),true);
}

static void _CVChngNoUpdate(CharViewBase *cv,int changed) {
    _SCChngNoUpdate(cv->sc,CVLayer(cv),changed);
}

static void CVGlphRenameFixup(SplineFont *sf, const char *oldname, const char *newname) {
}

static void CV__LayerPaletteCheck(SplineFont *sf) {
}

static struct cv_interface noui_cv = {
    CVChngNoUpdate,
    _CVChngNoUpdate,
    CVGlphRenameFixup,
    CV__LayerPaletteCheck
};

struct cv_interface *cv_interface = &noui_cv;

void FF_SetCVInterface(struct cv_interface *cvi) {
    cv_interface = cvi;
}


void SCRemoveKern(SplineChar* sc) {
    if ( sc->kerns!=NULL ) {
	KernPairsFree(sc->kerns);
	sc->kerns = NULL;
	sc->parent->changed = true;
	if( sc->parent->fv->cidmaster!=NULL )
	    sc->parent->fv->cidmaster->changed = true;
    }
}

void SCRemoveVKern(SplineChar* sc) {
    if ( sc->vkerns!=NULL ) {
	KernPairsFree(sc->vkerns);
	sc->vkerns = NULL;
	sc->parent->changed = true;
	if( sc->parent->fv->cidmaster!=NULL )
	    sc->parent->fv->cidmaster->changed = true;
    }
}

const char* SCNameCheck(const unichar_t *name, bool *p_questionable) {
    bool bad = false, questionable = false;
    extern int allow_utf8_glyphnames;

    if (p_questionable) *p_questionable = questionable;

    if ( uc_strcmp(name,".notdef")==0 )		/* This name is a special case and doesn't follow conventions */
        return NULL;
    if ( u_strlen(name)>31 ) {
        return _("Glyph names are limited to 31 characters");
    } else if ( *name=='\0' ) {
        return _("Bad Name");
    } else if ( isdigit(*name) || *name=='.' ) {
        return _("A glyph name may not start with a digit nor a full stop (period)");
    }

    while ( *name ) {
        if ( *name<=' ' || (!allow_utf8_glyphnames && *name>=0x7f) ||
                *name=='(' || *name=='[' || *name=='{' || *name=='<' ||
                *name==')' || *name==']' || *name=='}' || *name=='>' ||
                *name=='%' || *name=='/' )
            bad=true;
        else if ( !isalnum(*name) && *name!='.' && *name!='_' )
            questionable = true;
        ++name;
    }
    if ( bad ) {
        return _("A glyph name must be ASCII, without spaces and may not contain the characters \"([{<>}])/%%\", and should contain only alphanumerics, periods and underscores");
    } else if ( questionable ) {
        if (p_questionable == NULL) {
            /* The caller doesn't care, just accept the name. */
            return NULL;
	} else {
            *p_questionable = questionable;
            return _("A glyph name should contain only alphanumerics, periods and underscores\nDo you want to use this name in spite of that?");
        }
    }
    return NULL;
}

const char* SCGetName(const SplineChar* sc) { return sc->name; }

void SCGetEncoding(const SplineChar* sc, int* p_unicodeenc, int* p_ttf_glyph) {
    if (p_unicodeenc) *p_unicodeenc = sc->unicodeenc;
    if (p_ttf_glyph) *p_ttf_glyph = sc->ttf_glyph;
}
