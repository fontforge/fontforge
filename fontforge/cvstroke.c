/* Copyright (C) 2000-2008 by George Williams */
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
#include <utype.h>
#include <gkeysym.h>
#include <math.h>

extern GDevEventMask input_em[];
extern const int input_em_cnt;

typedef struct strokedlg {
    int done;
    GWindow gw;
    CharView *cv;
    FontView *fv;
    SplineFont *sf;
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
	/* For Kanou (& me) */
#define CID_RmInternal	1022
#define CID_RmExternal	1023
#define CID_CleanupSelfIntersect	1024
	/* Elipses */
#define CID_Elipse	1025
#define CID_PenAngle2	1026
#define CID_PenAngle2Txt 1027
#define CID_MinorAxis	1028
#define CID_MinorAxisTxt 1029

static void CVStrokeIt(void *_cv, StrokeInfo *si) {
    CharView *cv = _cv;
    int anypoints;
    SplineSet *spl, *prev, *head=NULL, *last=NULL, *cur, *snext;

    CVPreserveState(&cv->b);
    if ( CVAnySel(cv,&anypoints,NULL,NULL,NULL) && anypoints ) {
	prev = NULL;
	for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = snext ) {
	    snext = spl->next;
	    if ( PointListIsSelected(spl)) {
		cur = SplineSetStroke(spl,si,cv->b.sc);
		if ( prev==NULL )
		    cv->b.layerheads[cv->b.drawmode]->splines=cur;
		else
		    prev->next = cur;
		while ( cur->next ) cur=cur->next;
		cur->next = snext;
		spl->next = NULL;
		SplinePointListMDFree(cv->b.sc,spl);
		prev = cur;
	    } else
		prev = spl;
	}
    } else {
	for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	    cur = SplineSetStroke(spl,si,cv->b.sc);
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    while ( cur->next!=NULL ) cur = cur->next;
	    last = cur;
	}
	SplinePointListsFree( cv->b.layerheads[cv->b.drawmode]->splines );
	cv->b.layerheads[cv->b.drawmode]->splines = head;
    }
    CVCharChangedUpdate(&cv->b);
}

static void SCStrokeIt(void *_sc, StrokeInfo *si) {
    SplineChar *sc = _sc;
    SplineSet *temp;

    SCPreserveState(sc,false);
    temp = SSStroke(sc->layers[ly_fore].splines,si,sc);
    SplinePointListsFree( sc->layers[ly_fore].splines );
    sc->layers[ly_fore].splines = temp;
    SCCharChangedUpdate(sc,ly_fore);
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
	}
	si->stroke_type = si_std;
	if ( GGadgetIsChecked( GWidgetGetControl(sw,CID_Caligraphic)) )
	    si->stroke_type = si_caligraphic;
	else if ( GGadgetIsChecked( GWidgetGetControl(sw,CID_Elipse)) )
	    si->stroke_type = si_elipse;
	else if ( si!= &strokeinfo &&
		GGadgetIsChecked( GWidgetGetControl(sw,CID_CenterLine)) )
	    si->stroke_type = si_centerline;
	if ( si!=&strokeinfo && si->stroke_type!=si_centerline ) {
	    si->pressure1 = GetReal8(sw,CID_Pressure1,_("_Pressure:"),&err);
	    si->pressure2 = GetReal8(sw,CID_Pressure2,_("_Pressure:"),&err);
	    if ( si->pressure1!=si->pressure2 )
		si->radius2 = GetReal8(sw,CID_Width2,_("Stroke _Width:"),&err)/2;
	}
	si->cap = GGadgetIsChecked( GWidgetGetControl(sw,CID_ButtCap))?lc_butt:
		GGadgetIsChecked( GWidgetGetControl(sw,CID_RoundCap))?lc_round:
		lc_square;
	si->join = GGadgetIsChecked( GWidgetGetControl(sw,CID_BevelJoin))?lj_bevel:
		GGadgetIsChecked( GWidgetGetControl(sw,CID_RoundJoin))?lj_round:
		lj_miter;
	si->removeinternal = GGadgetIsChecked( GWidgetGetControl(sw,CID_RmInternal));
	si->removeexternal = GGadgetIsChecked( GWidgetGetControl(sw,CID_RmExternal));
	si->removeoverlapifneeded = GGadgetIsChecked( GWidgetGetControl(sw,CID_CleanupSelfIntersect));
	if ( si->removeinternal && si->removeexternal ) {
	    ff_post_error(_("Bad Value"),_("Removing both the internal and the external contours makes no sense"));
	    err = true;
	}
	si->radius = GetReal8(sw,CID_Width,_("Stroke _Width:"),&err)/2;
	if ( si->radius<0 ) si->radius = -si->radius;	/* Behavior is said to be very slow (but correct) for negative strokes */
	if ( si->stroke_type == si_elipse ) {
	    si->penangle = GetReal8(sw,CID_PenAngle2,_("Pen _Angle:"),&err);
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
	    si->ratio = GetReal8(sw,CID_ThicknessRatio,_("_Height Ratio:"),&err);
	    si->s = sin(si->penangle);
	    si->c = cos(si->penangle);
	    si->cap = lc_round; si->join = lj_round;
	    si->minorradius = GetReal8(sw,CID_MinorAxis,_("Minor A_xis:"),&err)/2;
	} else if ( si->stroke_type == si_caligraphic ) {
	    si->penangle = GetReal8(sw,CID_PenAngle,_("Pen _Angle:"),&err);
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
	    si->ratio = GetReal8(sw,CID_ThicknessRatio,_("_Height Ratio:"),&err);
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

    p1 = GetReal8(sd->gw,CID_Pressure1,_("_Pressure:"),&err);
    p2 = GetReal8(sd->gw,CID_Pressure2,_("_Pressure:"),&err);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_Width2),
	    !err && p1!=p2 && !sd->dontexpand);
}

static void StrokeSetup(StrokeDlg *sd, enum si_type stroke_type) {

    sd->dontexpand = ( stroke_type==si_centerline );

    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_LineCapTxt), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_ButtCap), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_RoundCap), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_SquareCap), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_LineJoinTxt), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_BevelJoin), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_RoundJoin), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_MiterJoin), stroke_type==si_std);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PenAngle), stroke_type==si_caligraphic);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PenAngleTxt), stroke_type==si_caligraphic);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_ThicknessRatio), stroke_type==si_caligraphic);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_ThicknessRatioTxt), stroke_type==si_caligraphic);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PenAngle2), stroke_type==si_elipse);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PenAngle2Txt), stroke_type==si_elipse);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_MinorAxis), stroke_type==si_elipse);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_MinorAxisTxt), stroke_type==si_elipse);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_WidthTxt), stroke_type!=-si_caligraphic);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_Width), stroke_type!=-si_caligraphic);
    if ( sd->si!=NULL ) {
	StrokePressureCheck(sd);
	GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_PressureTxt), stroke_type!=si_centerline);
	GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_Pressure1), stroke_type!=si_centerline);
	GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_Pressure2), stroke_type!=si_centerline);
    }
#if 0
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_RmInternal), true);
    GGadgetSetEnabled(GWidgetGetControl(sd->gw,CID_RmExternal), stroke_type==si_std);
#endif
}

static int Stroke_CenterLine(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StrokeDlg *sd = GDrawGetUserData(GGadgetGetWindow(g));
	StrokeSetup(sd,si_centerline);
    }
return( true );
}

static int Stroke_Elipse(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StrokeDlg *sd = GDrawGetUserData(GGadgetGetWindow(g));
	StrokeSetup(sd,si_elipse);
    }
return( true );
}

static int Stroke_Caligraphic(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StrokeDlg *sd = GDrawGetUserData(GGadgetGetWindow(g));
	StrokeSetup(sd,si_caligraphic);
    }
return( true );
}

static int Stroke_Stroke(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	StrokeDlg *sd = GDrawGetUserData(GGadgetGetWindow(g));
	StrokeSetup(sd,si_std);
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
	sprintf(buff,"%d",(int) event->u.mouse.pressure);
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
		GGadgetPreparePopup8(gw,_("Press in this square with a wacom pressure sensitive tool\nto set the pressure end-point"));
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
	GDrawSetLineWidth(gw,0);
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
#define SD_Height	335
#define FH_Height	(SD_Height+75)

static void MakeStrokeDlg(void *cv,void (*strokeit)(void *,StrokeInfo *),StrokeInfo *si) {
    static StrokeDlg strokedlg;
    StrokeDlg *sd, freehand_dlg;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[38];
    GTextInfo label[38];
    int yoff=0;
    int gcdoff, stroke_gcd, width_pos;
    static StrokeInfo defaults = { 25, lj_round, lc_butt, si_std,
	    /* toobigwarn */  false,
	    /* removeinternal */ false,
	    /* removeexternal */ false,
	    /* removeoverlapif*/ true,
	    /* gottoobig */	 false,
	    /* gottoobiglocal */ false,
	    3.1415926535897932/4, .2, 50 };
    StrokeInfo *def = si?si:&defaults;
    char anglebuf[20], ratiobuf[20], widthbuf[20], width2buf[20],
	    pressurebuf[20], pressure2buf[20], axisbuf[20];

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
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = strokeit!=NULL ? _("Expand Stroke") :
/* GT: This does not mean the program, but freehand drawing */
		    _("Freehand");
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
	    label[0].text = (unichar_t *) _("_Don't Expand");
	    label[0].text_is_1byte = true;
	    label[0].text_in_resource = true;
	    gcd[0].gd.label = &label[0];
	    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5;
	    gcd[0].gd.flags = gg_enabled | gg_visible | (def->stroke_type==si_centerline ? gg_cb_on : 0 );
	    gcd[0].gd.cid = CID_CenterLine;
	    gcd[0].gd.handle_controlevent = Stroke_CenterLine;
	    gcd[0].creator = GRadioCreate;
	    gcdoff = 1;
	}

	label[gcdoff].text = (unichar_t *) _("_Stroke");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'S';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = 5+yoff;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->stroke_type==si_std? gg_cb_on : 0);
	gcd[gcdoff].gd.cid = CID_Stroke;
	gcd[gcdoff].gd.handle_controlevent = Stroke_Stroke;
	gcd[gcdoff++].creator = GRadioCreate;

	    /* This radio button is here rather than where it's location would */
	    /*  suggest because itneeds to be grouped with stroke and dont expand */
	label[gcdoff].text = (unichar_t *) _("_Calligraphic");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'C';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+6+84+4;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->stroke_type == si_caligraphic ? gg_cb_on : 0);
	gcd[gcdoff].gd.cid = CID_Caligraphic;
	gcd[gcdoff].gd.handle_controlevent = Stroke_Caligraphic;
	gcd[gcdoff++].creator = GRadioCreate;

	    /* ditto */
	label[gcdoff].text = (unichar_t *) _("_Ellipse");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'E';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+6+58+4;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->stroke_type == si_elipse ? gg_cb_on : 0);
	gcd[gcdoff].gd.cid = CID_Elipse;
	gcd[gcdoff].gd.handle_controlevent = Stroke_Elipse;
	gcd[gcdoff++].creator = GRadioCreate;

	stroke_gcd = gcdoff;
	gcd[gcdoff].gd.pos.x = 1; gcd[gcdoff].gd.pos.y = gcd[gcdoff-3].gd.pos.y+6;
	gcd[gcdoff].gd.pos.width = SD_Width-2; gcd[gcdoff].gd.pos.height = 84;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff++].creator = GGroupCreate;

	gcd[gcdoff].gd.pos.x = 1; gcd[gcdoff].gd.pos.y = gcd[gcdoff-3].gd.pos.y+6;
	gcd[gcdoff].gd.pos.width = SD_Width-2; gcd[gcdoff].gd.pos.height = 58;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff++].creator = GGroupCreate;

	gcd[gcdoff].gd.pos.x = 1; gcd[gcdoff].gd.pos.y = gcd[gcdoff-3].gd.pos.y+6;
	gcd[gcdoff].gd.pos.width = SD_Width-2; gcd[gcdoff].gd.pos.height = 58;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff++].creator = GGroupCreate;

	label[gcdoff].text = (unichar_t *) _("Line Cap");
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 10; gcd[gcdoff].gd.pos.y = 23+yoff;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff].gd.cid = CID_LineCapTxt;
	gcd[gcdoff++].creator = GLabelCreate;

	gcd[gcdoff].gd.pos.x = 6; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+6;
	gcd[gcdoff].gd.pos.width = SD_Width-12; gcd[gcdoff].gd.pos.height = 25;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff++].creator = GGroupCreate;

/* GT: Butt is a PostScript concept which refers to a way of ending strokes */
/* GT: In the following image the line drawn with "=" is the original, and */
/* GT: the others are the results. The "Round" style is hard to draw with */
/* GT: ASCII glyphs. If this is unclear I suggest you look at the Expand Stroke */
/* GT: dialog which has little pictures */
/* GT: */
/* GT: -----------------+    -----------------+    ----------------+--+ */
/* GT:                  |                      \                      | */
/* GT: =================+    ================== )  =================  | */
/* GT:                  |                      /                      | */
/* GT: -----------------+    -----------------+    ----------------+--+ */
/* GT:       Butt                 Round                Square */
	label[gcdoff].text = (unichar_t *) _("_Butt");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_buttcap;
	gcd[gcdoff].gd.mnemonic = 'B';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 15; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+12;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->cap==lc_butt?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_ButtCap;
	gcd[gcdoff++].creator = GRadioCreate;

	label[gcdoff].text = (unichar_t *) _("_Round");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_roundcap;
	gcd[gcdoff].gd.mnemonic = 'R';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->cap==lc_round?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_RoundCap;
	gcd[gcdoff++].creator = GRadioCreate;

	label[gcdoff].text = (unichar_t *) _("S_quare");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_squarecap;
	gcd[gcdoff].gd.mnemonic = 'q';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 150; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->cap==lc_square?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_SquareCap;
	gcd[gcdoff++].creator = GRadioCreate;

	label[gcdoff].text = (unichar_t *) _("Line Join");
	label[gcdoff].text_is_1byte = true;
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

	label[gcdoff].text = (unichar_t *) _("_Miter");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_miterjoin;
	gcd[gcdoff].gd.mnemonic = 'M';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-5].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+12;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->join==lj_miter?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_MiterJoin;
	gcd[gcdoff++].creator = GRadioCreate;

	label[gcdoff].text = (unichar_t *) _("Ro_und");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_roundjoin;
	gcd[gcdoff].gd.mnemonic = 'u';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-5].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->join==lj_round?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_RoundJoin;
	gcd[gcdoff++].creator = GRadioCreate;

	label[gcdoff].text = (unichar_t *) _("Be_vel");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	label[gcdoff].image = &GIcon_beveljoin;
	gcd[gcdoff].gd.mnemonic = 'v';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-5].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->join==lj_bevel?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_BevelJoin;
	gcd[gcdoff++].creator = GRadioCreate;

	    /* Caligraphic */
	label[gcdoff].text = (unichar_t *) _("Pen _Angle:");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'A';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[stroke_gcd+5].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[stroke_gcd-2].gd.pos.y+15+3;
	gcd[gcdoff].gd.flags = gg_visible;
	gcd[gcdoff].gd.cid = CID_PenAngleTxt;
	gcd[gcdoff++].creator = GLabelCreate;

	sprintf( anglebuf, "%g", (double) (def->penangle*180/3.1415926535897932) );
	label[gcdoff].text = (unichar_t *) anglebuf;
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.mnemonic = 'A';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
	gcd[gcdoff].gd.flags = gg_visible;
	gcd[gcdoff].gd.cid = CID_PenAngle;
	gcd[gcdoff++].creator = GTextFieldCreate;

	label[gcdoff].text = (unichar_t *) _("_Height Ratio:");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'H';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-2].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+24;
	gcd[gcdoff].gd.flags = gg_visible | gg_utf8_popup;
	gcd[gcdoff].gd.cid = CID_ThicknessRatioTxt;
	gcd[gcdoff].gd.popup_msg = (unichar_t *) _("A calligraphic pen's nib has two dimensions, the width\n(which may be set by Stroke Width below) and the thickness\nor height. I express the height as a ratio to the width.");
	gcd[gcdoff++].creator = GLabelCreate;

	sprintf( ratiobuf, "%g", (double) def->ratio );
	label[gcdoff].text = (unichar_t *) ratiobuf;
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.mnemonic = 'H';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-2].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+24;
	gcd[gcdoff].gd.flags = gg_visible | gg_utf8_popup;
	gcd[gcdoff].gd.cid = CID_ThicknessRatio;
	gcd[gcdoff].gd.popup_msg = (unichar_t *) _("A calligraphic pen's nib has two dimensions, the width\n(which may be set by Stroke Width below) and the thickness\nor height. I express the height as a ratio to the width.");
	gcd[gcdoff++].creator = GTextFieldCreate;

	    /* Elipse */
	label[gcdoff].text = (unichar_t *) _("Pen _Angle:");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'A';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[stroke_gcd+5].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[stroke_gcd-1].gd.pos.y+15+3;
	gcd[gcdoff].gd.flags = gg_visible;
	gcd[gcdoff].gd.cid = CID_PenAngle2Txt;
	gcd[gcdoff++].creator = GLabelCreate;

	sprintf( anglebuf, "%g", (double) (def->penangle*180/3.1415926535897932) );
	label[gcdoff].text = (unichar_t *) anglebuf;
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.mnemonic = 'A';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
	gcd[gcdoff].gd.flags = gg_visible;
	gcd[gcdoff].gd.cid = CID_PenAngle2;
	gcd[gcdoff++].creator = GTextFieldCreate;

	label[gcdoff].text = (unichar_t *) _("Minor A_xis:");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'H';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-2].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+24;
	gcd[gcdoff].gd.flags = gg_visible;
	gcd[gcdoff].gd.cid = CID_MinorAxisTxt;
	gcd[gcdoff++].creator = GLabelCreate;

	sprintf( axisbuf, "%g", (double) def->minorradius );
	label[gcdoff].text = (unichar_t *) axisbuf;
	label[gcdoff].text_is_1byte = true;
	gcd[gcdoff].gd.mnemonic = 'H';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-2].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+24;
	gcd[gcdoff].gd.flags = gg_visible | gg_utf8_popup;
	gcd[gcdoff].gd.cid = CID_MinorAxis;
	gcd[gcdoff].gd.popup_msg = (unichar_t *) _("A calligraphic pen's nib has two dimensions, the width\n(which may be set by Stroke Width below) and the thickness\nor height. I express the height as a ratio to the width.");
	gcd[gcdoff++].creator = GTextFieldCreate;
	/* End radio area */

	width_pos = gcdoff;
	label[gcdoff].text = (unichar_t *) _("Stroke _Width:");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'W';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+31;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
	gcd[gcdoff].gd.cid = CID_WidthTxt;
	gcd[gcdoff++].creator = GLabelCreate;

	sprintf( widthbuf, "%g", (double) (2*def->radius) );
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

	    sprintf( width2buf, "%g", (double) (2*def->radius2) );
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

	    label[gcdoff].text = (unichar_t *) _("_Pressure:");
	    label[gcdoff].text_is_1byte = true;
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

	label[gcdoff].text = (unichar_t *) _("Remove Internal Contour");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[width_pos].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+20;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->removeinternal?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_RmInternal;
	gcd[gcdoff++].creator = GCheckBoxCreate;

	label[gcdoff].text = (unichar_t *) _("Remove External Contour");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-1].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+15;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (def->removeexternal?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_RmExternal;
	gcd[gcdoff++].creator = GCheckBoxCreate;

	label[gcdoff].text = (unichar_t *) _("Cleanup Self Intersect");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.pos.x = gcd[gcdoff-1].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+15;
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | gg_utf8_popup | (def->removeoverlapifneeded?gg_cb_on:0);
	gcd[gcdoff].gd.cid = CID_CleanupSelfIntersect;
	gcd[gcdoff].gd.popup_msg = (unichar_t *) _("When FontForge detects that an expanded stroke will self-intersect,\nthen setting this option will cause it to try to make things nice\nby removing the intersections");
	gcd[gcdoff++].creator = GCheckBoxCreate;

	gcd[gcdoff].gd.pos.x = 30-3; gcd[gcdoff].gd.pos.y = (strokeit!=NULL?SD_Height:FH_Height)-30-3;
	gcd[gcdoff].gd.pos.width = -1;
	gcd[gcdoff].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[gcdoff].text = (unichar_t *) _("_OK");
	label[gcdoff].text_is_1byte = true;
	label[gcdoff].text_in_resource = true;
	gcd[gcdoff].gd.mnemonic = 'O';
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.handle_controlevent = Stroke_OK;
	gcd[gcdoff++].creator = GButtonCreate;

	gcd[gcdoff].gd.pos.x = -30; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+3;
	gcd[gcdoff].gd.pos.width = -1;
	gcd[gcdoff].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[gcdoff].text = (unichar_t *) _("_Cancel");
	label[gcdoff].text_is_1byte = true;
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
	StrokeSetup(sd,GGadgetIsChecked( GWidgetGetControl(sd->gw,CID_Caligraphic))?si_caligraphic:
		GGadgetIsChecked( GWidgetGetControl(sd->gw,CID_Stroke))?si_std:
		GGadgetIsChecked( GWidgetGetControl(sd->gw,CID_Elipse))?si_elipse:
		si_centerline);
    else
	StrokeSetup(sd,def->stroke_type);
    GDrawSetVisible(sd->gw,true);
    while ( !sd->done )
	GDrawProcessOneEvent(NULL);
    if ( strokeit!=NULL )
	GDrawSetVisible(sd->gw,false);
    else
	GDrawDestroyWindow(sd->gw);
}

void CVStroke(CharView *cv) {

    if ( cv->b.layerheads[cv->b.drawmode]->splines==NULL )
return;

    MakeStrokeDlg(cv,CVStrokeIt,NULL);
}

void SCStroke(SplineChar *sc) {
    MakeStrokeDlg(sc,SCStrokeIt,NULL);
}

void FVStroke(FontView *fv) {
    MakeStrokeDlg(fv,FVStrokeItScript,NULL);
}

void FreeHandStrokeDlg(StrokeInfo *si) {
    MakeStrokeDlg(NULL,NULL,si);
}

/* ************************************************************************** */
/* ****************************** Layer Dialog ****************************** */
/* ************************************************************************** */
#ifdef FONTFORGE_CONFIG_TYPE3

#define LY_Width	300
#define LY_Height	336

#define CID_FillColor		2001
#define CID_FillCInherit	2002
#define CID_FillOpacity		2003
#define CID_FillOInherit	2004
#define CID_StrokeColor		2005
#define CID_StrokeCInherit	2006
#define CID_StrokeOpacity	2007
#define CID_StrokeOInherit	2008
#define CID_StrokeWInherit	2009
#define CID_Trans		2010
#define CID_InheritCap		2011
#define CID_InheritJoin		2012
#define CID_Fill		2013
#define CID_Dashes		2014
#define CID_DashesTxt		2015
#define CID_DashesInherit	2016

struct layer_dlg {
    int done;
    int ok;
    Layer *layer;
    GWindow gw;
};

static uint32 getcol(GGadget *g,int *err) {
    const unichar_t *ret=_GGadgetGetTitle(g);
    unichar_t *end;
    uint32 col = COLOR_INHERITED;

    if ( *ret=='#' ) ++ret;
    col = u_strtol(ret,&end,16);
    if ( col<0 || col>0xffffff || *end!='\0' ) {
	*err = true;
	ff_post_error(_("Bad Color"),_("Bad Color"));
    }
return( col );
}

static int Layer_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	Layer temp;
	int err=false;
	const unichar_t *ret;
	unichar_t *end, *end2;
	int i;

	LayerDefault(&temp);
	temp.dofill = GGadgetIsChecked(GWidgetGetControl(gw,CID_Fill));
	temp.dostroke = GGadgetIsChecked(GWidgetGetControl(gw,CID_Stroke));
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_FillCInherit)) )
	    temp.fill_brush.col = COLOR_INHERITED;
	else
	    temp.fill_brush.col = getcol(GWidgetGetControl(gw,CID_FillColor),&err);
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_FillOInherit)) )
	    temp.fill_brush.opacity = -1;
	else
	    temp.fill_brush.opacity = GetReal8(gw,CID_FillOpacity,_("Opacity:"),&err);

	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_StrokeCInherit)) )
	    temp.stroke_pen.brush.col = COLOR_INHERITED;
	else
	    temp.stroke_pen.brush.col = getcol(GWidgetGetControl(gw,CID_StrokeColor),&err);
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_StrokeOInherit)) )
	    temp.stroke_pen.brush.opacity = -1;
	else
	    temp.stroke_pen.brush.opacity = GetReal8(gw,CID_StrokeOpacity,_("Opacity:"),&err);
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_StrokeWInherit)) )
	    temp.stroke_pen.width = WIDTH_INHERITED;
	else
	    temp.stroke_pen.width = GetReal8(gw,CID_Width,_("_Width"),&err);
	if ( err )
return( true );

	ret = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Trans));
	while ( *ret==' ' || *ret=='[' ) ++ret;
	temp.stroke_pen.trans[0] = u_strtod(ret,&end);
	temp.stroke_pen.trans[1] = u_strtod(end,&end);
	temp.stroke_pen.trans[2] = u_strtod(end,&end);
	temp.stroke_pen.trans[3] = u_strtod(end,&end2);
	for ( ret = end2 ; *ret==' ' || *ret==']' ; ++ret );
	if ( end2==end || *ret!='\0' || temp.stroke_pen.trans[0] ==0 || temp.stroke_pen.trans[3]==0 ) {
	    ff_post_error(_("Bad Transformation Matrix"),_("Bad Transformation Matrix"));
return( true );
	}
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_DashesInherit)) ) {
	    temp.stroke_pen.dashes[0] = 0; temp.stroke_pen.dashes[1] = DASH_INHERITED;
	} else {
	    ret = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Dashes));
	    while ( *ret==' ' || *ret=='[' ) ++ret;
	    for ( i=0; ; ++i ) {
		long val = u_strtol(ret,&end,10);
		if ( *end=='\0' )
	    break;
		if ( val<0 || val>255 ) {
		    ff_post_error(_("Bad dash list"),_("Value out of range"));
return( true );
		} else if ( *end!=' ' ) {
		    ff_post_error(_("Bad dash list"),_("Bad Number"));
return( true );
		} else if ( i>=DASH_MAX ) {
		    ff_post_error(_("Bad dash list"),_("Too many dashes (at most %d allowed)"), DASH_MAX);
return( true );
		}
		temp.stroke_pen.dashes[i] = val;
		ret = end;
		while ( *ret==' ' ) ++ret;
	    }
	    if ( i<DASH_MAX ) temp.stroke_pen.dashes[i] = 0;
	}
	temp.stroke_pen.linecap =
		GGadgetIsChecked(GWidgetGetControl(gw,CID_ButtCap))?lc_butt:
		GGadgetIsChecked(GWidgetGetControl(gw,CID_RoundCap))?lc_round:
		GGadgetIsChecked(GWidgetGetControl(gw,CID_SquareCap))?lc_square:
			lc_inherited;
	temp.stroke_pen.linejoin =
		GGadgetIsChecked(GWidgetGetControl(gw,CID_BevelJoin))?lj_bevel:
		GGadgetIsChecked(GWidgetGetControl(gw,CID_RoundJoin))?lj_round:
		GGadgetIsChecked(GWidgetGetControl(gw,CID_MiterJoin))?lj_miter:
			lj_inherited;
#ifdef FONTFORGE_CONFIG_TYPE3
	GradientFree(ld->layer->fill_brush.gradient);
	free(ld->layer->fill_brush.pattern);
	GradientFree(ld->layer->stroke_pen.brush.gradient);
	free(ld->layer->stroke_pen.brush.pattern);
#endif
	ld->done = ld->ok = true;
	ld->layer->stroke_pen = temp.stroke_pen;
	ld->layer->fill_brush = temp.fill_brush;
	ld->layer->dofill = temp.dofill;
	ld->layer->dostroke = temp.dostroke;
	ld->layer->fillfirst = temp.fillfirst;
    }
return( true );
}

static int Layer_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct layer_dlg *ld = GDrawGetUserData(GGadgetGetWindow(g));
	ld->done = true;
    }
return( true );
}

static int Layer_Inherit(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	int cid = (intpt) GGadgetGetUserData(g);
	GGadgetSetEnabled(GWidgetGetControl(gw,cid),
		!GGadgetIsChecked(g));
    }
return( true );
}

static int layer_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct layer_dlg *ld = GDrawGetUserData(gw);
	ld->done = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("multilayer.html#Layer");
return( true );
	}
return( false );
    }
return( true );
}

int LayerDialog(Layer *layer) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[41];
    GTextInfo label[41];
    struct layer_dlg ld;
    int yoff=0;
    int gcdoff, fill_gcd;
    char widthbuf[20], fcol[12], scol[12], fopac[30], sopac[30], transbuf[150],
	    dashbuf[60];
    int i;
    char *pt;

    memset(&ld,0,sizeof(ld));
    ld.layer = layer;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Layer");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,LY_Width));
    pos.height = GDrawPointsToPixels(NULL,LY_Height);
    ld.gw = gw = GDrawCreateTopWindow(NULL,&pos,layer_e_h,&ld,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    gcdoff = 0;

    label[gcdoff].text = (unichar_t *) _("Fi_ll");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = 5+yoff;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->dofill? gg_cb_on : 0);
    gcd[gcdoff].gd.cid = CID_Fill;
    gcd[gcdoff++].creator = GCheckBoxCreate;

    fill_gcd = gcdoff;
    gcd[gcdoff].gd.pos.x = 2; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+6;
    gcd[gcdoff].gd.pos.width = LY_Width-4; gcd[gcdoff].gd.pos.height = 65;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GGroupCreate;

    label[gcdoff].text = (unichar_t *) _("Color:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+12;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;

    sprintf( fcol, "#%06x", layer->fill_brush.col );
    label[gcdoff].text = (unichar_t *) fcol;
    label[gcdoff].text_is_1byte = true;
    if ( layer->fill_brush.col==COLOR_INHERITED ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_FillColor;
    gcd[gcdoff++].creator = GTextFieldCreate;

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->fill_brush.col==COLOR_INHERITED? gg_cb_on : 0);
    gcd[gcdoff].data = (void *) CID_FillColor;
    gcd[gcdoff].gd.cid = CID_FillCInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff++].creator = GCheckBoxCreate;

    label[gcdoff].text = (unichar_t *) _("Opacity:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;

    sprintf( fopac, "%g", layer->fill_brush.opacity );
    label[gcdoff].text = (unichar_t *) fopac;
    label[gcdoff].text_is_1byte = true;
    if ( layer->fill_brush.opacity<0 ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_FillOpacity;
    gcd[gcdoff++].creator = GTextFieldCreate;

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->fill_brush.opacity<0? gg_cb_on : 0);
    gcd[gcdoff].data = (void *) CID_FillOpacity;
    gcd[gcdoff].gd.cid = CID_FillOInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff++].creator = GCheckBoxCreate;

    label[gcdoff].text = (unichar_t *) _("Stroke");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.mnemonic = 'S';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[fill_gcd].gd.pos.y+gcd[fill_gcd].gd.pos.height+4;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->dostroke? gg_cb_on : 0);
    gcd[gcdoff].gd.cid = CID_Stroke;
    gcd[gcdoff++].creator = GCheckBoxCreate;

    gcd[gcdoff].gd.pos.x = 2; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+6;
    gcd[gcdoff].gd.pos.width = LY_Width-4; gcd[gcdoff].gd.pos.height = 206;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GGroupCreate;

    label[gcdoff].text = (unichar_t *) _("Color:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+12;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;

    sprintf( scol, "#%06x", layer->stroke_pen.brush.col );
    label[gcdoff].text = (unichar_t *) scol;
    label[gcdoff].text_is_1byte = true;
    if ( layer->stroke_pen.brush.col==COLOR_INHERITED ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_StrokeColor;
    gcd[gcdoff++].creator = GTextFieldCreate;

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.brush.col==COLOR_INHERITED? gg_cb_on : 0);
    gcd[gcdoff].data = (void *) CID_StrokeColor;
    gcd[gcdoff].gd.cid = CID_StrokeCInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff++].creator = GCheckBoxCreate;

    label[gcdoff].text = (unichar_t *) _("Opacity:");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.mnemonic = 'W';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;

    sprintf( sopac, "%g", layer->stroke_pen.brush.opacity );
    label[gcdoff].text = (unichar_t *) sopac;
    label[gcdoff].text_is_1byte = true;
    if ( layer->stroke_pen.brush.opacity<0 ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_StrokeOpacity;
    gcd[gcdoff++].creator = GTextFieldCreate;

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.brush.opacity<0? gg_cb_on : 0);
    gcd[gcdoff].data = (void *) CID_StrokeOpacity;
    gcd[gcdoff].gd.cid = CID_StrokeOInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff++].creator = GCheckBoxCreate;

    label[gcdoff].text = (unichar_t *) _("Stroke _Width:");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+26;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff].gd.cid = CID_WidthTxt;
    gcd[gcdoff++].creator = GLabelCreate;

    sprintf( widthbuf, "%g", layer->stroke_pen.width );
    label[gcdoff].text = (unichar_t *) widthbuf;
    label[gcdoff].text_is_1byte = true;
    if ( layer->stroke_pen.width==WIDTH_INHERITED ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_Width;
    gcd[gcdoff++].creator = GTextFieldCreate;

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.width==WIDTH_INHERITED? gg_cb_on : 0);
    gcd[gcdoff].data = (void *) CID_Width;
    gcd[gcdoff].gd.cid = CID_StrokeWInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff++].creator = GCheckBoxCreate;

    label[gcdoff].text = (unichar_t *) _("Dashes");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+26;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[gcdoff].gd.cid = CID_DashesTxt;
    gcd[gcdoff].gd.popup_msg = (unichar_t *) _("This specifies the dash pattern for a line.\nLeave this field blank for a solid line.\nOtherwise specify a list of up to 8 integers\n(between 0 and 255) which give the dash pattern\nin em-units. So \"10 10\" will draw the first\n10 units of a line, leave the next 10 blank,\ndraw the next 10, and so on.");
    gcd[gcdoff++].creator = GLabelCreate;

    pt = dashbuf; dashbuf[0] = '\0';
    for ( i=0; i<DASH_MAX && layer->stroke_pen.dashes[i]!=0; ++i ) {
	sprintf( pt, "%d ", layer->stroke_pen.dashes[i]);
	pt += strlen(pt);
    }
    if ( pt>dashbuf ) pt[-1] = '\0';
    label[gcdoff].text = (unichar_t *) dashbuf;
    label[gcdoff].text_is_1byte = true;
    if ( layer->stroke_pen.dashes[0]==0 && layer->stroke_pen.dashes[1]==DASH_INHERITED ) 
	gcd[gcdoff].gd.flags = gg_visible;
    else {
	gcd[gcdoff].gd.label = &label[gcdoff];
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    }
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 80;
    gcd[gcdoff].gd.cid = CID_Dashes;
    gcd[gcdoff++].creator = GTextFieldCreate;

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 165; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+2;
    if ( layer->stroke_pen.dashes[0]==0 && layer->stroke_pen.dashes[1]==DASH_INHERITED ) 
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | gg_cb_on | gg_utf8_popup;
    else
	gcd[gcdoff].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[gcdoff].data = (void *) CID_Dashes;
    gcd[gcdoff].gd.cid = CID_DashesInherit;
    gcd[gcdoff].gd.handle_controlevent = Layer_Inherit;
    gcd[gcdoff].gd.popup_msg = (unichar_t *) _("This specifies the dash pattern for a line.\nLeave this field blank for a solid line.\nOtherwise specify a list of up to 8 integers\n(between 0 and 255) which give the dash pattern\nin em-units. So \"10 10\" will draw the first\n10 units of a line, leave the next 10 blank,\ndraw the next 10, and so on.");
    gcd[gcdoff++].creator = GCheckBoxCreate;

    label[gcdoff].text = (unichar_t *) _("_Transform Pen:");
    label[gcdoff].text_in_resource = true;
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 5; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GLabelCreate;

    sprintf( transbuf, "[%.4g %.4g %.4g %.4g]", layer->stroke_pen.trans[0],
	    layer->stroke_pen.trans[1], layer->stroke_pen.trans[2],
	    layer->stroke_pen.trans[3]);
    label[gcdoff].text = (unichar_t *) transbuf;
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y-3;
    gcd[gcdoff].gd.pos.width = 210;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff].gd.cid = CID_Trans;
    gcd[gcdoff++].creator = GTextFieldCreate;

    label[gcdoff].text = (unichar_t *) _("Line Cap");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 10; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+20;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff].gd.cid = CID_LineCapTxt;
    gcd[gcdoff++].creator = GLabelCreate;

    gcd[gcdoff].gd.pos.x = 6; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+6;
    gcd[gcdoff].gd.pos.width = LY_Width-12; gcd[gcdoff].gd.pos.height = 25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GGroupCreate;

    label[gcdoff].text = (unichar_t *) _("_Butt");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_buttcap;
    gcd[gcdoff].gd.mnemonic = 'B';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 15; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+12;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linecap==lc_butt?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_ButtCap;
    gcd[gcdoff++].creator = GRadioCreate;

    label[gcdoff].text = (unichar_t *) _("_Round");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_roundcap;
    gcd[gcdoff].gd.mnemonic = 'R';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 80; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linecap==lc_round?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_RoundCap;
    gcd[gcdoff++].creator = GRadioCreate;

    label[gcdoff].text = (unichar_t *) _("S_quare");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_squarecap;
    gcd[gcdoff].gd.mnemonic = 'q';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 150; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linecap==lc_square?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_SquareCap;
    gcd[gcdoff++].creator = GRadioCreate;

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 220; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linecap==lc_inherited?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_InheritCap;
    gcd[gcdoff++].creator = GRadioCreate;

    label[gcdoff].text = (unichar_t *) _("Line Join");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = gcd[gcdoff-6].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-3].gd.pos.y+25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff].gd.cid = CID_LineJoinTxt;
    gcd[gcdoff++].creator = GLabelCreate;

    gcd[gcdoff].gd.pos.x = gcd[gcdoff-6].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+6;
    gcd[gcdoff].gd.pos.width = LY_Width-12; gcd[gcdoff].gd.pos.height = 25;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible;
    gcd[gcdoff++].creator = GGroupCreate;

    label[gcdoff].text = (unichar_t *) _("_Miter");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_miterjoin;
    gcd[gcdoff].gd.mnemonic = 'M';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = gcd[gcdoff-6].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y+12;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linejoin==lj_miter?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_MiterJoin;
    gcd[gcdoff++].creator = GRadioCreate;

    label[gcdoff].text = (unichar_t *) _("Ro_und");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_roundjoin;
    gcd[gcdoff].gd.mnemonic = 'u';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = gcd[gcdoff-6].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linejoin==lj_round?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_RoundJoin;
    gcd[gcdoff++].creator = GRadioCreate;

    label[gcdoff].text = (unichar_t *) _("Be_vel");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    label[gcdoff].image = &GIcon_beveljoin;
    gcd[gcdoff].gd.mnemonic = 'v';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = gcd[gcdoff-6].gd.pos.x; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linejoin==lj_bevel?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_BevelJoin;
    gcd[gcdoff++].creator = GRadioCreate;

    label[gcdoff].text = (unichar_t *) _("Inherited");
    label[gcdoff].text_is_1byte = true;
    gcd[gcdoff].gd.mnemonic = 'q';
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.pos.x = 220; gcd[gcdoff].gd.pos.y = gcd[gcdoff-2].gd.pos.y;
    gcd[gcdoff].gd.flags = gg_enabled | gg_visible | (layer->stroke_pen.linejoin==lj_inherited?gg_cb_on:0);
    gcd[gcdoff].gd.cid = CID_InheritCap;
    gcd[gcdoff++].creator = GRadioCreate;


    gcd[gcdoff].gd.pos.x = 30-3; gcd[gcdoff].gd.pos.y = LY_Height-30-3;
    gcd[gcdoff].gd.pos.width = -1;
    gcd[gcdoff].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[gcdoff].text = (unichar_t *) _("_OK");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_OK;
    gcd[gcdoff++].creator = GButtonCreate;

    gcd[gcdoff].gd.pos.x = -30; gcd[gcdoff].gd.pos.y = gcd[gcdoff-1].gd.pos.y+3;
    gcd[gcdoff].gd.pos.width = -1;
    gcd[gcdoff].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[gcdoff].text = (unichar_t *) _("_Cancel");
    label[gcdoff].text_is_1byte = true;
    label[gcdoff].text_in_resource = true;
    gcd[gcdoff].gd.label = &label[gcdoff];
    gcd[gcdoff].gd.handle_controlevent = Layer_Cancel;
    gcd[gcdoff].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);

    GWidgetHidePalettes();
    /*GWidgetIndicateFocusGadget(GWidgetGetControl(ld.gw,CID_Width));*/
    GDrawSetVisible(ld.gw,true);
    while ( !ld.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(ld.gw);
return( ld.ok );
}
#endif		/* TYPE3 */
