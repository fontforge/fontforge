/* Copyright (C) 2000-2003 by George Williams */
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

extern GDevEventMask input_em[];
extern const int input_em_cnt;

typedef struct strokedlg {
    int done;
    GWindow gw;
    CharView *cv;
    void (*strokeit)(void *,StrokeInfo *);
    StrokeInfo *si;
    GRect r1, r2;
    int up[2];
    int dontexpand;
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
	/* For freehand */
#define CID_CenterLine	1016
#define CID_Width2	1017
#define CID_Pressure1	1018
#define CID_Pressure2	1019
#define CID_WidthTxt	1020
#define CID_PressureTxt	1021

static void CVStrokeIt(void *_cv, StrokeInfo *si) {
    CharView *cv = _cv;
    int anypoints;
    SplineSet *spl, *prev, *head=NULL, *last=NULL, *cur, *snext;

    CVPreserveState(cv);
    if ( CVAnySel(cv,&anypoints,NULL,NULL,NULL) && anypoints ) {
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
    StrokeInfo *si, strokeinfo;
    int err;
    real r2;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow sw = GGadgetGetWindow(g);
	StrokeDlg *sd = GDrawGetUserData(sw);

	err = false;
	if ( (si = sd->si)==NULL ) {
	    memset(&strokeinfo,'\0',sizeof(strokeinfo));
	    si = &strokeinfo;
	} else {
	    si->centerline = GGadgetIsChecked( GWidgetGetControl(sw,CID_CenterLine));
	    if ( !si->centerline ) {
		si->pressure1 = GetRealR(sw,CID_Pressure1,_STR_Pressure,&err);
		si->pressure2 = GetRealR(sw,CID_Pressure2,_STR_Pressure,&err);
		if ( si->pressure1!=si->pressure2 )
		    si->radius2 = GetRealR(sw,CID_Width2,_STR_StrokeWidth,&err)/2;
	    }
	}
	si->cap = GGadgetIsChecked( GWidgetGetControl(sw,CID_ButtCap))?lc_butt:
		GGadgetIsChecked( GWidgetGetControl(sw,CID_RoundCap))?lc_round:
		lc_square;
	si->join = GGadgetIsChecked( GWidgetGetControl(sw,CID_BevelJoin))?lj_bevel:
		GGadgetIsChecked( GWidgetGetControl(sw,CID_RoundJoin))?lj_round:
		lj_miter;
	si->caligraphic = GGadgetIsChecked( GWidgetGetControl(sw,CID_Caligraphic));
	si->radius = GetRealR(sw,CID_Width,_STR_StrokeWidth,&err)/2;
	if ( si->caligraphic ) {
	    si->penangle = GetRealR(sw,CID_PenAngle,_STR_PenAngle,&err);
	    if ( si->penangle>180 || si->penangle < -180 ) {
		si->penangle = fmod(si->penangle,360);
		if ( si->penangle>180 )
		    si->penangle -= 360;
		else if ( si->penangle<-180 )
		    si->penangle += 360;
	    }
	    si->penangle *= 3.1415926535897932/180;
	    si->cap = lc_butt;
	    si->join = lj_bevel;
	    si->ratio = GetRealR(sw,CID_ThicknessRatio,_STR_PenHeightRatio,&err);
	    si->s = sin(si->penangle);
	    si->c = cos(si->penangle);
	    r2 = si->ratio*si->radius;
	    si->xoff[0] = si->xoff[4] = si->radius*si->c + r2*si->s;
	    si->yoff[0] = si->yoff[4] = -r2*si->c + si->radius*si->s;
	    si->xoff[1] = si->xoff[5] = si->radius*si->c - r2*si->s;
	    si->yoff[1] = si->yoff[5] = r2*si->c + si->radius*si->s;
	    si->xoff[2] = si->xoff[6] = -si->radius*si->c - r2*si->s;
	    si->yoff[2] = si->yoff[6] = r2*si->c - si->radius*si->s;
	    si->xoff[3] = si->xoff[7] = -si->radius*si->c + r2*si->s;
	    si->yoff[3] = si->yoff[7] = -r2*si->c - si->radius*si->s;
	}
	if ( err )
return( true );
	if ( sd->strokeit!=NULL )
	    (sd->strokeit)(sd->cv,si);
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

static void StrokePressureCheck(StrokeDlg *sd) {
    int err = false;
    real p1, p2;

    p1 = GetRealR(sd->gw,CID_Pressure1,_STR_Pressure,&err);
    p2 = GetRealR(sd->gw,CID_Pressure2,_STR_Pressure,&err);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_Width2),
	    !err && p1!=p2 && !sd->dontexpand);
}

static void StrokeSetup(StrokeDlg *sd, int calig) {

    sd->dontexpand = ( calig==-1 );

    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_LineCapTxt), calig==0);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_ButtCap), calig==0);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_RoundCap), calig==0);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_SquareCap), calig==0);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_LineJoinTxt), calig==0);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_BevelJoin), calig==0);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_RoundJoin), calig==0);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_MiterJoin), calig==0);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PenAngle), calig==1);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PenAngleTxt), calig==1);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_ThicknessRatio), calig==1);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_ThicknessRatioTxt), calig==1);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_WidthTxt), calig!=-1);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_Width), calig!=-1);
    if ( sd->si!=NULL ) {
	StrokePressureCheck(sd);
	GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PressureTxt), calig!=-1);
	GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_Pressure1), calig!=-1);
	GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_Pressure2), calig!=-1);
    }
}

static int Stroke_CenterLine(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StrokeDlg *sd = GDrawGetUserData(GGadgetGetWindow(g));
	StrokeSetup(sd,-1);
    }
return( true );
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

static int Stroke_PressureChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	StrokeDlg *sd = GDrawGetUserData(GGadgetGetWindow(g));
	StrokePressureCheck(sd);
    }
return( true );
}

static void Stroke_PressureSet(StrokeDlg *sd,int cid, GEvent *event) {
    char buff[20];
    unichar_t ubuf[20];
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(sd->gw,cid));
    double old;
    int i = cid==CID_Pressure1?0:1;

    old = u_strtol(ret,NULL,10);
    if ( event->u.mouse.pressure==0 )
	sd->up[i] = true;
    else if ( sd->up[i] || event->u.mouse.pressure>old ) {
	sd->up[i] = false;
	sprintf(buff,"%d",event->u.mouse.pressure);
	uc_strcpy(ubuf,buff);
	GGadgetSetTitle(GWidgetGetControl(sd->gw,cid),ubuf);
	StrokePressureCheck(sd);
    }
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
    } else if ( event->type == et_mousemove ) {
	StrokeDlg *sd = GDrawGetUserData(gw);
	if ( sd->si && (!(event->u.mouse.state&0x0f00) || event->u.mouse.device!=NULL ) &&
		!sd->dontexpand ) {
	    if ( event->u.mouse.y >= sd->r1.y-3 && event->u.mouse.y < sd->r1.y+sd->r1.height+3 )
		GGadgetPreparePopup(gw,GStringGetResource(_STR_PressurePopup,NULL));
	    if ( event->u.mouse.y >= sd->r1.y && event->u.mouse.y < sd->r1.y+sd->r1.height &&
		    event->u.mouse.device!=NULL ) {
		if ( event->u.mouse.x>=sd->r1.x && event->u.mouse.x < sd->r1.x+sd->r1.width )
		    Stroke_PressureSet(sd,CID_Pressure1,event);
		if ( event->u.mouse.x>=sd->r2.x && event->u.mouse.x < sd->r2.x+sd->r2.width )
		    Stroke_PressureSet(sd,CID_Pressure2,event);
	    }
	}
    } else if ( event->type == et_expose ) {
	StrokeDlg *sd = GDrawGetUserData(gw);
	if ( sd->si ) {
	    GDrawDrawRect(gw,&sd->r1,0x000000);
	    GDrawDrawRect(gw,&sd->r2,0x000000);
	}
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

#define SD_Width	230
#define SD_Height	226
#define FH_Height	(SD_Height+75)

static void MakeStrokeDlg(void *cv,void (*strokeit)(void *,StrokeInfo *),StrokeInfo *si) {
    static StrokeDlg strokedlg;
    StrokeDlg *sd, freehand_dlg;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[30];
    GTextInfo label[30];
    int yoff=0;
    int gcdoff;
    static StrokeInfo defaults = { 25, lj_round, lc_butt, false, false, false, 3.1415926535897932/4, .2, 50 };
    StrokeInfo *def = si?si:&defaults;
    char anglebuf[20], ratiobuf[20], widthbuf[20], width2buf[20],
	    pressurebuf[20], pressure2buf[20];

    if ( strokeit!=NULL )
	sd = &strokedlg;
    else {
	sd = &freehand_dlg;
	memset(&freehand_dlg,0,sizeof(freehand_dlg));
	sd->si = si;
	yoff = 18;
    }

    if ( sd->gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(strokeit!=NULL ? _STR_Stroke : _STR_FreeHand,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,SD_Width));
	pos.height = GDrawPointsToPixels(NULL,strokeit!=NULL ? SD_Height : FH_Height);
	sd->gw = gw = GDrawCreateTopWindow(NULL,&pos,stroke_e_h,sd,&wattrs);
	if ( si!=NULL )
	    GDrawRequestDeviceEvents(gw,input_em_cnt,input_em);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) _STR_LineCap;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 23+yoff;
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
	gcd[2].gd.flags = gg_enabled | gg_visible | (def->cap==lc_butt?gg_cb_on:0);
	gcd[2].gd.cid = CID_ButtCap;
	gcd[2].creator = GRadioCreate;

	label[3].text = (unichar_t *) _STR_Round;
	label[3].text_in_resource = true;
	label[3].image = &GIcon_roundcap;
	gcd[3].gd.mnemonic = 'R';
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 80; gcd[3].gd.pos.y = gcd[2].gd.pos.y;
	gcd[3].gd.flags = gg_enabled | gg_visible | (def->cap==lc_round?gg_cb_on:0);
	gcd[3].gd.cid = CID_RoundCap;
	gcd[3].creator = GRadioCreate;

	label[4].text = (unichar_t *) _STR_Squareq;
	label[4].text_in_resource = true;
	label[4].image = &GIcon_squarecap;
	gcd[4].gd.mnemonic = 'q';
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = 150; gcd[4].gd.pos.y = gcd[2].gd.pos.y;
	gcd[4].gd.flags = gg_enabled | gg_visible | (def->cap==lc_square?gg_cb_on:0);
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
	gcd[7].gd.flags = gg_enabled | gg_visible | (def->join==lj_miter?gg_cb_on:0);
	gcd[7].gd.cid = CID_MiterJoin;
	gcd[7].creator = GRadioCreate;

	label[8].text = (unichar_t *) _STR_Roundu;
	label[8].text_in_resource = true;
	label[8].image = &GIcon_roundjoin;
	gcd[8].gd.mnemonic = 'u';
	gcd[8].gd.label = &label[8];
	gcd[8].gd.pos.x = gcd[3].gd.pos.x; gcd[8].gd.pos.y = gcd[7].gd.pos.y;
	gcd[8].gd.flags = gg_enabled | gg_visible | (def->join==lj_round?gg_cb_on:0);
	gcd[8].gd.cid = CID_RoundJoin;
	gcd[8].creator = GRadioCreate;

	label[9].text = (unichar_t *) _STR_Bevel;
	label[9].text_in_resource = true;
	label[9].image = &GIcon_beveljoin;
	gcd[9].gd.mnemonic = 'v';
	gcd[9].gd.label = &label[9];
	gcd[9].gd.pos.x = gcd[4].gd.pos.x; gcd[9].gd.pos.y = gcd[7].gd.pos.y;
	gcd[9].gd.flags = gg_enabled | gg_visible | (def->join==lj_bevel?gg_cb_on:0);
	gcd[9].gd.cid = CID_BevelJoin;
	gcd[9].creator = GRadioCreate;

	gcd[10].gd.pos.x = 30-3; gcd[10].gd.pos.y = (strokeit!=NULL?SD_Height:FH_Height)-30-3;
	gcd[10].gd.pos.width = -1;
	gcd[10].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[10].text = (unichar_t *) _STR_OK;
	label[10].text_in_resource = true;
	gcd[10].gd.mnemonic = 'O';
	gcd[10].gd.label = &label[10];
	gcd[10].gd.handle_controlevent = Stroke_OK;
	gcd[10].creator = GButtonCreate;

	gcd[11].gd.pos.x = -30; gcd[11].gd.pos.y = gcd[10].gd.pos.y+3;
	gcd[11].gd.pos.width = -1;
	gcd[11].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[11].text = (unichar_t *) _STR_Cancel;
	label[11].text_in_resource = true;
	gcd[11].gd.label = &label[11];
	gcd[11].gd.mnemonic = 'C';
	gcd[11].gd.handle_controlevent = Stroke_Cancel;
	gcd[11].creator = GButtonCreate;

	gcdoff = 0;
	if ( strokeit==NULL ) {
	    label[12].text = (unichar_t *) _STR_CenterLine;
	    label[12].text_in_resource = true;
	    gcd[12].gd.mnemonic = 'D';
	    gcd[12].gd.label = &label[12];
	    gcd[12].gd.pos.x = 5; gcd[12].gd.pos.y = 5;
	    gcd[12].gd.flags = gg_enabled | gg_visible | (def->centerline ? gg_cb_on : 0 );
	    gcd[12].gd.cid = CID_CenterLine;
	    gcd[12].gd.handle_controlevent = Stroke_CenterLine;
	    gcd[12].creator = GRadioCreate;
	    gcdoff = 1;
	}

	label[12+gcdoff].text = (unichar_t *) _STR_Strok;
	label[12+gcdoff].text_in_resource = true;
	gcd[12+gcdoff].gd.mnemonic = 'S';
	gcd[12+gcdoff].gd.label = &label[12+gcdoff];
	gcd[12+gcdoff].gd.pos.x = 5; gcd[12+gcdoff].gd.pos.y = 5+yoff;
	gcd[12+gcdoff].gd.flags = gg_enabled | gg_visible | (!def->caligraphic && !def->centerline? gg_cb_on : 0);
	gcd[12+gcdoff].gd.cid = CID_Stroke;
	gcd[12+gcdoff].gd.handle_controlevent = Stroke_Stroke;
	gcd[12+gcdoff].creator = GRadioCreate;

	label[13+gcdoff].text = (unichar_t *) _STR_Caligraphic;
	label[13+gcdoff].text_in_resource = true;
	gcd[13+gcdoff].gd.mnemonic = 'C';
	gcd[13+gcdoff].gd.label = &label[13+gcdoff];
	gcd[13+gcdoff].gd.pos.x = 5; gcd[13+gcdoff].gd.pos.y = gcd[5].gd.pos.y+38;
	gcd[13+gcdoff].gd.flags = gg_enabled | gg_visible | (def->caligraphic ? gg_cb_on : 0);
	gcd[13+gcdoff].gd.cid = CID_Caligraphic;
	gcd[13+gcdoff].gd.handle_controlevent = Stroke_Caligraphic;
	gcd[13+gcdoff].creator = GRadioCreate;

	gcd[14+gcdoff].gd.pos.x = 1; gcd[14+gcdoff].gd.pos.y = gcd[12+gcdoff].gd.pos.y+6;
	gcd[14+gcdoff].gd.pos.width = SD_Width-2; gcd[14+gcdoff].gd.pos.height = 84;
	gcd[14+gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[14+gcdoff].creator = GGroupCreate;

	gcd[15+gcdoff].gd.pos.x = 1; gcd[15+gcdoff].gd.pos.y = gcd[13+gcdoff].gd.pos.y+6;
	gcd[15+gcdoff].gd.pos.width = SD_Width-2; gcd[15+gcdoff].gd.pos.height = 58;
	gcd[15+gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[15+gcdoff].creator = GGroupCreate;

	label[16+gcdoff].text = (unichar_t *) _STR_PenAngle;
	label[16+gcdoff].text_in_resource = true;
	gcd[16+gcdoff].gd.mnemonic = 'A';
	gcd[16+gcdoff].gd.label = &label[16+gcdoff];
	gcd[16+gcdoff].gd.pos.x = gcd[2].gd.pos.x; gcd[16+gcdoff].gd.pos.y = gcd[13+gcdoff].gd.pos.y+15+3;
	gcd[16+gcdoff].gd.flags = gg_visible;
	gcd[16+gcdoff].gd.cid = CID_PenAngleTxt;
	gcd[16+gcdoff].creator = GLabelCreate;

	sprintf( anglebuf, "%g", def->penangle*180/3.1415926535897932 );
	label[17+gcdoff].text = (unichar_t *) anglebuf;
	label[17+gcdoff].text_is_1byte = true;
	gcd[17+gcdoff].gd.mnemonic = 'A';
	gcd[17+gcdoff].gd.label = &label[17+gcdoff];
	gcd[17+gcdoff].gd.pos.x = 80; gcd[17+gcdoff].gd.pos.y = gcd[13+gcdoff].gd.pos.y+15;
	gcd[17+gcdoff].gd.flags = gg_visible;
	gcd[17+gcdoff].gd.cid = CID_PenAngle;
	gcd[17+gcdoff].creator = GTextFieldCreate;

	label[18+gcdoff].text = (unichar_t *) _STR_PenHeightRatio;
	label[18+gcdoff].text_in_resource = true;
	gcd[18+gcdoff].gd.mnemonic = 'H';
	gcd[18+gcdoff].gd.label = &label[18+gcdoff];
	gcd[18+gcdoff].gd.pos.x = gcd[2].gd.pos.x; gcd[18+gcdoff].gd.pos.y = gcd[16+gcdoff].gd.pos.y+24;
	gcd[18+gcdoff].gd.flags = gg_visible;
	gcd[18+gcdoff].gd.cid = CID_ThicknessRatioTxt;
	gcd[18+gcdoff].gd.popup_msg = GStringGetResource(_STR_PenHeightRatioPopup,NULL);
	gcd[18+gcdoff].creator = GLabelCreate;

	sprintf( ratiobuf, "%g", def->ratio );
	label[19+gcdoff].text = (unichar_t *) ratiobuf;
	label[19+gcdoff].text_is_1byte = true;
	gcd[19+gcdoff].gd.mnemonic = 'H';
	gcd[19+gcdoff].gd.label = &label[19+gcdoff];
	gcd[19+gcdoff].gd.pos.x = gcd[17+gcdoff].gd.pos.x; gcd[19+gcdoff].gd.pos.y = gcd[17+gcdoff].gd.pos.y+24;
	gcd[19+gcdoff].gd.flags = gg_visible;
	gcd[19+gcdoff].gd.cid = CID_ThicknessRatio;
	gcd[19+gcdoff].gd.popup_msg = GStringGetResource(_STR_PenHeightRatioPopup,NULL);
	gcd[19+gcdoff].creator = GTextFieldCreate;

	label[20+gcdoff].text = (unichar_t *) _STR_StrokeWidth;
	label[20+gcdoff].text_in_resource = true;
	gcd[20+gcdoff].gd.mnemonic = 'W';
	gcd[20+gcdoff].gd.label = &label[20+gcdoff];
	gcd[20+gcdoff].gd.pos.x = 5; gcd[20+gcdoff].gd.pos.y = 166+3+yoff;
	gcd[20+gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[20+gcdoff].gd.cid = CID_WidthTxt;
	gcd[20+gcdoff].creator = GLabelCreate;

	sprintf( widthbuf, "%g", 2*def->radius );
	label[21+gcdoff].text = (unichar_t *) widthbuf;
	label[21+gcdoff].text_is_1byte = true;
	gcd[21+gcdoff].gd.mnemonic = 'W';
	gcd[21+gcdoff].gd.label = &label[21+gcdoff];
	gcd[21+gcdoff].gd.pos.x = 80; gcd[21+gcdoff].gd.pos.y = gcd[20+gcdoff].gd.pos.y-3;
	gcd[21+gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[21+gcdoff].gd.cid = CID_Width;
	gcd[21+gcdoff].creator = GTextFieldCreate;

	if ( si!=NULL ) {
	    gcd[21+gcdoff].gd.pos.width = 50;

	    sprintf( width2buf, "%g", 2*def->radius2 );
	    label[22+gcdoff].text = (unichar_t *) width2buf;
	    label[22+gcdoff].text_is_1byte = true;
	    gcd[22+gcdoff].gd.label = &label[22+gcdoff];
	    gcd[22+gcdoff].gd.pos.x = 140; gcd[22+gcdoff].gd.pos.y = gcd[21+gcdoff].gd.pos.y;
	    gcd[22+gcdoff].gd.flags = gg_visible;
	    if ( def->pressure1!=def->pressure2 )
		gcd[22+gcdoff].gd.flags = gg_enabled | gg_visible;
	    gcd[22+gcdoff].gd.cid = CID_Width2;
	    gcd[22+gcdoff].creator = GTextFieldCreate;
	    gcd[22+gcdoff].gd.pos.width = 50;

	    sd->r1.x = GDrawPointsToPixels(NULL,90);
	    sd->r1.width=sd->r1.height=GDrawPointsToPixels(NULL,20);
	    sd->r1.y = GDrawPointsToPixels(NULL,gcd[22+gcdoff].gd.pos.y+26);
	    sd->r2 = sd->r1;
	    sd->r2.x = GDrawPointsToPixels(NULL,150);

	    label[23+gcdoff].text = (unichar_t *) _STR_Pressure;
	    label[23+gcdoff].text_in_resource = true;
	    gcd[23+gcdoff].gd.mnemonic = 'P';
	    gcd[23+gcdoff].gd.label = &label[23+gcdoff];
	    gcd[23+gcdoff].gd.pos.x = 5;
	    gcd[23+gcdoff].gd.pos.y = GDrawPixelsToPoints(NULL,sd->r1.y+sd->r1.height)+5+3;
	    gcd[23+gcdoff].gd.flags = gg_enabled | gg_visible;
	    gcd[23+gcdoff].gd.cid = CID_PressureTxt;
	    gcd[23+gcdoff].creator = GLabelCreate;

	    sprintf( pressurebuf, "%d", def->pressure1 );
	    label[24+gcdoff].text = (unichar_t *) pressurebuf;
	    label[24+gcdoff].text_is_1byte = true;
	    gcd[24+gcdoff].gd.mnemonic = 'W';
	    gcd[24+gcdoff].gd.label = &label[24+gcdoff];
	    gcd[24+gcdoff].gd.pos.x = gcd[21+gcdoff].gd.pos.x; gcd[24+gcdoff].gd.pos.y = gcd[23+gcdoff].gd.pos.y-3;
	    gcd[24+gcdoff].gd.flags = gg_enabled | gg_visible;
	    gcd[24+gcdoff].gd.cid = CID_Pressure1;
	    gcd[24+gcdoff].gd.handle_controlevent = Stroke_PressureChange;
	    gcd[24+gcdoff].creator = GTextFieldCreate;
	    gcd[24+gcdoff].gd.pos.width = 50;

	    sprintf( pressure2buf, "%d", def->pressure2 );
	    label[25+gcdoff].text = (unichar_t *) pressure2buf;
	    label[25+gcdoff].text_is_1byte = true;
	    gcd[25+gcdoff].gd.label = &label[25+gcdoff];
	    gcd[25+gcdoff].gd.pos.x = gcd[22+gcdoff].gd.pos.x; gcd[25+gcdoff].gd.pos.y = gcd[24+gcdoff].gd.pos.y;
	    gcd[25+gcdoff].gd.flags = gg_enabled | gg_visible;
	    gcd[25+gcdoff].gd.cid = CID_Pressure2;
	    gcd[25+gcdoff].gd.handle_controlevent = Stroke_PressureChange;
	    gcd[25+gcdoff].creator = GTextFieldCreate;
	    gcd[25+gcdoff].gd.pos.width = 50;
	}

	GGadgetsCreate(gw,gcd);
    }

    sd->cv = cv;
    sd->strokeit = strokeit;
    sd->done = false;
    sd->up[0] = sd->up[1] = true;
    GWidgetHidePalettes();
    GWidgetIndicateFocusGadget(GWidgetGetControl(sd->gw,CID_Width));
    if ( si==NULL )
	StrokeSetup(sd,GGadgetIsChecked( GWidgetGetControl(sd->gw,CID_Caligraphic)) );
    else
	StrokeSetup(sd,def->centerline?-1:def->caligraphic);
    GDrawSetVisible(sd->gw,true);
    while ( !sd->done )
	GDrawProcessOneEvent(NULL);
    if ( strokeit!=NULL )
	GDrawSetVisible(sd->gw,false);
    else
	GDrawDestroyWindow(sd->gw);
}

void CVStroke(CharView *cv) {

    if ( *cv->heads[cv->drawmode]==NULL )
return;

    MakeStrokeDlg(cv,CVStrokeIt,NULL);
}

void SCStroke(SplineChar *sc) {
    MakeStrokeDlg(sc,SCStrokeIt,NULL);
}

void FVStroke(FontView *fv) {
    MakeStrokeDlg(fv,FVStrokeIt,NULL);
}

void FVStrokeItScript(FontView *fv, StrokeInfo *si) {
    FVStrokeIt(fv, si);
}

void FreeHandStrokeDlg(StrokeInfo *si) {
    MakeStrokeDlg(NULL,NULL,si);
}
