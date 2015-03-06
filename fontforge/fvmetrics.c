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
#include <math.h>
#include <ustring.h>
#include "fvmetrics.h"

static void DoChar(SplineChar *sc,CreateWidthData *wd, FontViewBase *fv,
	BDFChar *bc) {
    real transform[6];
    DBounds bb;
    IBounds ib;
    int width=0;
    BVTFunc bvts[2];
    BDFFont *bdf;
    RefChar *r = HasUseMyMetrics(sc,fv->active_layer);

    /* Can't change the horizontal or vertical advance if there's a "use my metrics" bit set */
    if ( r!=NULL && wd->wtype != wt_lbearing )
return;

    if ( wd->wtype == wt_width ) {
	if ( wd->type==st_set )
	    width = wd->setto;
	else if ( wd->type == st_incr )
	    width = sc->width + wd->increment;
	else
	    width = sc->width * wd->scale/100;
	sc->widthset = true;
	if ( width!=sc->width ) {
	    SCPreserveWidth(sc);
	    SCSynchronizeWidth(sc,width,sc->width,fv);
	}
    } else if ( wd->wtype == wt_lbearing ) {
	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[5] = 0;
	bvts[1].func = bvt_none;
	bvts[0].func = bvt_transmove; bvts[0].y = 0;
	if ( bc==NULL ) {
	    SplineCharFindBounds(sc,&bb);
	    if ( wd->type==st_set )
		transform[4] = wd->setto-bb.minx;
	    else if ( wd->type == st_incr )
		transform[4] = wd->increment;
	    else
		transform[4] = bb.minx*wd->scale/100 - bb.minx;
	} else {
	    double scale = (fv->sf->ascent+fv->sf->descent)/(double) (fv->active_bitmap->pixelsize);
	    BDFCharFindBounds(bc,&ib);
	    if ( wd->type==st_set )
		transform[4] = wd->setto-ib.minx*scale;
	    else if ( wd->type == st_incr )
		transform[4] = wd->increment;
	    else
		transform[4] = scale*ib.minx*wd->scale/100 - ib.minx;
	}
	if ( transform[4]!=0 )
	{
	    FVTrans(fv,sc,transform,NULL,fvt_dontmovewidth | fvt_alllayers );
	    bvts[0].x = transform[4];
	    for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next ) if ( bdf->glyphs[sc->orig_pos]!=NULL )
		BCTrans(bdf,bdf->glyphs[sc->orig_pos],bvts,fv);
	}
return;
    } else if ( wd->wtype == wt_rbearing ) {
	if ( bc==NULL ) {
	    SplineCharFindBounds(sc,&bb);
	    if ( wd->type==st_set )
		width = bb.maxx + wd->setto;
	    else if ( wd->type == st_incr )
		width = sc->width+wd->increment;
	    else
		width = (sc->width-bb.maxx) * wd->scale/100 + bb.maxx;
	} else {
	    double scale = (fv->sf->ascent+fv->sf->descent)/(double) (fv->active_bitmap->pixelsize);
	    BDFCharFindBounds(bc,&ib);
	    ++ib.maxx;
	    if ( wd->type==st_set )
		width = rint(ib.maxx*scale + wd->setto);
	    else if ( wd->type == st_incr )
		width = rint(sc->width+wd->increment);
	    else
		width = rint(scale * (bc->width-ib.maxx) * wd->scale/100 + ib.maxx*scale);
	}
	if ( width!=sc->width ) {
	    SCPreserveWidth(sc);
	    SCSynchronizeWidth(sc,width,sc->width,fv);
	}
    } else if ( wd->wtype == wt_bearings ) {
	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[5] = 0;
	bvts[1].func = bvt_none;
	bvts[0].func = bvt_transmove; bvts[0].y = 0;
	if ( bc==NULL ) {
	    SplineCharFindBounds(sc,&bb);
	    if ( wd->type==st_set ) {
		transform[4] = wd->setto-bb.minx;
		width = bb.maxx-bb.minx + 2*wd->setto;
	    } else if ( wd->type == st_incr ) {
		transform[4] = wd->increment;
		width = sc->width + 2*wd->increment;
	    } else {
		transform[4] = bb.minx*wd->scale/100 - bb.minx;
		width = bb.maxx-bb.minx +
			(bb.minx + (sc->width-bb.maxx))*wd->scale/100;
	    }
	} else {
	    double scale = (fv->sf->ascent+fv->sf->descent)/(double) (fv->active_bitmap->pixelsize);
	    BDFCharFindBounds(bc,&ib);
	    ++ib.maxx;
	    if ( wd->type==st_set ) {
		transform[4] = wd->setto-ib.minx;
		width = (ib.maxx-ib.minx + 2*wd->setto);
	    } else if ( wd->type == st_incr ) {
		transform[4] = wd->increment;
		width = sc->width + 2*wd->increment;
	    } else {
		transform[4] = ib.minx*wd->scale/100 - ib.minx;
		width = ib.maxx-ib.minx +
			(ib.minx + (bc->width-ib.maxx))*wd->scale/100;
	    }
	    transform[4] *= scale;
	    width = rint(width*scale);
	}
	if ( width!=sc->width ) {
	    SCPreserveWidth(sc);
	    SCSynchronizeWidth(sc,width,sc->width,fv);
	}
	if ( transform[4]!=0 ) {
	    FVTrans(fv,sc,transform,NULL, fvt_dontmovewidth | fvt_alllayers );
	    bvts[0].x = transform[4];
	    for ( bdf = fv->sf->bitmaps; bdf!=NULL; bdf=bdf->next ) if ( bdf->glyphs[sc->orig_pos]!=NULL )
		BCTrans(bdf,bdf->glyphs[sc->orig_pos],bvts,fv);
	}
return;
    } else {
	if ( wd->type==st_set )
	    width = wd->setto;
	else if ( wd->type == st_incr )
	    width = sc->vwidth + wd->increment;
	else
	    width = sc->vwidth * wd->scale/100;
	if ( width!=sc->vwidth ) {
	    SCPreserveVWidth(sc);
	    sc->vwidth = width;
	}
    }
    SCCharChangedUpdate(sc,fv->active_layer);
}

void FVDoit(CreateWidthData *wd) {
    FontViewBase *fv = (FontViewBase *) (wd->_fv);
    int i;
    BDFChar *bc;

    if ( fv->sf->onlybitmaps && fv->active_bitmap!=NULL && fv->sf->bitmaps!=NULL ) {
	double scale = (fv->sf->ascent+fv->sf->descent)/(double) fv->active_bitmap->pixelsize;
	wd->setto *= scale;
	wd->increment *= scale;
    }
    bc = NULL;
    for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	SplineChar *sc;

	sc = SFMakeChar(fv->sf,fv->map,i);
	if ( fv->sf->onlybitmaps && fv->sf->bitmaps!=NULL )
	    if ( fv->active_bitmap!=NULL )
		bc = BDFMakeChar(fv->active_bitmap,fv->map,i);
	DoChar(sc,wd,fv,bc);
    }
    wd->done = true;
}

void CVDoit(CreateWidthData *wd) {
    CharView *cv = (CharView *) (wd->_fv);

    DoChar(cv->b.sc,wd,(FontViewBase *) (cv->b.fv),NULL);
    wd->done = true;
}
void GenericVDoit(CreateWidthData *wd) {
    FontViewBase* fv = (FontViewBase*)wd->_fv;
    SplineChar* sc = (SplineChar*)wd->_sc;
    
    DoChar(sc,wd,fv,NULL);
    wd->done = true;
}

void FVSetWidthScript(FontViewBase *fv,enum widthtype wtype,int val,int incr) {
    CreateWidthData wd;

    memset(&wd,0,sizeof(wd));
    wd._fv = fv;
    wd.doit = FVDoit;
    wd.setto = wd.increment = wd.scale = val;
    wd.type = incr==0 ? st_set : incr==2 ? st_scale : st_incr;
    wd.wtype = wtype;
    FVDoit(&wd);
}
