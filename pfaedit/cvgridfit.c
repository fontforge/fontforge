/* Copyright (C) 2001-2003 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * dercved from this software without specific prior written permission.

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
#include <ustring.h>
#include <gkeysym.h>
#include <math.h>

void SCDeGridFit(SplineChar *sc) {
    CharView *cv;

    for ( cv=sc->views; cv!=NULL; cv=cv->next ) if ( cv->show_ft_results ) {
	SplinePointListsFree(cv->gridfit); cv->gridfit = NULL;
	FreeType_FreeRaster(cv->raster); cv->raster = NULL;
	cv->show_ft_results = false;
	GDrawRequestExpose(cv->v,NULL,false);
    }
}

void CVGridFitChar(CharView *cv) {
    void *single_glyph_context;
    SplineFont *sf = cv->sc->parent;

    SplinePointListsFree(cv->gridfit); cv->gridfit = NULL;
    FreeType_FreeRaster(cv->raster); cv->raster = NULL;

    single_glyph_context = _FreeTypeFontContext(sf,cv->sc,NULL,
	    sf->order2?ff_ttf:ff_otf,0,NULL);
    if ( single_glyph_context==NULL ) {
	fprintf( stderr,"Freetype rasterization failed.\n" );
return;
    }

    cv->raster = FreeType_GetRaster(single_glyph_context,cv->sc->enc,
	    cv->ft_pointsize, cv->ft_dpi );
    cv->gridfit = FreeType_GridFitChar(single_glyph_context,cv->sc->enc,
	    cv->ft_pointsize, cv->ft_dpi, &cv->ft_gridfitwidth,
	    cv->sc->splines );


    FreeTypeFreeContext(single_glyph_context);
    GDrawRequestExpose(cv->v,NULL,false);
}

#define CID_PointSize	1001
#define CID_DPI		1002
#define CID_ShowGrid	1003

typedef struct ftsizedata {
    unsigned int done: 1;
    CharView *cv;
    GWindow gw;
} FtSizeData;

static int FtPpem_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	FtSizeData *fsd = GDrawGetUserData(GGadgetGetWindow(g));
	int dpi;
	real ptsize;
	int err = 0;
	CharView *cv = fsd->cv;

	ptsize = GetRealR(fsd->gw,CID_PointSize,_STR_Pointsize,&err);
	dpi = GetIntR(fsd->gw,CID_DPI,_STR_DPI,&err);
	if ( err )
return(true);

	cv->show_ft_results = GGadgetIsChecked(GWidgetGetControl(fsd->gw,CID_ShowGrid));
	cv->ft_pointsize = ptsize; cv->ft_dpi = dpi;
	cv->ft_ppem = rint(cv->ft_pointsize*cv->ft_dpi/72.0);

	CVLayersSet(cv);
	if ( cv->show_ft_results )
	    CVGridFitChar(cv);
	else {
	    SplinePointListsFree(cv->gridfit); cv->gridfit = NULL;
	    FreeType_FreeRaster(cv->raster); cv->raster = NULL;
	    GDrawRequestExpose(cv->v,NULL,false);
	}
	fsd->done = true;
    }
return( true );
}

static int FtPpem_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	FtSizeData *fsd = GDrawGetUserData(GGadgetGetWindow(g));
	fsd->done = true;
    }
return( true );
}

static int fsd_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	FtSizeData *hd = GDrawGetUserData(gw);
	hd->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void CVFtPpemDlg(CharView *cv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9];
    GTextInfo label[9];
    FtSizeData fsd;
    char buffer[20], buffer2[20];

    memset(&fsd,0,sizeof(fsd));
    fsd.cv = cv;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_FreeTypeParams,NULL);
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,190));
    pos.height = GDrawPointsToPixels(NULL,90);
    fsd.gw = gw = GDrawCreateTopWindow(NULL,&pos,fsd_e_h,&fsd,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_Pointsize;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 17+5+6; 
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    sprintf( buffer, "%g", cv->ft_pointsize );
    label[1].text = (unichar_t *) buffer;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 57; gcd[1].gd.pos.y = 17+5;  gcd[1].gd.pos.width = 40;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_PointSize;
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) _STR_DPI;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 110; gcd[2].gd.pos.y = 17+5+6; 
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    sprintf( buffer2, "%d", cv->ft_dpi );
    label[3].text = (unichar_t *) buffer2;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 140; gcd[3].gd.pos.y = 17+5;  gcd[3].gd.pos.width = 40;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_DPI;
    gcd[3].creator = GTextFieldCreate;

    gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = 17+37;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[4].text = (unichar_t *) _STR_OK;
    label[4].text_in_resource = true;
    gcd[4].gd.mnemonic = 'O';
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = FtPpem_OK;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = -20; gcd[5].gd.pos.y = 17+37+3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[5].text = (unichar_t *) _STR_Cancel;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.mnemonic = 'C';
    gcd[5].gd.handle_controlevent = FtPpem_Cancel;
    gcd[5].creator = GButtonCreate;

    label[6].text = (unichar_t *) _STR_ShowGridFit;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 17; gcd[6].gd.pos.y = 5; 
    gcd[6].gd.flags = gg_enabled|gg_visible;
    if ( !cv->show_ft_results )
	gcd[6].gd.flags |= gg_cb_on;
    gcd[6].gd.cid = CID_ShowGrid;
    gcd[6].creator = GCheckBoxCreate;

    gcd[7].gd.pos.x = 5; gcd[7].gd.pos.y = 17+31;
    gcd[7].gd.pos.width = 190-10;
    gcd[7].gd.flags = gg_enabled|gg_visible;
    gcd[7].creator = GLineCreate;

    GGadgetsCreate(gw,gcd);

    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_PointSize));
    GTextFieldSelect(GWidgetGetControl(gw,CID_PointSize),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !fsd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
