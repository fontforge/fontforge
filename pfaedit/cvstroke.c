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
	/* For Kanou */
#define CID_RmInternal	1022

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
	si->removeinternal = GGadgetIsChecked( GWidgetGetControl(sw,CID_RmInternal));
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
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_RmInternal), calig==0);
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
#define SD_Height	241
#define FH_Height	(SD_Height+75)

static void MakeStrokeDlg(void *cv,void (*strokeit)(void *,StrokeInfo *),StrokeInfo *si) {
    static StrokeDlg strokedlg;
    StrokeDlg *sd, freehand_dlg;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[31];
    GTextInfo label[31];
    int yoff=0;
    int gcdoff, stroke_gcd;
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

	gcdoff = 0;
	if ( strokeit==NULL ) {
	    label[0].text = (unichar_t *) _STR_CenterLine;
	    label[0].text_in_resource = true;
	    gcd[0].gd.label = &label[0];
	    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5;
	    gcd[0].gd.flags = gg_enabled | gg_visible | (def->centerline ? gg_cb_on : 0 );
	    gcd[0].gd.cid = CID_CenterLine;
	    gcd[0].gd.handle_controlevent = Stroke_CenterLine;
	    gcd[0].creator = GRadioCreate;
	    gcdoff = 1;
	}

	label[gcdoff].text = (unichar_t *) _STR_Strok;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'S';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = 5+yoff;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (!def->caligraphic && !def->centerline? gg_cb_on : 0);
	gcd[gcdoff].gd.cid = CID_Stroke;
	gcd[gcdoff].gd.handle_controlevent = Stroke_Stroke;
	gcd[gcdoff++].creator = GRadioCreate;

	stroke_gcd = gcdoff;
	gcd[gcdoff].gd.pos.x = 1; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+6;
	gcd[gcdoff].gd.pos.width = SD_Width-2; gcd[gcdoff].gd.pos.height = 99;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff++].creator = GGroupCreate;

	label[gcdoff].text = (unichar_t *) _STR_LineCap;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 10; gcd[gcdoff].gd.pos.y = 23+yoff;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff].gd.cid = CID_LineCapTxt;
	gcd[gcdoff++].creator = GLabelCreate;

	gcd[gcdoff].gd.pos.x = 6; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+6;
	gcd[gcdoff].gd.pos.width = SD_Width-12; gcd[gcdoff].gd.pos.height = 25;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff++].creator = GGroupCreate;

	label[gcdoff].text = (unichar_t *) _STR_Butt;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_buttcap;
	gcd[gcdoff].gd.mnemonic = 'B';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 15; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+12;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->cap==lc_butt?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_ButtCap;
	gcd[gcdoff++].creator = GRadioCreate;

	label[gcdoff].text = (unichar_t *) _STR_Round;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_roundcap;
	gcd[gcdoff].gd.mnemonic = 'R';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->cap==lc_round?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_RoundCap;
	gcd[gcdoff++].creator = GRadioCreate;

	label[gcdoff].text = (unichar_t *) _STR_Squareq;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_squarecap;
	gcd[gcdoff].gd.mnemonic = 'q';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 150; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->cap==lc_square?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_SquareCap;
	gcd[gcdoff++].creator = GRadioCreate;

	label[gcdoff].text = (unichar_t *) _STR_LineJoin;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-5].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-3].gd.pos.y+25;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff].gd.cid = CID_LineJoinTxt;
	gcd[gcdoff++].creator = GLabelCreate;

	gcd[gcdoff].gd.pos.x = gcd[gcdoff-5].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+6;
	gcd[gcdoff].gd.pos.width = SD_Width-12; gcd[gcdoff].gd.pos.height = 25;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff++].creator = GGroupCreate;

	label[gcdoff].text = (unichar_t *) _STR_Miter;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_miterjoin;
	gcd[gcdoff].gd.mnemonic = 'M';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-5].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+12;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->join==lj_miter?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_MiterJoin;
	gcd[gcdoff++].creator = GRadioCreate;

	label[gcdoff].text = (unichar_t *) _STR_Roundu;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_roundjoin;
	gcd[gcdoff].gd.mnemonic = 'u';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-5].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->join==lj_round?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_RoundJoin;
	gcd[gcdoff++].creator = GRadioCreate;

	label[gcdoff].text = (unichar_t *) _STR_Bevel;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_beveljoin;
	gcd[gcdoff].gd.mnemonic = 'v';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-5].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->join==lj_bevel?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_BevelJoin;
	gcd[gcdoff++].creator = GRadioCreate;

	label[gcdoff].text = (unichar_t *) _STR_RmInternalContour;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-4].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+20;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->removeinternal?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_RmInternal;
	gcd[gcdoff++].creator = GCheckBoxCreate;

	label[gcdoff].text = (unichar_t *) _STR_Caligraphic;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'C';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[stroke_gcd].gd.pos.y+gcd[stroke_gcd].gd.pos.height+4;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->caligraphic ? gg_cb_on : 0);
	gcd[gcdoff].gd.cid = CID_Caligraphic;
	gcd[gcdoff].gd.handle_controlevent = Stroke_Caligraphic;
	gcd[gcdoff++].creator = GRadioCreate;

	gcd[gcdoff].gd.pos.x = 1; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+6;
	gcd[gcdoff].gd.pos.width = SD_Width-2; gcd[gcdoff].gd.pos.height = 58;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff++].creator = GGroupCreate;

	label[gcdoff].text = (unichar_t *) _STR_PenAngle;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'A';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[stroke_gcd+3].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+15+3;
	gcd[gcdoff].gd.flags = gg_visible;
	gcd[gcdoff].gd.cid = CID_PenAngleTxt;
	gcd[gcdoff++].creator = GLabelCreate;

	sprintf( anglebuf, "%g", def->penangle*180/3.1415926535897932 );
	label[gcdoff].text = (unichar_t *) anglebuf;
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.mnemonic = 'A';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
	gcd[gcdoff].gd.flags = gg_visible;
	gcd[gcdoff].gd.cid = CID_PenAngle;
	gcd[gcdoff++].creator = GTextFieldCreate;

	label[gcdoff].text = (unichar_t *) _STR_PenHeightRatio;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'H';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-2].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+24;
	gcd[gcdoff].gd.flags = gg_visible;
	gcd[gcdoff].gd.cid = CID_ThicknessRatioTxt;
	gcd[gcdoff].gd.popup_msg = GStringGetResource(_STR_PenHeightRatioPopup,NULL);
	gcd[gcdoff++].creator = GLabelCreate;

	sprintf( ratiobuf, "%g", def->ratio );
	label[gcdoff].text = (unichar_t *) ratiobuf;
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.mnemonic = 'H';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-2].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+24;
	gcd[gcdoff].gd.flags = gg_visible;
	gcd[gcdoff].gd.cid = CID_ThicknessRatio;
	gcd[gcdoff].gd.popup_msg = GStringGetResource(_STR_PenHeightRatioPopup,NULL);
	gcd[gcdoff++].creator = GTextFieldCreate;

	label[gcdoff].text = (unichar_t *) _STR_StrokeWidth;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'W';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+31;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff].gd.cid = CID_WidthTxt;
	gcd[gcdoff++].creator = GLabelCreate;

	sprintf( widthbuf, "%g", 2*def->radius );
	label[gcdoff].text = (unichar_t *) widthbuf;
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.mnemonic = 'W';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff].gd.cid = CID_Width;
	gcd[gcdoff++].creator = GTextFieldCreate;

	if ( si!=NULL ) {
	    gcd[gcdoff-1].gd.pos.width = 50;

	    sprintf( width2buf, "%g", 2*def->radius2 );
	    label[gcdoff].text = (unichar_t *) width2buf;
	    label[gcdoff].text_is_1byte = true;
	    gcd[gcdoff].gd.label = &label[gcdoff];
	    gcd[gcdoff].gd.pos.x = 140; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
	    gcd[gcdoff].gd.flags = gg_visible;
	    if ( def->pressure1!=def->pressure2 )
		gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	    gcd[gcdoff].gd.cid = CID_Width2;
	    gcd[gcdoff].creator = GTextFieldCreate;
	    gcd[gcdoff++].gd.pos.width = 50;

	    sd->r1.x = GDrawPointsToPixels(NULL,90);
	    sd->r1.width=sd->r1.height=GDrawPointsToPixels(NULL,20);
	    sd->r1.y = GDrawPointsToPixels(NULL,gcd[gcdoff-1].gd.pos.y+26);
	    sd->r2 = sd->r1;
	    sd->r2.x = GDrawPointsToPixels(NULL,150);

	    label[gcdoff].text = (unichar_t *) _STR_Pressure;
	    label[gcdoff].text_in_resource = true;
	    gcd[gcdoff].gd.mnemonic = 'P';
	    gcd[gcdoff].gd.label = &label[gcdoff];
	    gcd[gcdoff].gd.pos.x = 5;
	    gcd[gcdoff].gd.pos.y = GDrawPixelsToPoints(NULL,sd->r1.y+sd->r1.height)+5+3;
	    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	    gcd[gcdoff].gd.cid = CID_PressureTxt;
	    gcd[gcdoff++].creator = GLabelCreate;

	    sprintf( pressurebuf, "%d", def->pressure1 );
	    label[gcdoff].text = (unichar_t *) pressurebuf;
	    label[gcdoff].text_is_1byte = true;
	    gcd[gcdoff].gd.mnemonic = 'W';
	    gcd[gcdoff].gd.label = &label[gcdoff];
	    gcd[gcdoff].gd.pos.x = gcd[gcdoff-3].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
	    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	    gcd[gcdoff].gd.cid = CID_Pressure1;
	    gcd[gcdoff].gd.handle_controlevent = Stroke_PressureChange;
	    gcd[gcdoff].creator = GTextFieldCreate;
	    gcd[gcdoff++].gd.pos.width = 50;

	    sprintf( pressure2buf, "%d", def->pressure2 );
	    label[gcdoff].text = (unichar_t *) pressure2buf;
	    label[gcdoff].text_is_1byte = true;
	    gcd[gcdoff].gd.label = &label[gcdoff];
	    gcd[gcdoff].gd.pos.x = gcd[gcdoff-3].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
	    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	    gcd[gcdoff].gd.cid = CID_Pressure2;
	    gcd[gcdoff].gd.handle_controlevent = Stroke_PressureChange;
	    gcd[gcdoff].creator = GTextFieldCreate;
	    gcd[gcdoff++].gd.pos.width = 50;
	}

	gcd[gcdoff].gd.pos.x = 30-3; gcd[gcdoff].gd.pos.y = (strokeit!=NULL?SD_Height:FH_Height)-30-3;
	gcd[gcdoff].gd.pos.width = -1;
	gcd[gcdoff].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[gcdoff].text = (unichar_t *) _STR_OK;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'O';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.handle_controlevent = Stroke_OK;
	gcd[gcdoff++].creator = GButtonCreate;

	gcd[gcdoff].gd.pos.x = -30; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+3;
	gcd[gcdoff].gd.pos.width = -1;
	gcd[gcdoff].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[gcdoff].text = (unichar_t *) _STR_Cancel;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.mnemonic = 'C';
	gcd[gcdoff].gd.handle_controlevent = Stroke_Cancel;
	gcd[gcdoff].creator = GButtonCreate;

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
