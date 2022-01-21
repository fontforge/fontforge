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

#include "bitmapcontrol.h"

#include "autohint.h"
#include "bitmapcontrol.h"
#include "bvedit.h"
#include "fontforgevw.h"
#include "splinefill.h"
#include "splinesaveafm.h"
#include "ustring.h"

#include <math.h>

int bdfcontrol_lastwhich = bd_selected;

static void RemoveBDFWindows(BDFFont *bdf) {
    int i;

    for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
	BCDestroyAll(bdf->glyphs[i]);
    }
    if ( !no_windowing_ui ) {
	ff_progress_allow_events();
	/* Just in case... */
	ff_progress_allow_events();
    }
    /* We can't free the bdf until all the windows have executed their destroy*/
    /*  routines (which they will when they get the destroy window event) */
    /*  because those routines depend on the bdf existing ... */
}

static BDFFont *BDFNew(SplineFont *sf,int pixel_size, int depth) {
    BDFFont *new = chunkalloc(sizeof(BDFFont));
    int linear_scale = 1<<(depth/2);

    new->sf = sf;
    new->glyphcnt = new->glyphmax = sf->glyphcnt;
    new->glyphs = calloc(new->glyphcnt,sizeof(BDFChar *));
    new->pixelsize = pixel_size;
    new->ascent = (sf->ascent*pixel_size+.5)/(sf->ascent+sf->descent);
    new->descent = pixel_size-new->ascent;
    new->res = -1;
    if ( linear_scale!=1 )
	BDFClut(new,linear_scale);
return( new );
}

static void SFRemoveUnwantedBitmaps(SplineFont *sf,int32_t *sizes) {
    BDFFont *bdf, *prev, *next;
    FontViewBase *fv;
    int i;

    for ( prev = NULL, bdf=sf->bitmaps; bdf!=NULL; bdf = next ) {
	next = bdf->next;
	for ( i=0; sizes[i]!=0 && ((sizes[i]&0xffff)!=bdf->pixelsize || (sizes[i]>>16)!=BDFDepth(bdf)); ++i );
	if ( sizes[i]==0 ) {
	    /* To be deleted */
	    if ( prev==NULL )
		sf->bitmaps = next;
	    else
		prev->next = next;
	    for ( fv=sf->fv; fv!=NULL; fv=fv->nextsame ) {
		if ( fv->active_bitmap==bdf ) {
		    if ( sf->onlybitmaps && sf->bitmaps!=NULL )
			FVChangeDisplayBitmap(fv,sf->bitmaps);
		    else
			FVShowFilled(fv);
		}
	    }
	    RemoveBDFWindows(bdf);
	    BDFFontFree(bdf);
	    sf->changed = true;
	} else {
	    sizes[i] = -sizes[i];		/* don't need to create it */
	    prev = bdf;
	}
    }
}

static void SFFigureBitmaps(SplineFont *sf,int32_t *sizes,int usefreetype,int rasterize,int layer) {
    BDFFont *bdf;
    int i, first;
    void *freetypecontext = NULL;

    SFRemoveUnwantedBitmaps(sf,sizes);

    first = true;
    for ( i=0; sizes[i]!=0 ; ++i ) if ( sizes[i]>0 ) {
	if ( first && autohint_before_generate )
	    SplineFontAutoHint(sf,layer);
	if ( first && usefreetype )
	    freetypecontext = FreeTypeFontContext(sf,NULL,NULL,layer);
	if ( !rasterize )
	    bdf = BDFNew(sf,sizes[i]&0xffff,sizes[i]>>16);
	else if ( freetypecontext )
	    bdf = SplineFontFreeTypeRasterize(freetypecontext,sizes[i]&0xffff,sizes[i]>>16);
	else if ( usefreetype )
	    bdf = SplineFontFreeTypeRasterizeNoHints(sf,layer,sizes[i]&0xffff,sizes[i]>>16);
	else
	    bdf = SplineFontAntiAlias(sf,layer,sizes[i]&0xffff,1<<((sizes[i]>>16)/2));
	bdf->next = sf->bitmaps;
	sf->bitmaps = bdf;
	sf->changed = true;
	first = false;
    }
    if ( freetypecontext )
	FreeTypeFreeContext(freetypecontext);

    /* Order the list */
    SFOrderBitmapList(sf);
}

static int SizeExists(BDFFont *list, int size) {
    BDFFont *bdf;

    for ( bdf=list; bdf!=NULL; bdf = bdf->next ) {
	if ( (size&0xffff)==bdf->pixelsize && (size>>16)==BDFDepth(bdf) )
return( true );
    }
return( false );
}

static void FVScaleBitmaps(FontViewBase *fv,int32_t *sizes, int rasterize) {
    BDFFont *bdf, *scale;
    int i, cnt=0;

    for ( i=0; sizes[i]!=0 ; ++i ) if ( sizes[i]>0 )
	++cnt;
    scale = fv->active_bitmap;
    ff_progress_start_indicator(10,_("Scaling Bitmaps"),_("Scaling Bitmaps"),0,cnt,1);

    for ( i=0; sizes[i]!=0 ; ++i ) if ( !SizeExists(fv->sf->bitmaps,sizes[i]) ) {
	if ( rasterize )
	    bdf = BitmapFontScaleTo(scale,sizes[i]);
	else
	    bdf = BDFNew(fv->sf,sizes[i]&0xffff,sizes[i]>>16);
	if ( bdf==NULL )
    continue;
	bdf->next = fv->sf->bitmaps;
	fv->sf->bitmaps = bdf;
	fv->sf->changed = true;
	if ( !ff_progress_next())
    break;
    }
    ff_progress_end_indicator();

    /* Order the list */
    SFOrderBitmapList(fv->sf);

    SFRemoveUnwantedBitmaps(fv->sf,sizes);
}

static void ReplaceBDFC(SplineFont *sf,int32_t *sizes,int gid,
	void *freetypecontext, int usefreetype,int layer) {
    BDFFont *bdf;
    BDFChar *bdfc, temp;
    int i;

    if ( gid==-1 || gid>=sf->glyphcnt || sf->glyphs[gid]==NULL )
return;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( i=0; sizes[i]!=0 && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16)); ++i );
	if ( sizes[i]!=0 ) {
	    bdfc = NULL;
	    if ( freetypecontext )
		bdfc = SplineCharFreeTypeRasterize(freetypecontext,gid,bdf->pixelsize,72,BDFDepth(bdf));
	    else if ( usefreetype )
		bdfc = SplineCharFreeTypeRasterizeNoHints(sf->glyphs[gid],layer,bdf->pixelsize,72,BDFDepth(bdf));
	    if ( bdfc==NULL ) {
		if ( autohint_before_generate && 
			sf->glyphs[gid]->changedsincelasthinted &&
			!sf->glyphs[gid]->manualhints )
		    SplineCharAutoHint(sf->glyphs[gid],layer,NULL);
		bdfc = SplineCharAntiAlias(sf->glyphs[gid],layer,bdf->pixelsize,(1<<(BDFDepth(bdf)/2)));
	    }
	    if ( bdf->glyphs[gid]==NULL )
		bdf->glyphs[gid] = bdfc;
	    else {
		temp = *(bdf->glyphs[gid]);
		*bdf->glyphs[gid] = *bdfc;
		*bdfc = temp;
		bdf->glyphs[gid]->views = bdfc->views;
		bdfc->views = NULL;
		BDFCharFree(bdfc);
		BCRefreshAll(bdf->glyphs[gid]);
	    }
	}
    }
}

static int FVRegenBitmaps(CreateBitmapData *bd,int32_t *sizes,int usefreetype) {
    FontViewBase *fv = bd->fv, *selfv = bd->which==bd_all ? NULL : fv;
    SplineFont *sf = bd->sf, *subsf, *bdfsf = sf->cidmaster!=NULL ? sf->cidmaster : sf;
    int i,j;
    BDFFont *bdf;
    void *freetypecontext=NULL;

    for ( i=0; sizes[i]!=0; ++i ) {
	for ( bdf = bdfsf->bitmaps; bdf!=NULL &&
		(bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16));
		bdf=bdf->next );
	if ( bdf==NULL ) {
	    ff_post_notice(_("Missing Bitmap"),_("Attempt to regenerate a pixel size that has not been created (%d@%d)"),
			    sizes[i]&0xffff, sizes[i]>>16);
return( false );
	}
    }
    if ( bd->which==bd_current && bd->sc!=NULL ) {
	if ( usefreetype )
	    freetypecontext = FreeTypeFontContext(bd->sc->parent,bd->sc, selfv,bd->layer);
	ReplaceBDFC(bd->sc->parent,sizes,bd->sc->orig_pos,freetypecontext,usefreetype,bd->layer);
	if ( freetypecontext )
	    FreeTypeFreeContext(freetypecontext);
    } else {
	if ( bdfsf->subfontcnt!=0 && bd->which==bd_all ) {
	    for ( j=0 ; j<bdfsf->subfontcnt; ++j ) {
		subsf = bdfsf->subfonts[j];
	        if ( usefreetype && freetypecontext==NULL )
		    freetypecontext = FreeTypeFontContext(subsf,NULL, selfv, bd->layer);
		for ( i=0; i<subsf->glyphcnt; ++i ) {
		    if ( SCWorthOutputting(subsf->glyphs[i])) {
			ReplaceBDFC(subsf,sizes,i,freetypecontext,usefreetype, bd->layer);
		    }
		}
		if ( freetypecontext )
		    FreeTypeFreeContext(freetypecontext);
		freetypecontext = NULL;
	    }
	} else {
	    if ( usefreetype && freetypecontext==NULL )
	        freetypecontext = FreeTypeFontContext(sf,NULL, selfv, bd->layer);
	    for ( i=0; i<fv->map->enccount; ++i ) {
		if ( fv->selected[i] || bd->which == bd_all ) {
		    ReplaceBDFC(sf,sizes,fv->map->map[i],freetypecontext,usefreetype, bd->layer);
		}
	    }
	    if ( freetypecontext )
		FreeTypeFreeContext(freetypecontext);
	    freetypecontext = NULL;
	}
    }
    sf->changed = true;
    FVRefreshAll(fv->sf);
return( true );
}

static void BDFClearGlyph(BDFFont *bdf, int gid, int pass) {

    if ( bdf==NULL || bdf->glyphs[gid]==NULL )
return;
    if ( pass==0 ) {
	BCDestroyAll(bdf->glyphs[gid]);
	ff_progress_allow_events();
    } else {
	BDFCharFree(bdf->glyphs[gid]);
	bdf->glyphs[gid] = NULL;
    }
}


static int FVRemoveBitmaps(CreateBitmapData *bd,int32_t *sizes) {
    FontViewBase *fv = bd->fv;
    SplineFont *sf = bd->sf, *bdfsf = sf->cidmaster!=NULL ? sf->cidmaster : sf;
    int i,j,pass;
    BDFFont *bdf;
    int gid;

    /* The two passes are because we want to make sure any windows open on */
    /*  these bitmaps will get closed before we free any data. Complicated */
    /*  because the Window manager can delay things */
    for ( pass=0; pass<2; ++pass ) {
	for ( i=0; sizes[i]!=0; ++i ) {
	    for ( bdf = bdfsf->bitmaps; bdf!=NULL &&
		    (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16));
		    bdf=bdf->next );
	    if ( bdf!=NULL ) {
		if ( bd->which==bd_current && bd->sc!=NULL )
		    BDFClearGlyph(bdf,bd->sc->orig_pos,pass);
		else if ( bd->which==bd_all ) {
		    for ( j=0; j<bdf->glyphcnt; ++j )
			BDFClearGlyph(bdf,j,pass);
		} else {
		    for ( j=0; j<fv->map->enccount; ++j )
			if ( fv->selected[j] && (gid = fv->map->map[j])!=-1 )
			    BDFClearGlyph(bdf,gid,pass);
		}
	    }
	}
	ff_progress_allow_events();
    }
    sf->changed = true;
    FVRefreshAll(fv->sf);
return( true );
}

void BitmapsDoIt(CreateBitmapData *bd,int32_t *sizes,int usefreetype) {

    if ( bd->isavail==-1 )
	FVRemoveBitmaps(bd,sizes);
    else if ( bd->isavail && bd->sf->onlybitmaps && bd->sf->bitmaps!=NULL )
	FVScaleBitmaps(bd->fv,sizes,bd->rasterize);
    else if ( bd->isavail ) {
	SFFigureBitmaps(bd->sf,sizes,usefreetype,bd->rasterize,bd->layer);
	/* If we had an empty font, to which we've just added bitmaps, then */
	/*  presumably we should treat this as a bitmap font and switch the */
	/*  fontview so that it shows one of the bitmaps */
	if ( bd->sf->onlybitmaps && bd->sf->bitmaps!=NULL ) {
	    BDFFont *bdf;
	    FontViewBase *fvs;
            // Select the last bitmap font.
	    for ( bdf=bd->sf->bitmaps; bdf->next!=NULL; bdf=bdf->next );
	    for ( fvs = bd->sf->fv; fvs!=NULL; fvs= fvs->nextsame )
		FVChangeDisplayBitmap(fvs,bdf);
	}
    } else {
	if ( FVRegenBitmaps(bd,sizes,usefreetype))
	    bdfcontrol_lastwhich = bd->which;
	else {
	    bd->done = false;
return;
	}
    }
    bd->done = true;
}


int BitmapControl(FontViewBase *fv,int32_t *sizes,int isavail,int rasterize) {
    CreateBitmapData bd;

    memset(&bd,0,sizeof(bd));
    bd.fv = fv;
    bd.sf = fv->sf;
    bd.isavail = isavail;
    bd.which = bd_selected;
    bd.rasterize = rasterize;
    bd.layer = fv->active_layer;
    BitmapsDoIt(&bd,sizes,hasFreeType());
return( bd.done );
}
