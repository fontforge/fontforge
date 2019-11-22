/* Copyright (C) 2003-2012 by George Williams */
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

#include "cvundoes.h"
#include "edgelist.h"
#include "effects.h"
#include "fontforgeui.h"
#include "gkeysym.h"
#include "splinestroke.h"
#include "splineutil2.h"
#include "ustring.h"
#include "utype.h"

#include <math.h>

static void CVOutline(CharView *cv, real width) {
    StrokeInfo si;
    SplineSet *temp, *spl;
    int changed;

    InitializeStrokeInfo(&si);
    si.removeexternal = true;
    si.width = width*2;

    CVPreserveState((CharViewBase *) cv);
    temp = SplineSetStroke(cv->b.layerheads[cv->b.drawmode]->splines,&si,cv->b.layerheads[cv->b.drawmode]->order2);
    for ( spl=cv->b.layerheads[cv->b.drawmode]->splines; spl->next!=NULL; spl=spl->next );
    spl->next = temp;
    SplineSetsCorrect(cv->b.layerheads[cv->b.drawmode]->splines,&changed);
    CVCharChangedUpdate(&cv->b);
}

static void MVOutline(MetricsView *mv, real width) {
    StrokeInfo si;
    SplineSet *temp, *spl;
    int i, changed;

    InitializeStrokeInfo(&si);
    si.removeexternal = true;
    si.width = width*2;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	SCPreserveLayer(sc,mv->layer,false);
	temp = SplineSetStroke(sc->layers[mv->layer].splines,&si,sc->layers[mv->layer].order2);
	for ( spl=sc->layers[mv->layer].splines; spl->next!=NULL; spl=spl->next );
	spl->next = temp;
	SplineSetsCorrect(sc->layers[mv->layer].splines,&changed);
	SCCharChangedUpdate(sc,mv->layer);
    }
}

static void CVInline(CharView *cv, real width, real inset) {
    StrokeInfo si;
    SplineSet *temp, *spl, *temp2;
    int changed;

    InitializeStrokeInfo(&si);
    si.removeexternal = true;

    CVPreserveState((CharViewBase *) cv);
    si.width = width*2;
    temp = SplineSetStroke(cv->b.layerheads[cv->b.drawmode]->splines,&si,cv->b.layerheads[cv->b.drawmode]->order2);
    si.width = (width+inset)*2;
    temp2 = SplineSetStroke(cv->b.layerheads[cv->b.drawmode]->splines,&si,cv->b.layerheads[cv->b.drawmode]->order2);
    for ( spl=cv->b.layerheads[cv->b.drawmode]->splines; spl->next!=NULL; spl=spl->next );
    spl->next = temp;
    for ( ; spl->next!=NULL; spl=spl->next );
    spl->next = temp2;
    SplineSetsCorrect(cv->b.layerheads[cv->b.drawmode]->splines,&changed);
    CVCharChangedUpdate(&cv->b);
}

static void MVInline(MetricsView *mv, real width, real inset) {
    StrokeInfo si;
    SplineSet *temp, *spl, *temp2;
    int i, changed;

    InitializeStrokeInfo(&si);
    si.removeexternal = true;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	SCPreserveLayer(sc,mv->layer,false);
	si.width = width*2;
	temp = SplineSetStroke(sc->layers[mv->layer].splines,&si,sc->layers[mv->layer].order2);
	si.width = (width+inset)*2;
	temp2 = SplineSetStroke(sc->layers[mv->layer].splines,&si,sc->layers[mv->layer].order2);
	for ( spl=sc->layers[mv->layer].splines; spl->next!=NULL; spl=spl->next );
	spl->next = temp;
	for ( ; spl->next!=NULL; spl=spl->next );
	spl->next = temp2;
	SplineSetsCorrect(sc->layers[mv->layer].splines,&changed);
	SCCharChangedUpdate(sc,mv->layer);
    }
}

static double def_outline_width = 10, def_gap_width = 20;

#define CID_Width	1000
#define CID_Gap		1001

typedef struct outlinedata {
    unsigned int done: 1;
    FontView *fv;
    CharView *cv;
    MetricsView *mv;
    int isinline;
    int wireframe;
    GWindow gw;
} OutlineData;

static int OD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	OutlineData *od = GDrawGetUserData(GGadgetGetWindow(g));
	real width, gap;
	int err = 0;

	width = GetReal8(od->gw,CID_Width,_("Outline Width"),&err);
	if ( od->isinline )
	    gap = GetReal8(od->gw,CID_Gap,_("_Gap:"),&err);
	if ( err )
return(true);
	def_outline_width = width;
	if ( od->isinline ) {
	    def_gap_width = gap;
	    if ( od->fv!=NULL )
		FVInline((FontViewBase *) od->fv,width,gap);
	    else if ( od->cv!=NULL )
		CVInline(od->cv,width,gap);
	    else if ( od->mv!=NULL )
		MVInline(od->mv,width,gap);
	} else {
	    if ( od->fv!=NULL )
		FVOutline((FontViewBase *) od->fv,width);
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
    GGadgetCreateData gcd[9], *harray[6], *butarray[7], *varray[7], boxes[4];
    GTextInfo label[9];
    OutlineData od;
    char buffer[20], buffer2[20];
    int i,k;

    od.done = false;
    od.fv = fv;
    od.cv = cv;
    od.mv = mv;
    od.isinline = isinline;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = isinline?_("Inline"):_("Outline");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,170));
	pos.height = GDrawPointsToPixels(NULL,75);
	od.gw = gw = GDrawCreateTopWindow(NULL,&pos,od_e_h,&od,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));

	i = k = 0;
	label[i].text = (unichar_t *) _("Outline Width:");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 7; gcd[i].gd.pos.y = 7+3; 
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;
	harray[k++] = &gcd[i-1];

	sprintf( buffer, "%g", def_outline_width );
	label[i].text = (unichar_t *) buffer;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 40; gcd[i].gd.pos.y = 7; gcd[i].gd.pos.width = 40;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = CID_Width;
	gcd[i++].creator = GTextFieldCreate;
	harray[k++] = &gcd[i-1];
	harray[k++] = GCD_Glue;

	if ( isinline ) {
	    label[i].text = (unichar_t *) _("_Gap:");
	    label[i].text_is_1byte = true;
	    label[i].text_in_resource = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 90; gcd[i].gd.pos.y = 7+3; 
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    gcd[i++].creator = GLabelCreate;
	    harray[k++] = &gcd[i-1];

	    sprintf( buffer2, "%g", def_gap_width );
	    label[i].text = (unichar_t *) buffer2;
	    label[i].text_is_1byte = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 120; gcd[i].gd.pos.y = 7;  gcd[i].gd.pos.width = 40;
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    gcd[i].gd.cid = CID_Gap;
	    gcd[i++].creator = GTextFieldCreate;
	    harray[k++] = &gcd[i-1];
	}
	harray[k] = NULL;

	k = 0;
	gcd[i].gd.pos.x = 20-3; gcd[i].gd.pos.y = 7+32;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_OK");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = OD_OK;
	gcd[i++].creator = GButtonCreate;
	butarray[k++] = GCD_Glue; butarray[k++] = &gcd[i-1]; butarray[k++] = GCD_Glue;

	gcd[i].gd.pos.x = -20; gcd[i].gd.pos.y = 7+32+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = OD_Cancel;
	gcd[i++].creator = GButtonCreate;
	butarray[k++] = GCD_Glue; butarray[k++] = &gcd[i-1]; butarray[k++] = GCD_Glue;
	butarray[k] = 0;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray;
	boxes[2].creator = GHBoxCreate;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = butarray;
	boxes[3].creator = GHBoxCreate;

	varray[0] = &boxes[2]; varray[1] = NULL;
	varray[2] = GCD_Glue; varray[3] = NULL;
	varray[4] = &boxes[3]; varray[5] = NULL;
	varray[6] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
	gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
	gcd[i].creator = GGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);

    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_Width));
    GTextFieldSelect(GWidgetGetControl(gw,CID_Width),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !od.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
}

static void CVShadow(CharView *cv,real angle, real outline_width,
	real shadow_length,int wireframe) {
    CVPreserveState((CharViewBase *) cv);
    cv->b.layerheads[cv->b.drawmode]->splines = SSShadow(cv->b.layerheads[cv->b.drawmode]->splines,angle,outline_width,shadow_length,cv->b.sc,wireframe);
    CVCharChangedUpdate(&cv->b);
}

static void MVShadow(MetricsView *mv,real angle, real outline_width,
	real shadow_length,int wireframe) {
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	SCPreserveLayer(sc,mv->layer,false);
	sc->layers[mv->layer].splines = SSShadow(sc->layers[mv->layer].splines,angle,outline_width,shadow_length,sc,wireframe);
	SCCharChangedUpdate(sc,mv->layer);
    }
}

static double def_shadow_len=100, def_sun_angle= -45;

#define CID_ShadowLen		1001
#define CID_LightAngle		1002

static int SD_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	OutlineData *od = GDrawGetUserData(GGadgetGetWindow(g));
	real width, angle, len;
	int err = 0;

	width = GetReal8(od->gw,CID_Width,_("Outline Width"),&err);
	len = GetReal8(od->gw,CID_ShadowLen,_("Shadow Length:"),&err);
	angle = GetReal8(od->gw,CID_LightAngle,_("Light Angle:"),&err);
	if ( err )
return(true);
	def_outline_width = width;
	def_shadow_len = len;
	def_sun_angle = angle;
	angle *= -FF_PI/180;
	angle -= FF_PI/2;
	if ( od->fv!=NULL )
	    FVShadow((FontViewBase *) od->fv,angle,width,len,od->wireframe);
	else if ( od->cv!=NULL )
	    CVShadow(od->cv,angle,width,len,od->wireframe);
	else if ( od->mv!=NULL )
	    MVShadow(od->mv,angle,width,len,od->wireframe);
	od->done = true;
    }
return( true );
}

void ShadowDlg(FontView *fv, CharView *cv,MetricsView *mv,int wireframe) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10], *butarray[7], *hvarray[16], boxes[3];
    GTextInfo label[10];
    OutlineData od;
    char buffer[20], buffer2[20], buffer3[20];
    int i, k;

    od.done = false;
    od.fv = fv;
    od.cv = cv;
    od.mv = mv;
    od.wireframe = wireframe;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Shadow");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,160));
	pos.height = GDrawPointsToPixels(NULL,125);
	od.gw = gw = GDrawCreateTopWindow(NULL,&pos,od_e_h,&od,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));

	i = k = 0;
	label[i].text = (unichar_t *) _("Outline Width:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 7; gcd[i].gd.pos.y = 7+3; 
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;
	hvarray[k++] = &gcd[i-1];

	sprintf( buffer, "%g", def_outline_width );
	label[i].text = (unichar_t *) buffer;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 90; gcd[i].gd.pos.y = 7; gcd[i].gd.pos.width = 50;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = CID_Width;
	gcd[i++].creator = GTextFieldCreate;
	hvarray[k++] = &gcd[i-1]; hvarray[k++] = NULL;

	label[i].text = (unichar_t *) _("Shadow Length:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;
	hvarray[k++] = &gcd[i-1];

	sprintf( buffer2, "%g", def_shadow_len );
	label[i].text = (unichar_t *) buffer2;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-3;  gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = CID_ShadowLen;
	gcd[i++].creator = GTextFieldCreate;
	hvarray[k++] = &gcd[i-1]; hvarray[k++] = NULL;

	label[i].text = (unichar_t *) _("Light Angle:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;
	hvarray[k++] = &gcd[i-1];

	sprintf( buffer3, "%g", def_sun_angle );
	label[i].text = (unichar_t *) buffer3;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-3;  gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = CID_LightAngle;
	gcd[i++].creator = GTextFieldCreate;
	hvarray[k++] = &gcd[i-1]; hvarray[k++] = NULL;
	hvarray[k++] = &boxes[2]; hvarray[k++] = GCD_ColSpan; hvarray[k++] = NULL;
	hvarray[k++] = GCD_Glue; hvarray[k++] = GCD_Glue; hvarray[k++] = NULL;
	hvarray[k] = NULL;

	k = 0;
	gcd[i].gd.pos.x = 20-3; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+30;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_OK");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = SD_OK;
	gcd[i++].creator = GButtonCreate;
	butarray[k++] = GCD_Glue; butarray[k++] = &gcd[i-1]; butarray[k++] = GCD_Glue;

	gcd[i].gd.pos.x = -20; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.handle_controlevent = OD_Cancel;
	gcd[i++].creator = GButtonCreate;
	butarray[k++] = GCD_Glue; butarray[k++] = &gcd[i-1]; butarray[k++] = GCD_Glue;
	butarray[k] = NULL;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = butarray;
	boxes[2].creator = GHBoxCreate;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = hvarray;
	boxes[0].creator = GHVGroupCreate;

	gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
	gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
	gcd[i].creator = GGroupCreate;

	GGadgetsCreate(gw,boxes);

    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);

    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_ShadowLen));
    GTextFieldSelect(GWidgetGetControl(gw,CID_ShadowLen),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !od.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
}
