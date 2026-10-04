/* Copyright (C) 2001-2012 by George Williams */
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

#include <fontforge-config.h>

#include "cvundoes.h"
#include "fffreetype.h"
#include "fontforgeui.h"
#include "gkeysym.h"
#include "splineutil.h"
#include "ustring.h"

#include <math.h>

int gridfit_dpi=72, gridfit_depth=1; float gridfit_pointsizey=12, gridfit_pointsizex=12;
int gridfit_x_sameas_y = true;

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

void CVGridHandlePossibleFitChar(CharView *cv)
{
    if( cv->show_ft_results_live_update )
	CVGridFitChar(cv);
}

void CVGridFitChar(CharView *cv) {
    void *single_glyph_context;
    SplineFont *sf = cv->b.sc->parent;
    int layer = CVLayer((CharViewBase *) cv);

    SplinePointListsFree(cv->b.gridfit); cv->b.gridfit = NULL;
    FreeType_FreeRaster(cv->raster); cv->raster = NULL;

    single_glyph_context = _FreeTypeFontContext(sf,cv->b.sc,NULL,layer,
	    sf->layers[layer].order2?ff_ttf:ff_otf,0,NULL);
    if ( single_glyph_context==NULL ) {
	LogError(_("Freetype rasterization failed.") );
return;
    }

    if ( cv->b.sc->layers[layer].refs!=NULL )
	SCNumberPoints(cv->b.sc,layer);

    cv->raster = FreeType_GetRaster(single_glyph_context,cv->b.sc->orig_pos,
	    cv->ft_pointsizey, cv->ft_pointsizex, cv->ft_dpi, cv->ft_depth );
    cv->b.gridfit = FreeType_GridFitChar(single_glyph_context,cv->b.sc->orig_pos,
	    cv->ft_pointsizey, cv->ft_pointsizex, cv->ft_dpi, &cv->b.ft_gridfitwidth,
	    cv->b.sc, cv->ft_depth, true );

    FreeTypeFreeContext(single_glyph_context);
    GDrawRequestExpose(cv->v,NULL,false);
    if ( cv->b.sc->instructions_out_of_date && cv->b.sc->ttf_instrs_len!=0 )
	ff_post_notice(_("Instructions out of date"),
	    _("The points have been changed. This may mean that the truetype instructions now refer to the wrong points and they may cause unexpected results."));
}

void SCReGridFit(SplineChar *sc,int layer) {
    CharView *cv;

    for ( cv=(CharView *) (sc->views); cv!=NULL; cv=(CharView *) (cv->b.next) ) if ( cv->show_ft_results ) {
	if ( cv->show_ft_results && CVLayer((CharViewBase *) cv)==layer ) {
	    SplinePointListsFree(cv->b.gridfit); cv->b.gridfit = NULL;
	    FreeType_FreeRaster(cv->raster); cv->raster = NULL;
	    CVGridFitChar(cv);
	}
    }
}

#define CID_PointSize	1001
#define CID_DPI		1002
#define CID_Debugfpgm	1004
#define CID_BW		1005
#define CID_SameAs	1006
#define CID_PointSizeX	1007

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
	real ptsize, ptsizex;
	int err = 0;
	CharView *cv = fsd->cv;

	ptsize = GetReal8(fsd->gw,CID_PointSize,_("Pointsize Y"),&err);
	if ( GGadgetIsChecked(GWidgetGetControl(fsd->gw,CID_SameAs)) )
	    ptsizex = ptsize;
	else
	    ptsizex = GetReal8(fsd->gw,CID_PointSizeX,_("Pointsize X"),&err);
	_dpi = GetInt8(fsd->gw,CID_DPI,_("DPI"),&err);
	_depth = GGadgetIsChecked(GWidgetGetControl(fsd->gw,CID_BW)) ? 1 : 8;
	if ( err )
return(true);

	last_fpgm = GGadgetIsChecked(GWidgetGetControl(fsd->gw,CID_Debugfpgm));
	cv->ft_pointsizey = ptsize; cv->ft_dpi = _dpi; cv->ft_depth = _depth;
	cv->ft_pointsizex = ptsizex;
	cv->ft_ppemy = rint(cv->ft_pointsizey*cv->ft_dpi/72.0);
	cv->ft_ppemx = rint(cv->ft_pointsizex*cv->ft_dpi/72.0);

	gridfit_dpi = _dpi; gridfit_pointsizey = ptsize; gridfit_depth = _depth;
	gridfit_pointsizex = ptsizex; gridfit_x_sameas_y = GGadgetIsChecked(GWidgetGetControl(fsd->gw,CID_SameAs));
	SavePrefs(true);

	SplinePointListsFree(cv->b.gridfit); cv->b.gridfit = NULL;
	FreeType_FreeRaster(cv->raster); cv->raster = NULL;

	if ( fsd->debug )
	    CVDebugReInit(cv,true,last_fpgm);
	else {
	    cv->show_ft_results = true;
	    CVGridFitChar(cv);
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

static int FtPpem_SameAsChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	FtSizeData *fsd = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(g)) {
	    const unichar_t *y = _GGadgetGetTitle(GWidgetGetControl(fsd->gw,CID_PointSize));
	    GGadgetSetTitle(GWidgetGetControl(fsd->gw,CID_PointSizeX),y);
	    GGadgetSetEnabled(GWidgetGetControl(fsd->gw,CID_PointSizeX),false);
	} else {
	    GGadgetSetEnabled(GWidgetGetControl(fsd->gw,CID_PointSizeX),true);
	}
    }
return( true );
}

static int FtPpem_PtYChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	FtSizeData *fsd = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(fsd->gw,CID_SameAs))) {
	    const unichar_t *y = _GGadgetGetTitle(g);
	    GGadgetSetTitle(GWidgetGetControl(fsd->gw,CID_PointSizeX),y);
	}
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
    GGadgetCreateData gcd[16], boxes[8];
    GTextInfo label[16];
    FtSizeData fsd;
    char buffer[20], buffer2[20], buffer3[20];
    GGadgetCreateData *varray[7][4], *barray[9], *harray1[3], *harray2[3], *harray3[3];
    int k,r;

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
    memset(&boxes,0,sizeof(boxes));

    k=r=0;
    label[k].text = (unichar_t *) _("Debug _fpgm/prep");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = 4; 
    gcd[k].gd.flags = debug ? (gg_enabled|gg_visible) : 0;
    if ( last_fpgm )
	gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Debugfpgm;
    gcd[k++].creator = GCheckBoxCreate;
    varray[r][0] = &gcd[k-1]; varray[r][1] = GCD_ColSpan; varray[r][2] = GCD_ColSpan; varray[r++][3] = NULL;

    label[k].text = (unichar_t *) _("Scale X/Y the same");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 110; gcd[k].gd.pos.y = 17+5+6; 
    gcd[k].gd.flags = gg_enabled|gg_visible;
    if ( gridfit_x_sameas_y )
	gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_SameAs;
    gcd[k].gd.handle_controlevent = FtPpem_SameAsChanged;
    gcd[k++].creator = GCheckBoxCreate;
    varray[r][0] = &gcd[k-1]; varray[r][1] = GCD_HPad10;

    label[k].text = (unichar_t *) _("_DPI:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 110; gcd[k].gd.pos.y = 17+5+6; 
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;
    harray1[0] = &gcd[k-1];

    sprintf( buffer2, "%d", gridfit_dpi );
    label[k].text = (unichar_t *) buffer2;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 140; gcd[k].gd.pos.y = 17+5;  gcd[k].gd.pos.width = 40;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_DPI;
    gcd[k++].creator = GTextFieldCreate;
    harray1[1] = &gcd[k-1]; harray1[2] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray1;
    boxes[2].creator = GHBoxCreate;
    varray[r][2] = &boxes[2]; varray[r++][3] = NULL;

    label[k].text = (unichar_t *) _("_Pointsize Y:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 17+5+6; 
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;
    harray2[0] = &gcd[k-1];

    sprintf( buffer, "%g", gridfit_pointsizey );
    label[k].text = (unichar_t *) buffer;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 57; gcd[k].gd.pos.y = 17+5;  gcd[k].gd.pos.width = 40;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.handle_controlevent = FtPpem_PtYChanged;
    gcd[k].gd.cid = CID_PointSize;
    gcd[k++].creator = GTextFieldCreate;
    harray2[1] = &gcd[k-1]; harray2[2] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray2;
    boxes[3].creator = GHBoxCreate;
    varray[r][0] = &boxes[3]; varray[r][1] = GCD_HPad10;

    label[k].text = (unichar_t *) _("_X:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 17+5+6; 
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;
    harray3[0] = &gcd[k-1];

    sprintf( buffer3, "%g", gridfit_x_sameas_y ? gridfit_pointsizey : gridfit_pointsizex);
    label[k].text = (unichar_t *) buffer3;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 57; gcd[k].gd.pos.y = 17+5;  gcd[k].gd.pos.width = 40;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    if ( gridfit_x_sameas_y )
	gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_PointSizeX;
    gcd[k++].creator = GTextFieldCreate;
    harray3[1] = &gcd[k-1]; harray3[2] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = harray3;
    boxes[4].creator = GHBoxCreate;
    varray[r][2] = &boxes[4]; varray[r++][3] = NULL;

    label[k].text = (unichar_t *) _("_Mono");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 20; gcd[k].gd.pos.y = 14+31; 
    gcd[k].gd.flags = gridfit_depth==1 ? (gg_enabled|gg_visible|gg_cb_on) : (gg_enabled|gg_visible);
    gcd[k].gd.cid = CID_BW;
    gcd[k++].creator = GRadioCreate;
    varray[r][0] = &gcd[k-1]; varray[r][1] = GCD_HPad10;

    label[k].text = (unichar_t *) _("_Anti-Aliased");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[9].gd.pos.y; 
    gcd[k].gd.flags = gridfit_depth!=1 ? (gg_enabled|gg_visible|gg_cb_on) : (gg_enabled|gg_visible);
    gcd[k++].creator = GRadioCreate;
    varray[r][2] = &gcd[k-1]; varray[r++][3] = NULL;

    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = 17+31+16;
    gcd[k].gd.pos.width = 190-10;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLineCreate;
    varray[r][0] = &gcd[k-1]; varray[r][1] = GCD_ColSpan; varray[r][2] = GCD_ColSpan; varray[r++][3] = NULL;

    gcd[k].gd.pos.x = 20-3; gcd[k].gd.pos.y = 17+37+16;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.mnemonic = 'O';
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = FtPpem_OK;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[k-1]; barray[2] = GCD_Glue;

    gcd[k].gd.pos.x = -20; gcd[k].gd.pos.y = gcd[4].gd.pos.y+3;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.mnemonic = 'C';
    gcd[k].gd.handle_controlevent = FtPpem_Cancel;
    gcd[k++].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[k-1]; barray[5] = GCD_Glue; barray[6] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = barray;
    boxes[5].creator = GHBoxCreate;
    varray[r][0] = &boxes[5]; varray[r][1] = GCD_ColSpan; varray[r][2] = GCD_ColSpan; varray[r++][3] = NULL;
    varray[r][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray[0];
    boxes[0].creator = GHVGroupCreate;


    GGadgetsCreate(gw,boxes);
    GHVBoxFitWindow(boxes[0].ret);

    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_PointSize));
    GTextFieldSelect(GWidgetGetControl(gw,CID_PointSize),0,-1);

    GDrawSetVisible(gw,true);
    while ( !fsd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
