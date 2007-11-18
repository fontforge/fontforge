/* Copyright (C) 2000-2007 by George Williams */
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
#include <math.h>
#include <locale.h>
#ifndef FONTFORGE_CONFIG_GTK
# include <ustring.h>
# include <utype.h>
# include <gresource.h>
#endif
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif

void SCClearRounds(SplineChar *sc) {
    SplineSet *ss;
    SplinePoint *sp;

    for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
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

RefChar *HasUseMyMetrics(SplineChar *sc) {
    RefChar *r;

    for ( r=sc->layers[ly_fore].refs; r!=NULL; r=r->next )
	if ( r->use_my_metrics )
return( r );

return( NULL );
}

/* if they changed the width, then change the width on all bitmap chars of */
/*  ours, and if we are a letter, then change the width on all chars linked */
/*  to us which had the same width that we used to have (so if we change the */
/*  width of A, we'll also change that of À and Ä and ... */
void SCSynchronizeWidth(SplineChar *sc,real newwidth, real oldwidth, FontView *flagfv) {
    BDFFont *bdf;
    struct splinecharlist *dlist;
    FontView *fv = sc->parent->fv;
    RefChar *r = HasUseMyMetrics(sc);
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

    isprobablybase = true;
    if ( sc->unicodeenc==-1 || sc->unicodeenc>=0x10000 ||
	    !isalpha(sc->unicodeenc) || iscombining(sc->unicodeenc))
	isprobablybase = false;

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next ) {
	RefChar *metrics = HasUseMyMetrics(dlist->sc);
	if ( metrics!=NULL && metrics->sc!=sc )
    continue;
	else if ( metrics==NULL && !isprobablybase )
    continue;
	if ( dlist->sc->width==oldwidth &&
		(flagfv==NULL || !flagfv->selected[flagfv->map->backmap[dlist->sc->orig_pos]])) {
	    SCSynchronizeWidth(dlist->sc,newwidth,oldwidth,fv);
	    if ( !dlist->sc->changed ) {
		dlist->sc->changed = true;
		if ( fv!=NULL )
		    FVToggleCharChanged(dlist->sc);
	    }
	    SCUpdateAll(dlist->sc);
	}
    }
}

/* If they change the left bearing of a character, then in all chars */
/*  that depend on it should be adjusted too. */
/* Also all vstem hints */
/* I deliberately don't set undoes in the dependants. The change is not */
/*  in them, after all */
void SCSynchronizeLBearing(SplineChar *sc,real off) {
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
	d->leftedgetop.x += off;
	d->rightedgetop.x += off;
	d->leftedgebottom.x += off;
	d->rightedgebottom.x += off;
    }

    if ( !adjustlbearing )
return;

    isprobablybase = true;
    if ( sc->unicodeenc==-1 || sc->unicodeenc>=0x10000 ||
	    !isalpha(sc->unicodeenc) || iscombining(sc->unicodeenc))
	isprobablybase = false;

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next ) {
	RefChar *metrics = HasUseMyMetrics(dlist->sc);
	if ( metrics!=NULL && metrics->sc!=sc )
    continue;
	else if ( metrics==NULL && !isprobablybase )
    continue;
	else if ( metrics==NULL && sc->width!=dlist->sc->width )
    continue;
	SCPreserveState(dlist->sc,false);
	SplinePointListShift(dlist->sc->layers[ly_fore].splines,off,true);
	for ( ref = dlist->sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next )
		if ( ref->sc!=sc ) {
	    SplinePointListShift(ref->layers[0].splines,off,true);
	    ref->transform[4] += off;
	    ref->bb.minx += off; ref->bb.maxx += off;
	}
	SCUpdateAll(dlist->sc);
	SCSynchronizeLBearing(dlist->sc,off);
    }
}
