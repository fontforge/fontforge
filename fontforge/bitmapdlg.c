/* Copyright (C) 2000-2006 by George Williams */
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
#include "gwidget.h"
#include "ustring.h"
#include <math.h>
#include <gkeysym.h>

static int oldusefreetype=1;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
int oldsystem=0 /* X */;

typedef struct createbitmapdata {
    unsigned int done: 1;
    FontView *fv;
    SplineFont *sf;
    SplineChar *sc;
    int isavail;
    int which;
    int rasterize;
#ifdef FONTFORGE_CONFIG_GDRAW
    GWindow gw;
#elif defined(FONTFORGE_CONFIG_GTK)
#else
#endif
} CreateBitmapData;

enum { bd_all, bd_selected, bd_current };
static int lastwhich = bd_selected;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static GTextInfo which[] = {
    { (unichar_t *) N_("All Glyphs"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1 },
    { (unichar_t *) N_("Selected Glyphs"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1 },
    { (unichar_t *) N_("Current Glyph"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1 },
    { NULL }};

static void RemoveBDFWindows(BDFFont *bdf) {
    int i;
    BitmapView *bv, *next;

    for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL ) {
	for ( bv = bdf->glyphs[i]->views; bv!=NULL; bv=next ) {
	    next = bv->next;
	    GDrawDestroyWindow(bv->gw);
	}
    }
    if ( !no_windowing_ui ) {
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
	/* Just in case... */
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
    }
    /* We can't free the bdf until all the windows have executed their destroy*/
    /*  routines (which they will when they get the destroy window event) */
    /*  because those routines depend on the bdf existing ... */
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static BDFFont *BDFNew(SplineFont *sf,int pixel_size, int depth) {
    BDFFont *new = chunkalloc(sizeof(BDFFont));
    int linear_scale = 1<<(depth/2);

    new->sf = sf;
    new->glyphcnt = new->glyphmax = sf->glyphcnt;
    new->glyphs = gcalloc(new->glyphcnt,sizeof(BDFChar *));
    new->pixelsize = pixel_size;
    new->ascent = (sf->ascent*pixel_size+.5)/(sf->ascent+sf->descent);
    new->descent = pixel_size-new->ascent;
    new->res = -1;
    if ( linear_scale!=1 )
	BDFClut(new,linear_scale);
return( new );
}

void SFOrderBitmapList(SplineFont *sf) {
    BDFFont *bdf, *prev, *next;
    BDFFont *bdf2, *prev2;

    for ( prev = NULL, bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	for ( prev2=NULL, bdf2=bdf->next; bdf2!=NULL; bdf2 = bdf2->next ) {
	    if ( bdf->pixelsize>bdf2->pixelsize ||
		    (bdf->pixelsize==bdf2->pixelsize && BDFDepth(bdf)>BDFDepth(bdf2)) ) {
		if ( prev==NULL )
		    sf->bitmaps = bdf2;
		else
		    prev->next = bdf2;
		if ( prev2==NULL ) {
		    bdf->next = bdf2->next;
		    bdf2->next = bdf;
		} else {
		    next = bdf->next;
		    bdf->next = bdf2->next;
		    bdf2->next = next;
		    prev2->next = bdf;
		}
		next = bdf;
		bdf = bdf2;
		bdf2 = next;
	    }
	    prev2 = bdf2;
	}
	prev = bdf;
    }
}

static void SFRemoveUnwantedBitmaps(SplineFont *sf,int32 *sizes) {
    BDFFont *bdf, *prev, *next;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FontView *fv;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	    for ( fv=sf->fv; fv!=NULL; fv=fv->nextsame ) {
		if ( fv->show==bdf ) {
		    if ( sf->onlybitmaps && sf->bitmaps!=NULL )
			FVChangeDisplayBitmap(fv,sf->bitmaps);
		    else
			FVShowFilled(sf->fv);
		}
	    }
	    RemoveBDFWindows(bdf);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	    BDFFontFree(bdf);
	    sf->changed = true;
	} else {
	    sizes[i] = -sizes[i];		/* don't need to create it */
	    prev = bdf;
	}
    }
}

static void SFFigureBitmaps(SplineFont *sf,int32 *sizes,int usefreetype,int rasterize) {
    BDFFont *bdf;
    int i, first;
    void *freetypecontext = NULL;

    SFRemoveUnwantedBitmaps(sf,sizes);

    first = true;
    for ( i=0; sizes[i]!=0 ; ++i ) if ( sizes[i]>0 ) {
	if ( first && autohint_before_rasterize )
	    SplineFontAutoHint(sf);
	if ( first && usefreetype )
	    freetypecontext = FreeTypeFontContext(sf,NULL,NULL);
	if ( !rasterize )
	    bdf = BDFNew(sf,sizes[i]&0xffff,sizes[i]>>16);
	else if ( freetypecontext )
	    bdf = SplineFontFreeTypeRasterize(freetypecontext,sizes[i]&0xffff,sizes[i]>>16);
	else if ( usefreetype )
	    bdf = SplineFontFreeTypeRasterizeNoHints(sf,sizes[i]&0xffff,sizes[i]>>16);
	else
	    bdf = SplineFontAntiAlias(sf,sizes[i]&0xffff,1<<((sizes[i]>>16)/2));
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

static void FVScaleBitmaps(FontView *fv,int32 *sizes, int rasterize) {
    BDFFont *bdf, *scale;
    int i, cnt=0;

    for ( i=0; sizes[i]!=0 ; ++i ) if ( sizes[i]>0 )
	++cnt;
    scale = fv->show;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_start_indicator(10,_("Scaling Bitmaps"),_("Scaling Bitmaps"),0,cnt,1);
#endif

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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( !gwwv_progress_next())
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    gwwv_progress_end_indicator();
#endif

    /* Order the list */
    SFOrderBitmapList(fv->sf);

    SFRemoveUnwantedBitmaps(fv->sf,sizes);
}

static void ReplaceBDFC(SplineFont *sf,int32 *sizes,int gid,
	void *freetypecontext, int usefreetype) {
    BDFFont *bdf;
    BDFChar *bdfc, temp;
    int i;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    BitmapView *bv;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    if ( gid==-1 || gid>=sf->glyphcnt || sf->glyphs[gid]==NULL )
return;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	for ( i=0; sizes[i]!=0 && (bdf->pixelsize!=(sizes[i]&0xffff) || BDFDepth(bdf)!=(sizes[i]>>16)); ++i );
	if ( sizes[i]!=0 ) {
	    bdfc = NULL;
	    if ( freetypecontext )
		bdfc = SplineCharFreeTypeRasterize(freetypecontext,gid,bdf->pixelsize,BDFDepth(bdf));
	    else if ( usefreetype )
		bdfc = SplineCharFreeTypeRasterizeNoHints(sf->glyphs[gid],bdf->pixelsize,BDFDepth(bdf));
	    if ( bdfc==NULL ) {
		if ( autohint_before_rasterize && 
			sf->glyphs[gid]->changedsincelasthinted &&
			!sf->glyphs[gid]->manualhints )
		    SplineCharAutoHint(sf->glyphs[gid],NULL);
		bdfc = SplineCharAntiAlias(sf->glyphs[gid],bdf->pixelsize,(1<<(BDFDepth(bdf)/2)));
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
		for ( bv = bdf->glyphs[gid]->views; bv!=NULL; bv=bv->next ) {
		    GDrawRequestExpose(bv->v,NULL,false);
		    /* Mess with selection?????!!!!! */
		}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	    }
	}
    }
}

static int FVRegenBitmaps(CreateBitmapData *bd,int32 *sizes,int usefreetype) {
    FontView *fv = bd->fv;
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
	    freetypecontext = FreeTypeFontContext(bd->sc->parent,bd->sc,fv);
	ReplaceBDFC(bd->sc->parent,sizes,bd->sc->orig_pos,freetypecontext,usefreetype);
	if ( freetypecontext )
	    FreeTypeFreeContext(freetypecontext);
    } else {
	if ( bdfsf->subfontcnt!=0 && bd->which==bd_all ) {
	    for ( j=0 ; j<bdfsf->subfontcnt; ++j ) {
		subsf = bdfsf->subfonts[j];
		for ( i=0; i<subsf->glyphcnt; ++i ) {
		    if ( SCWorthOutputting(subsf->glyphs[i])) {
			if ( usefreetype && freetypecontext==NULL )
			    freetypecontext = FreeTypeFontContext(subsf,NULL,fv);
			ReplaceBDFC(subsf,sizes,i,freetypecontext,usefreetype);
		    }
		}
		if ( freetypecontext )
		    FreeTypeFreeContext(freetypecontext);
		freetypecontext = NULL;
	    }
	} else {
	    for ( i=0; i<fv->map->enccount; ++i ) {
		if ( fv->selected[i] || bd->which == bd_all ) {
		    if ( usefreetype && freetypecontext==NULL )
			freetypecontext = FreeTypeFontContext(sf,NULL,fv);
		    ReplaceBDFC(sf,sizes,fv->map->map[i],freetypecontext,usefreetype);
		}
	    }
	    if ( freetypecontext )
		FreeTypeFreeContext(freetypecontext);
	    freetypecontext = NULL;
	}
    }
    sf->changed = true;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( fv->show!=fv->filled ) {
	for ( i=0; sizes[i]!=0 && ((sizes[i]&0xffff)!=fv->show->pixelsize || (sizes[i]>>16)!=BDFDepth(fv->show)); ++i );
	if ( sizes[i]!=0 && fv->v!=NULL )
	    GDrawRequestExpose(fv->v,NULL,false );
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
return( true );
}

static void BDFClearGlyph(BDFFont *bdf, int gid, int pass) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    BitmapView *bv, *bvnext;
#endif

    if ( bdf==NULL || bdf->glyphs[gid]==NULL )
return;
    if ( pass==0 ) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	for ( bv = bdf->glyphs[gid]->views; bv!=NULL; bv = bvnext ) {
	    bvnext = bv->next;
	    GDrawDestroyWindow(bv->gw);
	}
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
#endif
    } else {
	BDFCharFree(bdf->glyphs[gid]);
	bdf->glyphs[gid] = NULL;
    }
}


static int FVRemoveBitmaps(CreateBitmapData *bd,int32 *sizes) {
    FontView *fv = bd->fv;
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);
#endif
    }
    sf->changed = true;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( fv->show!=fv->filled ) {
	for ( i=0; sizes[i]!=0 && ((sizes[i]&0xffff)!=fv->show->pixelsize || (sizes[i]>>16)!=BDFDepth(fv->show)); ++i );
	if ( sizes[i]!=0 && fv->v!=NULL )
	    GDrawRequestExpose(fv->v,NULL,false );
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
return( true );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void BitmapsDoIt(CreateBitmapData *bd,int32 *sizes,int usefreetype) {

    if ( bd->isavail==-1 )
	FVRemoveBitmaps(bd,sizes);
    else if ( bd->isavail && bd->sf->onlybitmaps && bd->sf->bitmaps!=NULL )
	FVScaleBitmaps(bd->fv,sizes,bd->rasterize);
    else if ( bd->isavail ) {
	SFFigureBitmaps(bd->sf,sizes,usefreetype,bd->rasterize);
	/* If we had an empty font, to which we've just added bitmaps, then */
	/*  presumably we should treat this as a bitmap font and switch the */
	/*  fontview so that it shows one of the bitmaps */
	if ( bd->sf->onlybitmaps && bd->sf->bitmaps!=NULL ) {
	    BDFFont *bdf;
	    FontView *fvs;
	    for ( bdf=bd->sf->bitmaps; bdf->next!=NULL; bdf=bdf->next );
	    for ( fvs = bd->sf->fv; fvs!=NULL; fvs=fvs->nextsame )
		FVChangeDisplayBitmap(fvs,bdf);
	}
    } else {
	if ( FVRegenBitmaps(bd,sizes,usefreetype))
	    lastwhich = bd->which;
	else {
	    bd->done = false;
return;
	}
    }
    bd->done = true;
}

#define CID_Which	1001
#define CID_Pixel	1002
#define CID_75		1003
#define CID_100		1004
#define CID_75Lab	1005
#define CID_100Lab	1006
#define CID_X		1007
#define CID_Win		1008
#define CID_Mac		1009
#define CID_FreeType	1010
#define CID_RasterizedStrikes	1011

static int GetSystem(GWindow gw) {
    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_X)) )
return( CID_X );
    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Win)) )
return( CID_Win );

return( CID_Mac );
}
    
static int32 *ParseList(GWindow gw, int cid,int *err, int final) {
    const unichar_t *val = _GGadgetGetTitle(GWidgetGetControl(gw,cid)), *pt;
    unichar_t *end, *end2;
    int i;
    real *sizes;
    int32 *ret;
    int system = GetSystem(gw);
    int scale;

    *err = false;
    end2 = NULL;
    for ( i=1, pt = val; (end = u_strchr(pt,',')) || (end2=u_strchr(pt,' ')); ++i ) {
	if ( end!=NULL && end2!=NULL ) {
	    if ( end2<end ) end = end2;
	} else if ( end2!=NULL )
	    end = end2;
	pt = end+1;
	end2 = NULL;
    }
    sizes = galloc((i+1)*sizeof(real));
    ret = galloc((i+1)*sizeof(int32));

    for ( i=0, pt = val; *pt!='\0' ; ) {
	sizes[i]=u_strtod(pt,&end);
	if ( *end=='@' )
	    ret[i] = (u_strtol(end+1,&end,10)<<16);
	else
	    ret[i] = 0x10000;
	if ( sizes[i]>0 ) ++i;
	if ( *end!=' ' && *end!=',' && *end!='\0' ) {
	    free(sizes); free(ret);
	    if ( final )
		Protest8(_("Pixel Sizes:"));
	    *err = true;
return( NULL );
	}
	while ( *end==' ' || *end==',' ) ++end;
	pt = end;
    }
    sizes[i] = 0; ret[i] = 0;

    if ( cid==CID_75 ) {
	scale = system==CID_X?75:system==CID_Win?96:72;
	for ( i=0; sizes[i]!=0; ++i )
	    ret[i] |= (int) rint(sizes[i]*scale/72);
    } else if ( cid==CID_100 ) {
	scale = system==CID_X?100:system==CID_Win?120:100;
	for ( i=0; sizes[i]!=0; ++i )
	    ret[i] |= (int) rint(sizes[i]*scale/72);
    } else
	for ( i=0; sizes[i]!=0; ++i )
	    ret[i] |= (int) rint(sizes[i]);
    free(sizes);
return( ret );
}

static int CB_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int err;
	CreateBitmapData *bd = GDrawGetUserData(GGadgetGetWindow(g));
	int32 *sizes = ParseList(bd->gw, CID_Pixel,&err, true);
	if ( err )
return( true );
	oldusefreetype = GGadgetIsChecked(GWidgetGetControl(bd->gw,CID_FreeType));
	oldsystem = GetSystem(bd->gw)-CID_X;
	bd->rasterize = true;
	if ( bd->isavail<true )
	    bd->which = GGadgetGetFirstListSelectedItem(GWidgetGetControl(bd->gw,CID_Which));
	if ( bd->isavail==1 )
	    bd->rasterize = GGadgetIsChecked(GWidgetGetControl(bd->gw,CID_RasterizedStrikes));
	BitmapsDoIt(bd,sizes,oldusefreetype);
	free(sizes);
	SavePrefs();
    }
return( true );
}

static int CB_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CreateBitmapData *bd = GDrawGetUserData(GGadgetGetWindow(g));
	bd->done = true;
    }
return( true );
}

static unichar_t *GenText(int32 *sizes,real scale) {
    int i;
    char *cret, *pt;
    unichar_t *uret;

    for ( i=0; sizes[i]!=0; ++i );
    pt = cret = galloc(i*10+1);
    for ( i=0; sizes[i]!=0; ++i ) {
	if ( pt!=cret ) *pt++ = ',';
	sprintf(pt,"%.1f",(double) ((sizes[i]&0xffff)*scale) );
	pt += strlen(pt);
	if ( pt[-1]=='0' && pt[-2]=='.' ) {
	    pt -= 2;
	    *pt = '\0';
	}
	if ( (sizes[i]>>16)!=1 ) {
	    sprintf(pt,"@%d", (int) (sizes[i]>>16) );
	    pt += strlen(pt);
	}
    }
    *pt = '\0';
    uret = uc_copy(cret);
    free(cret);
return( uret );
}

static void _CB_TextChange(CreateBitmapData *bd, GGadget *g) {
    int cid = (int) GGadgetGetCid(g);
    unichar_t *val;
    int err=false;
    int32 *sizes = ParseList(bd->gw,cid,&err,false);
    int ncid;
    int system = GetSystem(bd->gw);
    int scale;

    if ( err )
return;
    for ( ncid=CID_Pixel; ncid<=CID_100; ++ncid ) if ( ncid!=cid ) {
	if ( ncid==CID_Pixel )
	    scale = 72;
	else if ( ncid==CID_75 )
	    scale = system==CID_X?75: system==CID_Win?96 : 72;
	else
	    scale = system==CID_X?100: system==CID_Win?120 : 100;
	val = GenText(sizes,72./scale);
	GGadgetSetTitle(GWidgetGetControl(bd->gw,ncid),val);
	free(val);
    }
    free(sizes);
return;
}

static int CB_TextChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CreateBitmapData *bd = GDrawGetUserData(GGadgetGetWindow(g));
	_CB_TextChange(bd,g);
    }
return( true );
}

static void _CB_SystemChange(CreateBitmapData *bd) {
    int system = GetSystem(bd->gw);
    GGadgetSetTitle8(GWidgetGetControl(bd->gw,CID_75Lab),
	    system==CID_X?_("Point sizes on a 75 dpi screen"):
	    system==CID_Win?_("Point sizes on a 96 dpi screen"):
			    _("Point sizes on a 72 dpi screen"));
    GGadgetSetTitle8(GWidgetGetControl(bd->gw,CID_100Lab),
	    system==CID_Win?_("Point sizes on a 120 dpi screen"):
			    _("Point sizes on a 100 dpi screen"));
    GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_100Lab),system!=CID_Mac);
    GGadgetSetEnabled(GWidgetGetControl(bd->gw,CID_100),system!=CID_Mac);
    _CB_TextChange(bd,GWidgetGetControl(bd->gw,CID_Pixel));
}

static int CB_SystemChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	CreateBitmapData *bd = GDrawGetUserData(GGadgetGetWindow(g));
	_CB_SystemChange(bd);
    }
return( true );
}

static int bd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	CreateBitmapData *bd = GDrawGetUserData(gw);
	bd->done = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("elementmenu.html#Bitmaps");
return( true );
	}
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void BitmapDlg(FontView *fv,SplineChar *sc, int isavail) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[18], boxes[8], *varray[32], *harray[6][7];
    GTextInfo label[18];
    CreateBitmapData bd;
    int i,j,k,y;
    int32 *sizes;
    BDFFont *bdf;
    static int done= false;

    if ( !done ) {
	for ( i=0; which[i].text!=NULL; ++i )
	    which[i].text = (unichar_t *) _((char *) which[i].text);
	done = true;
    }

    bd.fv = fv;
    bd.sc = sc;
    bd.sf = fv->cidmaster ? fv->cidmaster : fv->sf;
    bd.isavail = isavail;
    bd.done = false;

    for ( bdf=bd.sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i );
/*
    if ( i==0 && isavail )
	i = 2;
*/
    sizes = galloc((i+1)*sizeof(int32));
    for ( bdf=bd.sf->bitmaps, i=0; bdf!=NULL; bdf=bdf->next, ++i )
	sizes[i] = bdf->pixelsize | (BDFDepth(bdf)<<16);
/*
    if ( i==0 && isavail ) {
	sizes[i++] = 12.5;
	sizes[i++] = 17;
    }
*/
    sizes[i] = 0;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = isavail==1 ? _("Bitmap Strikes Available...") :
				isavail==0 ? _("Regenerate Bitmap Glyphs...") :
					    _("Remove Bitmap Glyphs...");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,190));
    pos.height = GDrawPointsToPixels(NULL,252);
    bd.gw = GDrawCreateTopWindow(NULL,&pos,bd_e_h,&bd,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k=0;
    if ( isavail==1 ) {
	label[0].text = (unichar_t *) _("The list of current pixel bitmap sizes");
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[0].creator = GLabelCreate;
	varray[k++] = &gcd[0]; varray[k++] = NULL;

	label[1].text = (unichar_t *) _(" Removing a size will delete it.");
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 5+13;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].creator = GLabelCreate;
	varray[k++] = &gcd[1]; varray[k++] = NULL;

	if ( bd.sf->onlybitmaps && bd.sf->bitmaps!=NULL )
	    label[2].text = (unichar_t *) _(" Adding a size will create it by scaling.");
	else
	    label[2].text = (unichar_t *) _(" Adding a size will create it.");
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = 5+26;
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].creator = GLabelCreate;
	varray[k++] = &gcd[2]; varray[k++] = NULL;
	j = 3; y = 5+39+3;
    } else {
	if ( isavail==0 )
	    label[0].text = (unichar_t *) _("Specify bitmap sizes to be regenerated");
	else
	    label[0].text = (unichar_t *) _("Specify bitmap sizes to be removed");
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[0].creator = GLabelCreate;
	varray[k++] = &gcd[0]; varray[k++] = NULL;

	if ( lastwhich==bd_current && sc==NULL )
	    lastwhich = bd_selected;
	gcd[1].gd.label = &which[lastwhich];
	gcd[1].gd.u.list = which;
	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 5+13;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].gd.cid = CID_Which;
	gcd[1].creator = GListButtonCreate;
	which[bd_current].disabled = sc==NULL;
	which[lastwhich].selected = true;
	harray[0][0] = &gcd[1]; harray[0][1] = GCD_Glue; harray[0][2] = NULL;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray[0];
	boxes[2].creator = GHBoxCreate;
	varray[k++] = &boxes[2]; varray[k++] = NULL;

	j=2; y = 5+13+28;
    }

    label[j].text = (unichar_t *) _("X");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 10; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_X;
    gcd[j].gd.handle_controlevent = CB_SystemChange;
    gcd[j++].creator = GRadioCreate;
    harray[1][0] = &gcd[j-1];

    label[j].text = (unichar_t *) _("Win");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 50; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_Win;
    gcd[j].gd.handle_controlevent = CB_SystemChange;
    gcd[j++].creator = GRadioCreate;
    harray[1][1] = &gcd[j-1];

    label[j].text = (unichar_t *) _("Mac");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 90; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_Mac;
    gcd[j].gd.handle_controlevent = CB_SystemChange;
    gcd[j++].creator = GRadioCreate;
    y += 26;
    gcd[j-3+oldsystem].gd.flags |= gg_cb_on;
    harray[1][2] = &gcd[j-1]; harray[1][3] = GCD_Glue; harray[1][4] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray[1];
    boxes[3].creator = GHBoxCreate;
    varray[k++] = &boxes[3]; varray[k++] = NULL;
    varray[k++] = GCD_Glue; varray[k++] = NULL;

    label[j].text = (unichar_t *) _("Point sizes on a 75 dpi screen");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_75Lab;
    gcd[j++].creator = GLabelCreate;
    varray[k++] = &gcd[j-1]; varray[k++] = NULL;
    y += 13;

    label[j].text = GenText(sizes,oldsystem==0?72./75.:oldsystem==1?72/96.:1.);
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = y;
    gcd[j].gd.pos.width = 170;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_75;
    gcd[j].gd.handle_controlevent = CB_TextChange;
    gcd[j++].creator = GTextFieldCreate;
    y += 26;
    harray[2][0] = GCD_HPad10; harray[2][1] = &gcd[j-1]; harray[2][2] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = harray[2];
    boxes[4].creator = GHBoxCreate;
    varray[k++] = &boxes[4]; varray[k++] = NULL;

    label[j].text = (unichar_t *) _("Point sizes on a 100 dpi screen");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_100Lab;
    gcd[j++].creator = GLabelCreate;
    y += 13;
    varray[k++] = &gcd[j-1]; varray[k++] = NULL;

    label[j].text = GenText(sizes,oldsystem==1?72./120.:72/100.);
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = y;
    gcd[j].gd.pos.width = 170;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    if ( oldsystem==2 )
	gcd[j].gd.flags = gg_visible;
    gcd[j].gd.cid = CID_100;
    gcd[j].gd.handle_controlevent = CB_TextChange;
    gcd[j++].creator = GTextFieldCreate;
    y += 26;
    harray[3][0] = GCD_HPad10; harray[3][1] = &gcd[j-1]; harray[3][2] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = harray[3];
    boxes[5].creator = GHBoxCreate;
    varray[k++] = &boxes[5]; varray[k++] = NULL;

    label[j].text = (unichar_t *) _("Pixel Sizes:");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 5; gcd[j].gd.pos.y = y;
    gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j++].creator = GLabelCreate;
    y += 13;
    varray[k++] = &gcd[j-1]; varray[k++] = NULL;

    label[j].text = GenText(sizes,1.);
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 15; gcd[j].gd.pos.y = y;
    gcd[j].gd.pos.width = 170;
    gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    gcd[j].gd.cid = CID_Pixel;
    gcd[j].gd.handle_controlevent = CB_TextChange;
    gcd[j++].creator = GTextFieldCreate;
    y += 26;
    harray[4][0] = GCD_HPad10; harray[4][1] = &gcd[j-1]; harray[4][2] = NULL;

    boxes[6].gd.flags = gg_enabled|gg_visible;
    boxes[6].gd.u.boxelements = harray[4];
    boxes[6].creator = GHBoxCreate;
    varray[k++] = &boxes[6]; varray[k++] = NULL;

    label[j].text = (unichar_t *) _("Use FreeType");
    label[j].text_is_1byte = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.pos.x = 10; gcd[j].gd.pos.y = y;
    if ( !hasFreeType() || bd.sf->onlybitmaps)
	gcd[j].gd.flags = gg_visible;
    else if ( oldusefreetype )
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
    else
	gcd[j].gd.flags = gg_enabled|gg_visible;
    gcd[j].gd.cid = CID_FreeType;
    gcd[j].gd.handle_controlevent = CB_SystemChange;
    gcd[j++].creator = GCheckBoxCreate;
    y += 26;
    varray[k++] = &gcd[j-1]; varray[k++] = NULL;

    if ( isavail==1 ) {
	label[j].text = (unichar_t *) _("Create Rasterized Strikes (Not empty ones)");
	label[j].text_is_1byte = true;
	gcd[j].gd.label = &label[j];
	gcd[j].gd.pos.x = 10; gcd[j].gd.pos.y = y;
	gcd[j].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[j].gd.cid = CID_RasterizedStrikes;
	gcd[j++].creator = GCheckBoxCreate;
	y += 26;
	varray[k++] = &gcd[j-1]; varray[k++] = NULL;
    }
    varray[k++] = GCD_Glue; varray[k++] = NULL;

    gcd[j].gd.pos.x = 2; gcd[j].gd.pos.y = 2;
    gcd[j].gd.pos.width = pos.width-4;
    gcd[j].gd.pos.height = pos.height-4;
    gcd[j].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[j++].creator = GGroupCreate;

    gcd[j].gd.pos.x = 20-3; gcd[j].gd.pos.y = 252-32-3;
    gcd[j].gd.pos.width = -1; gcd[j].gd.pos.height = 0;
    gcd[j].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[j].text = (unichar_t *) _("_OK");
    label[j].text_is_1byte = true;
    label[j].text_in_resource = true;
    gcd[j].gd.mnemonic = 'O';
    gcd[j].gd.label = &label[j];
    gcd[j].gd.handle_controlevent = CB_OK;
    gcd[j++].creator = GButtonCreate;
    harray[5][0] = GCD_Glue; harray[5][1] = &gcd[j-1]; harray[5][2] = GCD_Glue;

    gcd[j].gd.pos.x = -20; gcd[j].gd.pos.y = 252-32;
    gcd[j].gd.pos.width = -1; gcd[j].gd.pos.height = 0;
    gcd[j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[j].text = (unichar_t *) _("_Cancel");
    label[j].text_is_1byte = true;
    label[j].text_in_resource = true;
    gcd[j].gd.label = &label[j];
    gcd[j].gd.mnemonic = 'C';
    gcd[j].gd.handle_controlevent = CB_Cancel;
    gcd[j++].creator = GButtonCreate;
    harray[5][3] = GCD_Glue; harray[5][4] = &gcd[j-1]; harray[5][5] = GCD_Glue;
    harray[5][6] = NULL;

    boxes[7].gd.flags = gg_enabled|gg_visible;
    boxes[7].gd.u.boxelements = harray[5];
    boxes[7].creator = GHBoxCreate;
    varray[k++] = &boxes[7]; varray[k++] = NULL;
    varray[k] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(bd.gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    if ( isavail<true )
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[7].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    which[lastwhich].selected = false;
    _CB_SystemChange(&bd);

    GWidgetHidePalettes();
    GDrawSetVisible(bd.gw,true);
    while ( !bd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(bd.gw);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

int BitmapControl(FontView *fv,int32 *sizes,int isavail,int rasterize) {
    CreateBitmapData bd;

    memset(&bd,0,sizeof(bd));
    bd.fv = fv;
    bd.sf = fv->sf;
    bd.isavail = isavail;
    bd.which = bd_selected;
    bd.rasterize = rasterize;
    BitmapsDoIt(&bd,sizes,hasFreeType());
return( bd.done );
}
