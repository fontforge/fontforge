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
#include <utype.h>

void FVOutline(FontView *fv, real width) {
    StrokeInfo si;
    SplineSet *temp, *spl;
    int i, cnt=0, changed;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] && fv->sf->chars[i]->splines )
	++cnt;
    GProgressStartIndicatorR(10,_STR_Stroking,_STR_Stroking,0,cnt,1);

    memset(&si,0,sizeof(si));
    si.removeexternal = true;
    si.radius = width;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] && fv->sf->chars[i]->splines ) {
	SplineChar *sc = fv->sf->chars[i];
	SCPreserveState(sc,false);
	temp = SSStroke(sc->splines,&si,sc);
	for ( spl=sc->splines; spl->next!=NULL; spl=spl->next );
	spl->next = temp;
	SplineSetsCorrect(sc->splines,&changed);
	SCCharChangedUpdate(sc);
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static void CVOutline(CharView *cv, real width) {
    StrokeInfo si;
    SplineSet *temp, *spl;
    int changed;

    memset(&si,0,sizeof(si));
    si.removeexternal = true;
    si.radius = width;

    CVPreserveState(cv);
    temp = SSStroke(*cv->heads[cv->drawmode],&si,cv->sc);
    for ( spl=*cv->heads[cv->drawmode]; spl->next!=NULL; spl=spl->next );
    spl->next = temp;
    SplineSetsCorrect(*cv->heads[cv->drawmode],&changed);
    CVCharChangedUpdate(cv);
}

static void MVOutline(MetricsView *mv, real width) {
    StrokeInfo si;
    SplineSet *temp, *spl;
    int i, changed;

    memset(&si,0,sizeof(si));
    si.removeexternal = true;
    si.radius = width;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	SCPreserveState(sc,false);
	temp = SSStroke(sc->splines,&si,sc);
	for ( spl=sc->splines; spl->next!=NULL; spl=spl->next );
	spl->next = temp;
	SplineSetsCorrect(sc->splines,&changed);
	SCCharChangedUpdate(sc);
    }
}

void FVInline(FontView *fv, real width, real inset) {
    StrokeInfo si;
    SplineSet *temp, *spl, *temp2;
    int i, cnt=0, changed;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] && fv->sf->chars[i]->splines )
	++cnt;
    GProgressStartIndicatorR(10,_STR_Stroking,_STR_Stroking,0,cnt,1);

    memset(&si,0,sizeof(si));
    si.removeexternal = true;

    for ( i=0; i<fv->sf->charcnt; ++i ) if ( fv->sf->chars[i]!=NULL && fv->selected[i] && fv->sf->chars[i]->splines ) {
	SplineChar *sc = fv->sf->chars[i];
	SCPreserveState(sc,false);
	si.radius = width;
	temp = SSStroke(sc->splines,&si,sc);
	si.radius = width+inset;
	temp2 = SSStroke(sc->splines,&si,sc);
	for ( spl=sc->splines; spl->next!=NULL; spl=spl->next );
	spl->next = temp;
	for ( ; spl->next!=NULL; spl=spl->next );
	spl->next = temp2;
	SplineSetsCorrect(sc->splines,&changed);
	SCCharChangedUpdate(sc);
	if ( !GProgressNext())
    break;
    }
    GProgressEndIndicator();
}

static void CVInline(CharView *cv, real width, real inset) {
    StrokeInfo si;
    SplineSet *temp, *spl, *temp2;
    int changed;

    memset(&si,0,sizeof(si));
    si.removeexternal = true;

    CVPreserveState(cv);
    si.radius = width;
    temp = SSStroke(*cv->heads[cv->drawmode],&si,cv->sc);
    si.radius = width+inset;
    temp2 = SSStroke(*cv->heads[cv->drawmode],&si,cv->sc);
    for ( spl=*cv->heads[cv->drawmode]; spl->next!=NULL; spl=spl->next );
    spl->next = temp;
    for ( ; spl->next!=NULL; spl=spl->next );
    spl->next = temp2;
    SplineSetsCorrect(*cv->heads[cv->drawmode],&changed);
    CVCharChangedUpdate(cv);
}

static void MVInline(MetricsView *mv, real width, real inset) {
    StrokeInfo si;
    SplineSet *temp, *spl, *temp2;
    int i, changed;

    memset(&si,0,sizeof(si));
    si.removeexternal = true;

    for ( i=mv->charcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->perchar[i].sc;
	SCPreserveState(sc,false);
	si.radius = width;
	temp = SSStroke(sc->splines,&si,sc);
	si.radius = width+inset;
	temp2 = SSStroke(sc->splines,&si,sc);
	for ( spl=sc->splines; spl->next!=NULL; spl=spl->next );
	spl->next = temp;
	for ( ; spl->next!=NULL; spl=spl->next );
	spl->next = temp2;
	SplineSetsCorrect(sc->splines,&changed);
	SCCharChangedUpdate(sc);
    }
}

static real def_outline_width = 10, def_gap_width = 20;

#define CID_Width	1000
#define CID_Gap		1001

typedef struct outlinedata {
    unsigned int done: 1;
    FontView *fv;
    CharView *cv;
    MetricsView *mv;
    int isinline;
    GWindow gw;
} OutlineData;

static int OD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	OutlineData *od = GDrawGetUserData(GGadgetGetWindow(g));
	int width, gap;
	int err = 0;

	width = GetIntR(od->gw,CID_Width,_STR_Width,&err);
	if ( od->isinline )
	    gap = GetIntR(od->gw,CID_Gap,_STR_Gap,&err);
	if ( err )
return(true);
	def_outline_width = width;
	if ( od->isinline ) {
	    def_gap_width = gap;
	    if ( od->fv!=NULL )
		FVInline(od->fv,width,gap);
	    else if ( od->cv!=NULL )
		CVInline(od->cv,width,gap);
	    else if ( od->mv!=NULL )
		MVInline(od->mv,width,gap);
	} else {
	    if ( od->fv!=NULL )
		FVOutline(od->fv,width);
	    else if ( od->cv!=NULL )
		CVOutline(od->cv,width);
	    else if ( od->mv!=NULL )
		MVOutline(od->mv,width);
	}
	od->done = true;
    }
return( true );
}

static int OD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	OutlineData *od = GDrawGetUserData(GGadgetGetWindow(g));
	od->done = true;
    }
return( true );
}

static int od_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	OutlineData *od = GDrawGetUserData(gw);
	od->done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void OutlineDlg(FontView *fv, CharView *cv,MetricsView *mv,int isinline) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9];
    GTextInfo label[9];
    OutlineData od;
    char buffer[20], buffer2[20];
    int i;

    od.done = false;
    od.fv = fv;
    od.cv = cv;
    od.mv = mv;
    od.isinline = isinline;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(isinline?_STR_Inline:_STR_Outline,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,170));
	pos.height = GDrawPointsToPixels(NULL,75);
	od.gw = gw = GDrawCreateTopWindow(NULL,&pos,od_e_h,&od,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	i = 0;
	label[i].text = (unichar_t *) _STR_WidthC;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 7; gcd[i].gd.pos.y = 7+3; 
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	sprintf( buffer, "%g", def_outline_width );
	label[i].text = (unichar_t *) buffer;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 40; gcd[i].gd.pos.y = 7; gcd[i].gd.pos.width = 40;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = CID_Width;
	gcd[i++].creator = GTextFieldCreate;

	if ( isinline ) {
	    label[i].text = (unichar_t *) _STR_Gap;
	    label[i].text_in_resource = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 90; gcd[i].gd.pos.y = 7+3; 
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    gcd[i++].creator = GLabelCreate;

	    sprintf( buffer2, "%g", def_gap_width );
	    label[i].text = (unichar_t *) buffer2;
	    label[i].text_is_1byte = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 120; gcd[i].gd.pos.y = 7;  gcd[i].gd.pos.width = 40;
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    gcd[i].gd.cid = CID_Gap;
	    gcd[i++].creator = GTextFieldCreate;
	}

	gcd[i].gd.pos.x = 20-3; gcd[i].gd.pos.y = 7+32;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _STR_OK;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = OD_OK;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -20; gcd[i].gd.pos.y = 7+32+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _STR_Cancel;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = OD_Cancel;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
	gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
	gcd[i].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);

    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_Width));
    GTextFieldSelect(GWidgetGetControl(gw,CID_Width),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !od.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
}
