/* Copyright (C) 2000-2002 by George Williams */
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
#include <ustring.h>
#include <gkeysym.h>
#include <math.h>

typedef struct strokedlg {
    int done;
    GWindow gw;
    CharView *cv;
    void (*strokeit)(void *,StrokeInfo *);
} StrokeDlg;

#define CID_ButtCap	1001
#define CID_RoundCap	1002
#define CID_SquareCap	1003
#define CID_BevelJoin	1004
#define CID_RoundJoin	1005
#define CID_MiterJoin	1006
#define CID_Width	1007
#define CID_Stroke	1008
#define CID_Caligraphic	1009
#define CID_PenAngle	1010
#define CID_PenAngleTxt	1011
#define CID_ThicknessRatio	1012
#define CID_ThicknessRatioTxt	1013
#define CID_LineCapTxt	1014
#define CID_LineJoinTxt	1015

static void CVStrokeIt(void *_cv, StrokeInfo *si) {
    CharView *cv = _cv;
    int anypoints;
    SplineSet *spl, *prev, *head=NULL, *last=NULL, *cur, *snext;

    CVPreserveState(cv);
    if ( CVAnySel(cv,&anypoints,NULL,NULL) && anypoints ) {
	prev = NULL;
	for ( spl= *cv->heads[cv->drawmode]; spl!=NULL; spl = snext ) {
	    snext = spl->next;
	    if ( PointListIsSelected(spl)) {
		cur = SplineSetStroke(spl,si,cv->sc);
		if ( prev==NULL )
		    *cv->heads[cv->drawmode]=cur;
		else
		    prev->next = cur;
		while ( cur->next ) cur=cur->next;
		cur->next = snext;
		spl->next = NULL;
		SplinePointListMDFree(cv->sc,spl);
		prev = cur;
	    } else
		prev = spl;
	}
    } else {
	for ( spl= *cv->heads[cv->drawmode]; spl!=NULL; spl = spl->next ) {
	    cur = SplineSetStroke(spl,si,cv->sc);
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    while ( cur->next!=NULL ) cur = cur->next;
	    last = cur;
	}
	SplinePointListsFree( *cv->heads[cv->drawmode] );
	*cv->heads[cv->drawmode] = head;
    }
    CVCharChangedUpdate(cv);
}

static void SCStrokeIt(void *_sc, StrokeInfo *si) {
    SplineChar *sc = _sc;
    SplineSet *spl, *head=NULL, *last=NULL, *cur;

    SCPreserveState(sc,false);
    for ( spl= sc->splines; spl!=NULL; spl = spl->next ) {
	cur = SplineSetStroke(spl,si,sc);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	while ( cur->next!=NULL ) cur = cur->next;
	last = cur;
    }
    SplinePointListsFree( sc->splines );
    sc->splines = head;
    SCCharChangedUpdate(sc);
}

static void FVStrokeIt(void *_fv, StrokeInfo *si) {
    FontView *fv = _fv;
    SplineSet *spl, *head, *last, *cur;
    int i, cnt=0;
    static unichar_t stroke[] = { 'S','t','r','o','k','i','n','g',' ','.','.','.',  '\0' };

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] )
	++cnt;
    GProgressStartIndicator(10,stroke,stroke,NULL,cnt,1);

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] ) {
	SplineChar *sc = fv->sf->chars[i];
	SCPreserveState(sc,false);
	head = NULL; last = NULL;
	for ( spl= sc->splines; spl!=NULL; spl = spl->next ) {
	    cur = SplineSetStroke(spl,si,sc);
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    while ( cur->next!=NULL ) cur = cur->next;
	    last = cur;
	}
	SplinePointListsFree( sc->splines );
	sc->splines = head;
	SCCharChangedUpdate(sc);
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static int Stroke_OK(GGadget *g, GEvent *e) {
    StrokeInfo si;
    int err;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow sw = GGadgetGetWindow(g);
	StrokeDlg *sd = GDrawGetUserData(sw);

	memset(&si,'\0',sizeof(si));
	si.cap = GGadgetIsChecked( GWidgetGetControl(sw,CID_ButtCap))?lc_butt:
		GGadgetIsChecked( GWidgetGetControl(sw,CID_RoundCap))?lc_round:
		lc_square;
	si.join = GGadgetIsChecked( GWidgetGetControl(sw,CID_BevelJoin))?lj_bevel:
		GGadgetIsChecked( GWidgetGetControl(sw,CID_RoundJoin))?lj_round:
		lj_miter;
	si.caligraphic = GGadgetIsChecked( GWidgetGetControl(sw,CID_Caligraphic));
	err = false;
	si.radius = GetRealR(sw,CID_Width,_STR_StrokeWidth,&err)/2;
	if ( si.caligraphic ) {
	    si.penangle = GetRealR(sw,CID_PenAngle,_STR_PenAngle,&err);
	    if ( si.penangle>180 || si.penangle < -180 ) {
		si.penangle = fmod(si.penangle,360);
		if ( si.penangle>180 )
		    si.penangle -= 360;
		else if ( si.penangle<-180 )
		    si.penangle += 360;
	    }
	    si.penangle *= 3.1415926535897932/180;
	    si.cap = lc_butt;
	    si.join = lj_bevel;
	    si.thickness = GetRealR(sw,CID_ThicknessRatio,_STR_PenHeightRatio,&err)*si.radius;
	    si.s = sin(si.penangle);
	    si.c = cos(si.penangle);
	    si.xoff[0] = si.xoff[4] = si.radius*si.c + si.thickness*si.s;
	    si.yoff[0] = si.yoff[4] = -si.thickness*si.c + si.radius*si.s;
	    si.xoff[1] = si.xoff[5] = si.radius*si.c - si.thickness*si.s;
	    si.yoff[1] = si.yoff[5] = si.thickness*si.c + si.radius*si.s;
	    si.xoff[2] = si.xoff[6] = -si.radius*si.c - si.thickness*si.s;
	    si.yoff[2] = si.yoff[6] = si.thickness*si.c - si.radius*si.s;
	    si.xoff[3] = si.xoff[7] = -si.radius*si.c + si.thickness*si.s;
	    si.yoff[3] = si.yoff[7] = -si.thickness*si.c - si.radius*si.s;
	}
	if ( err )
return( true );
	(sd->strokeit)(sd->cv,&si);
	sd->done = true;
    }
return( true );
}

static int Stroke_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	StrokeDlg *sd = GDrawGetUserData(GGadgetGetWindow(g));
	sd->done = true;
    }
return( true );
}

static void StrokeSetup(StrokeDlg *sd, int calig) {
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_LineCapTxt), !calig);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_ButtCap), !calig);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_RoundCap), !calig);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_SquareCap), !calig);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_LineJoinTxt), !calig);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_BevelJoin), !calig);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_RoundJoin), !calig);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_MiterJoin), !calig);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PenAngle), calig);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PenAngleTxt), calig);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_ThicknessRatio), calig);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_ThicknessRatioTxt), calig);
}

static int Stroke_Caligraphic(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StrokeDlg *sd = GDrawGetUserData(GGadgetGetWindow(g));
	StrokeSetup(sd,true);
    }
return( true );
}

static int Stroke_Stroke(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StrokeDlg *sd = GDrawGetUserData(GGadgetGetWindow(g));
	StrokeSetup(sd,false);
    }
return( true );
}

static int stroke_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	StrokeDlg *sd = GDrawGetUserData(gw);
	sd->done = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("elementmenu.html#Expand");
return( true );
	}
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

#define SD_Width	230
#define SD_Height	226

static void MakeStrokeDlg(void *cv,void (*strokeit)(void *,StrokeInfo *)) {
    static StrokeDlg sd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[25];
    GTextInfo label[25];

    if ( sd.gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_Stroke,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,SD_Width));
	pos.height = GDrawPointsToPixels(NULL,SD_Height);
	sd.gw = gw = GDrawCreateTopWindow(NULL,&pos,stroke_e_h,&sd,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) _STR_LineCap;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 23;
	gcd[0].gd.flags = gg_enabled | gg_visible;
	gcd[0].gd.cid = CID_LineCapTxt;
	gcd[0].creator = GLabelCreate;

	gcd[1].gd.pos.x = 6; gcd[1].gd.pos.y = gcd[0].gd.pos.y+6;
	gcd[1].gd.pos.width = SD_Width-12; gcd[1].gd.pos.height = 25;
	gcd[1].gd.flags = gg_enabled | gg_visible;
	gcd[1].creator = GGroupCreate;

	label[2].text = (unichar_t *) _STR_Butt;
	label[2].text_in_resource = true;
	label[2].image = &GIcon_buttcap;
	gcd[2].gd.mnemonic = 'B';
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 15; gcd[2].gd.pos.y = gcd[0].gd.pos.y+12;
	gcd[2].gd.flags = gg_enabled | gg_visible | gg_cb_on;
	gcd[2].gd.cid = CID_ButtCap;
	gcd[2].creator = GRadioCreate;

	label[3].text = (unichar_t *) _STR_Round;
	label[3].text_in_resource = true;
	label[3].image = &GIcon_roundcap;
	gcd[3].gd.mnemonic = 'R';
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 80; gcd[3].gd.pos.y = gcd[2].gd.pos.y;
	gcd[3].gd.flags = gg_enabled | gg_visible;
	gcd[3].gd.cid = CID_RoundCap;
	gcd[3].creator = GRadioCreate;

	label[4].text = (unichar_t *) _STR_Squareq;
	label[4].text_in_resource = true;
	label[4].image = &GIcon_squarecap;
	gcd[4].gd.mnemonic = 'q';
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = 150; gcd[4].gd.pos.y = gcd[2].gd.pos.y;
	gcd[4].gd.flags = gg_enabled | gg_visible;
	gcd[4].gd.cid = CID_SquareCap;
	gcd[4].creator = GRadioCreate;

	label[5].text = (unichar_t *) _STR_LineJoin;
	label[5].text_in_resource = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.pos.x = gcd[0].gd.pos.x; gcd[5].gd.pos.y = gcd[2].gd.pos.y+25;
	gcd[5].gd.flags = gg_enabled | gg_visible;
	gcd[5].gd.cid = CID_LineJoinTxt;
	gcd[5].creator = GLabelCreate;

	gcd[6].gd.pos.x = gcd[1].gd.pos.x; gcd[6].gd.pos.y = gcd[5].gd.pos.y+6;
	gcd[6].gd.pos.width = SD_Width-12; gcd[6].gd.pos.height = 25;
	gcd[6].gd.flags = gg_enabled | gg_visible;
	gcd[6].creator = GGroupCreate;

	label[7].text = (unichar_t *) _STR_Miter;
	label[7].text_in_resource = true;
	label[7].image = &GIcon_miterjoin;
	gcd[7].gd.mnemonic = 'M';
	gcd[7].gd.label = &label[7];
	gcd[7].gd.pos.x = gcd[2].gd.pos.x; gcd[7].gd.pos.y = gcd[5].gd.pos.y+12;
	gcd[7].gd.flags = gg_enabled | gg_visible;
	gcd[7].gd.cid = CID_MiterJoin;
	gcd[7].creator = GRadioCreate;

	label[8].text = (unichar_t *) _STR_Roundu;
	label[8].text_in_resource = true;
	label[8].image = &GIcon_roundjoin;
	gcd[8].gd.mnemonic = 'u';
	gcd[8].gd.label = &label[8];
	gcd[8].gd.pos.x = gcd[3].gd.pos.x; gcd[8].gd.pos.y = gcd[7].gd.pos.y;
	gcd[8].gd.flags = gg_enabled | gg_visible | gg_cb_on;
	gcd[8].gd.cid = CID_RoundJoin;
	gcd[8].creator = GRadioCreate;

	label[9].text = (unichar_t *) _STR_Bevel;
	label[9].text_in_resource = true;
	label[9].image = &GIcon_beveljoin;
	gcd[9].gd.mnemonic = 'v';
	gcd[9].gd.label = &label[9];
	gcd[9].gd.pos.x = gcd[4].gd.pos.x; gcd[9].gd.pos.y = gcd[7].gd.pos.y;
	gcd[9].gd.flags = gg_enabled | gg_visible;
	gcd[9].gd.cid = CID_BevelJoin;
	gcd[9].creator = GRadioCreate;

	label[10].text = (unichar_t *) _STR_StrokeWidth;
	label[10].text_in_resource = true;
	gcd[10].gd.mnemonic = 'W';
	gcd[10].gd.label = &label[10];
	gcd[10].gd.pos.x = 5; gcd[10].gd.pos.y = 166+3;
	gcd[10].gd.flags = gg_enabled | gg_visible;
	gcd[10].creator = GLabelCreate;

	label[11].text = (unichar_t *) "50";
	label[11].text_is_1byte = true;
	gcd[11].gd.mnemonic = 'W';
	gcd[11].gd.label = &label[11];
	gcd[11].gd.pos.x = 80; gcd[11].gd.pos.y = 166;
	gcd[11].gd.flags = gg_enabled | gg_visible;
	gcd[11].gd.cid = CID_Width;
	gcd[11].creator = GTextFieldCreate;

	gcd[12].gd.pos.x = 30-3; gcd[12].gd.pos.y = SD_Height-30-3;
	gcd[12].gd.pos.width = -1;
	gcd[12].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[12].text = (unichar_t *) _STR_OK;
	label[12].text_in_resource = true;
	gcd[12].gd.mnemonic = 'O';
	gcd[12].gd.label = &label[12];
	gcd[12].gd.handle_controlevent = Stroke_OK;
	gcd[12].creator = GButtonCreate;

	gcd[13].gd.pos.x = -30; gcd[13].gd.pos.y = SD_Height-30;
	gcd[13].gd.pos.width = -1;
	gcd[13].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[13].text = (unichar_t *) _STR_Cancel;
	label[13].text_in_resource = true;
	gcd[13].gd.label = &label[13];
	gcd[13].gd.mnemonic = 'C';
	gcd[13].gd.handle_controlevent = Stroke_Cancel;
	gcd[13].creator = GButtonCreate;

	label[14].text = (unichar_t *) _STR_Strok;
	label[14].text_in_resource = true;
	gcd[14].gd.mnemonic = 'S';
	gcd[14].gd.label = &label[14];
	gcd[14].gd.pos.x = 5; gcd[14].gd.pos.y = 5;
	gcd[14].gd.flags = gg_enabled | gg_visible | gg_cb_on;
	gcd[14].gd.cid = CID_Stroke;
	gcd[14].gd.handle_controlevent = Stroke_Stroke;
	gcd[14].creator = GRadioCreate;

	label[15].text = (unichar_t *) _STR_Caligraphic;
	label[15].text_in_resource = true;
	gcd[15].gd.mnemonic = 'C';
	gcd[15].gd.label = &label[15];
	gcd[15].gd.pos.x = 5; gcd[15].gd.pos.y = gcd[5].gd.pos.y+38;
	gcd[15].gd.flags = gg_enabled | gg_visible;
	gcd[15].gd.cid = CID_Caligraphic;
	gcd[15].gd.handle_controlevent = Stroke_Caligraphic;
	gcd[15].creator = GRadioCreate;

	gcd[16].gd.pos.x = 1; gcd[16].gd.pos.y = gcd[14].gd.pos.y+6;
	gcd[16].gd.pos.width = SD_Width-2; gcd[16].gd.pos.height = 84;
	gcd[16].gd.flags = gg_enabled | gg_visible;
	gcd[16].creator = GGroupCreate;

	gcd[17].gd.pos.x = 1; gcd[17].gd.pos.y = gcd[15].gd.pos.y+6;
	gcd[17].gd.pos.width = SD_Width-2; gcd[17].gd.pos.height = 58;
	gcd[17].gd.flags = gg_enabled | gg_visible;
	gcd[17].creator = GGroupCreate;

	label[18].text = (unichar_t *) _STR_PenAngle;
	label[18].text_in_resource = true;
	gcd[18].gd.mnemonic = 'A';
	gcd[18].gd.label = &label[18];
	gcd[18].gd.pos.x = gcd[2].gd.pos.x; gcd[18].gd.pos.y = gcd[15].gd.pos.y+15+3;
	gcd[18].gd.flags = gg_visible;
	gcd[18].gd.cid = CID_PenAngleTxt;
	gcd[18].creator = GLabelCreate;

	label[19].text = (unichar_t *) "45";
	label[19].text_is_1byte = true;
	gcd[19].gd.mnemonic = 'A';
	gcd[19].gd.label = &label[19];
	gcd[19].gd.pos.x = 80; gcd[19].gd.pos.y = gcd[15].gd.pos.y+15;
	gcd[19].gd.flags = gg_visible;
	gcd[19].gd.cid = CID_PenAngle;
	gcd[19].creator = GTextFieldCreate;

	label[20].text = (unichar_t *) _STR_PenHeightRatio;
	label[20].text_in_resource = true;
	gcd[20].gd.mnemonic = 'H';
	gcd[20].gd.label = &label[20];
	gcd[20].gd.pos.x = gcd[2].gd.pos.x; gcd[20].gd.pos.y = gcd[18].gd.pos.y+24;
	gcd[20].gd.flags = gg_visible;
	gcd[20].gd.cid = CID_ThicknessRatioTxt;
	gcd[20].gd.popup_msg = GStringGetResource(_STR_PenHeightRatioPopup,NULL);
	gcd[20].creator = GLabelCreate;

	label[21].text = (unichar_t *) ".2";
	label[21].text_is_1byte = true;
	gcd[21].gd.mnemonic = 'H';
	gcd[21].gd.label = &label[21];
	gcd[21].gd.pos.x = gcd[19].gd.pos.x; gcd[21].gd.pos.y = gcd[19].gd.pos.y+24;
	gcd[21].gd.flags = gg_visible;
	gcd[21].gd.cid = CID_ThicknessRatio;
	gcd[21].gd.popup_msg = GStringGetResource(_STR_PenHeightRatioPopup,NULL);
	gcd[21].creator = GTextFieldCreate;

	GGadgetsCreate(gw,gcd);
    }

    sd.cv = cv;
    sd.strokeit = strokeit;
    sd.done = false;
    GWidgetHidePalettes();
    GWidgetIndicateFocusGadget(GWidgetGetControl(sd.gw,CID_Width));
    GDrawSetVisible(sd.gw,true);
    while ( !sd.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(sd.gw,false);
}

void CVStroke(CharView *cv) {

    if ( *cv->heads[cv->drawmode]==NULL )
return;

    MakeStrokeDlg(cv,CVStrokeIt);
}

void SCStroke(SplineChar *sc) {
    MakeStrokeDlg(sc,SCStrokeIt);
}

void FVStroke(FontView *fv) {
    MakeStrokeDlg(fv,FVStrokeIt);
}

void FVStrokeItScript(FontView *fv, StrokeInfo *si) {
    FVStrokeIt(fv, si);
}
