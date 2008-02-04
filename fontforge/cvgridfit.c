/* Copyright (C) 2001-2008 by George Williams */
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

int gridfit_dpi=72, gridfit_depth=2; float gridfit_pointsize=12;

static int last_fpgm = false;

void SCDeGridFit(SplineChar *sc) {
    CharView *cv;

    for ( cv=(CharView *) (sc->views); cv!=NULL; cv=(CharView *) (cv->b.next) ) if ( cv->show_ft_results ) {
	SplinePointListsFree(cv->b.gridfit); cv->b.gridfit = NULL;
	FreeType_FreeRaster(cv->raster); cv->raster = NULL;
	cv->show_ft_results = false;
	GDrawRequestExpose(cv->v,NULL,false);
    }
}

void CVGridFitChar(CharView *cv) {
    void *single_glyph_context;
    SplineFont *sf = cv->b.sc->parent;

    SplinePointListsFree(cv->b.gridfit); cv->b.gridfit = NULL;
    FreeType_FreeRaster(cv->raster); cv->raster = NULL;

    single_glyph_context = _FreeTypeFontContext(sf,cv->b.sc,NULL,
	    sf->layers[ly_fore].order2?ff_ttf:ff_otf,0,NULL);
    if ( single_glyph_context==NULL ) {
	LogError(_("Freetype rasterization failed.\n") );
return;
    }

    if ( cv->b.sc->layers[ly_fore].refs!=NULL )
	SCNumberPoints(cv->b.sc);

    cv->raster = FreeType_GetRaster(single_glyph_context,cv->b.sc->orig_pos,
	    cv->ft_pointsize, cv->ft_dpi, cv->ft_depth );
    cv->b.gridfit = FreeType_GridFitChar(single_glyph_context,cv->b.sc->orig_pos,
	    cv->ft_pointsize, cv->ft_dpi, &cv->b.ft_gridfitwidth,
	    cv->b.sc, cv->ft_depth );

    FreeTypeFreeContext(single_glyph_context);
    GDrawRequestExpose(cv->v,NULL,false);
    if ( cv->b.sc->instructions_out_of_date && cv->b.sc->ttf_instrs_len!=0 )
	ff_post_notice(_("Instructions out of date"),
	    _("The points have been changed. This may mean that the truetype instructions now refer to the wrong points and they may cause unexpected results."));
}

#define CID_PointSize	1001
#define CID_DPI		1002
#define CID_ShowGrid	1003
#define CID_Debugfpgm	1004
#define CID_BW		1005

typedef struct ftsizedata {
    unsigned int done: 1;
    unsigned int debug: 1;
    CharView *cv;
    GWindow gw;
} FtSizeData;

static int FtPpem_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	FtSizeData *fsd = GDrawGetUserData(GGadgetGetWindow(g));
	int _dpi, _depth;
	real ptsize;
	int err = 0, bit;
	CharView *cv = fsd->cv;

	ptsize = GetReal8(fsd->gw,CID_PointSize,_("_Pointsize:"),&err);
	_dpi = GetInt8(fsd->gw,CID_DPI,_("D_PI:"),&err);
	_depth = GGadgetIsChecked(GWidgetGetControl(fsd->gw,CID_BW)) ? 2 : 8;
	if ( err )
return(true);

	bit = GGadgetIsChecked(GWidgetGetControl(fsd->gw,CID_ShowGrid));
	last_fpgm = GGadgetIsChecked(GWidgetGetControl(fsd->gw,CID_Debugfpgm));
	cv->ft_pointsize = ptsize; cv->ft_dpi = _dpi; cv->ft_depth = _depth;
	cv->ft_ppem = rint(cv->ft_pointsize*cv->ft_dpi/72.0);

	gridfit_dpi = _dpi; gridfit_pointsize = ptsize; gridfit_depth = _depth;
	SavePrefs(true);

	SplinePointListsFree(cv->b.gridfit); cv->b.gridfit = NULL;
	FreeType_FreeRaster(cv->raster); cv->raster = NULL;

	if ( fsd->debug )
	    CVDebugReInit(cv,bit,last_fpgm);
	else {
	    cv->show_ft_results = bit;
	    if ( cv->show_ft_results )
		CVGridFitChar(cv);
	    else {
		GDrawRequestExpose(cv->v,NULL,false);
	    }
	}
	CVLayersSet(cv);
	fsd->done = true;
	SCRefreshTitles(cv->b.sc);
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

void CVFtPpemDlg(CharView *cv,int debug) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[12];
    GTextInfo label[12];
    FtSizeData fsd;
    char buffer[20], buffer2[20];

    memset(&fsd,0,sizeof(fsd));
    fsd.cv = cv;
    fsd.debug = debug;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Grid Fit Parameters");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,190));
    pos.height = GDrawPointsToPixels(NULL,106);
    fsd.gw = gw = GDrawCreateTopWindow(NULL,&pos,fsd_e_h,&fsd,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _("_Pointsize:");
    label[0].text_is_1byte = true;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 17+5+6; 
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    sprintf( buffer, "%g", gridfit_pointsize );
    label[1].text = (unichar_t *) buffer;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 57; gcd[1].gd.pos.y = 17+5;  gcd[1].gd.pos.width = 40;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_PointSize;
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) _("_DPI:");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 110; gcd[2].gd.pos.y = 17+5+6; 
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    sprintf( buffer2, "%d", gridfit_dpi );
    label[3].text = (unichar_t *) buffer2;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 140; gcd[3].gd.pos.y = 17+5;  gcd[3].gd.pos.width = 40;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_DPI;
    gcd[3].creator = GTextFieldCreate;

    gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = 17+37+16;
    gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[4].text = (unichar_t *) _("_OK");
    label[4].text_is_1byte = true;
    label[4].text_in_resource = true;
    gcd[4].gd.mnemonic = 'O';
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = FtPpem_OK;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = -20; gcd[5].gd.pos.y = gcd[4].gd.pos.y+3;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[5].text = (unichar_t *) _("_Cancel");
    label[5].text_is_1byte = true;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.mnemonic = 'C';
    gcd[5].gd.handle_controlevent = FtPpem_Cancel;
    gcd[5].creator = GButtonCreate;

    label[6].text = (unichar_t *) (debug ? _("_Debug") : _("Show _Grid Fit"));
    label[6].text_is_1byte = true;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 17; gcd[6].gd.pos.y = 4; 
    gcd[6].gd.flags = gg_enabled|gg_visible;
    if ( !cv->show_ft_results || debug )
	gcd[6].gd.flags |= gg_cb_on;
    gcd[6].gd.cid = CID_ShowGrid;
    gcd[6].creator = GCheckBoxCreate;

    label[7].text = (unichar_t *) _("Debug _fpgm/prep");
    label[7].text_is_1byte = true;
    label[7].text_in_resource = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = 80; gcd[7].gd.pos.y = 4; 
    gcd[7].gd.flags = debug ? (gg_enabled|gg_visible) : 0;
    if ( last_fpgm )
	gcd[7].gd.flags |= gg_cb_on;
    gcd[7].gd.cid = CID_Debugfpgm;
    gcd[7].creator = GCheckBoxCreate;

    gcd[8].gd.pos.x = 5; gcd[8].gd.pos.y = 17+31+16;
    gcd[8].gd.pos.width = 190-10;
    gcd[8].gd.flags = gg_enabled|gg_visible;
    gcd[8].creator = GLineCreate;

    label[9].text = (unichar_t *) _("_Mono");
    label[9].text_is_1byte = true;
    label[9].text_in_resource = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.pos.x = 20; gcd[9].gd.pos.y = 14+31; 
    gcd[9].gd.flags = gridfit_depth==2 ? (gg_enabled|gg_visible|gg_cb_on) : (gg_enabled|gg_visible);
    gcd[9].gd.cid = CID_BW;
    gcd[9].creator = GRadioCreate;

    label[10].text = (unichar_t *) _("_Anti-Aliased");
    label[10].text_is_1byte = true;
    label[10].text_in_resource = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.pos.x = 80; gcd[10].gd.pos.y = gcd[9].gd.pos.y; 
    gcd[10].gd.flags = gridfit_depth!=2 ? (gg_enabled|gg_visible|gg_cb_on) : (gg_enabled|gg_visible);
    gcd[10].creator = GRadioCreate;

    GGadgetsCreate(gw,gcd);

    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_PointSize));
    GTextFieldSelect(GWidgetGetControl(gw,CID_PointSize),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !fsd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
