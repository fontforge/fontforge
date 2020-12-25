/* Copyright (C) 2002-2012 by George Williams */
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

#include "fontforgeui.h"
#include "ustring.h"

#define CID_Extrema	1000
#define CID_Slopes	1001
#define CID_Error	1002
#define CID_Smooth	1003
#define CID_SmoothTan	1004
#define CID_SmoothHV	1005
#define CID_FlattenBumps	1006
#define CID_FlattenBound	1007
#define CID_LineLenMax		1008
#define CID_Start		1009
#define CID_SetAsDefault	1010

static double olderr_rat = 1/1000., oldsmooth_tan=.2,
	oldlinefixup_rat = 10./1000., oldlinelenmax_rat = 1/100.;
static int set_as_default = true;

static int oldextrema = false;
static int oldslopes = false;
static int oldsmooth = true;
static int oldsmoothhv = true;
static int oldlinefix = false;
static int oldstart = false;

typedef struct simplifydlg {
    int flags;
    double err;
    double tan_bounds;
    double linefixup;
    double linelenmax;
    int done;
    int cancelled;
    int em_size;
    int set_as_default;
} Simple;

static int Sim_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	Simple *sim = GDrawGetUserData(GGadgetGetWindow(g));
	int badparse=false;
	sim->flags = 0;
	if ( GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_Extrema)) )
	    sim->flags = sf_ignoreextremum;
	if ( GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_Slopes)) )
	    sim->flags |= sf_ignoreslopes;
	if ( GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_Smooth)) )
	    sim->flags |= sf_smoothcurves;
	if ( GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_SmoothHV)) )
	    sim->flags |= sf_choosehv;
	if ( GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_FlattenBumps)) )
	    sim->flags |= sf_forcelines;
	if ( GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_Start)) )
	    sim->flags |= sf_setstart2extremum;
	sim->err = GetReal8(GGadgetGetWindow(g),CID_Error,_("_Error Limit:"),&badparse);
	if ( sim->flags&sf_smoothcurves )
	    sim->tan_bounds= GetReal8(GGadgetGetWindow(g),CID_SmoothTan,_("_Tangent"),&badparse);
	if ( sim->flags&sf_forcelines )
	    sim->linefixup= GetReal8(GGadgetGetWindow(g),CID_FlattenBound,_("Bump Size"),&badparse);
	sim->linelenmax = GetReal8(GGadgetGetWindow(g),CID_LineLenMax,_("Line length max"),&badparse);
	if ( badparse )
return( true );
	olderr_rat = sim->err/sim->em_size;
	oldextrema = (sim->flags&sf_ignoreextremum);
	oldslopes = (sim->flags&sf_ignoreslopes);
	oldsmooth = (sim->flags&sf_smoothcurves);
	oldlinefix = (sim->flags&sf_forcelines);
	oldstart = (sim->flags&sf_setstart2extremum);
	if ( oldsmooth ) {
	    oldsmooth_tan = sim->tan_bounds;
	    oldsmoothhv = (sim->flags&sf_choosehv);
	}
	if ( oldlinefix )
	    oldlinefixup_rat = sim->linefixup/sim->em_size;
	oldlinelenmax_rat = sim->linelenmax/sim->em_size;
	sim->set_as_default = GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_SetAsDefault) );

	sim->done = true;
    }
return( true );
}

static int Sim_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	Simple *sim = GDrawGetUserData(GGadgetGetWindow(g));
	sim->done = sim->cancelled = true;
    }
return( true );
}

static int sim_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	Simple *sim = GDrawGetUserData(gw);
	sim->done = sim->cancelled = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

int SimplifyDlg(SplineFont *sf, struct simplifyinfo *smpl) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[24], boxes[7], *harray1[6], *harray2[6], *harray3[6],
	    *harray4[4][6], *barray[9], *varray[18][2];
    GTextInfo label[24];
    Simple sim;
    char buffer[12], buffer2[12], buffer3[12], buffer4[12];
    int k,l;

    memset(&sim,0,sizeof(sim));
    sim.em_size = sf->ascent+sf->descent;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Simplify");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,180));
    pos.height = GDrawPointsToPixels(NULL,275);
    gw = GDrawCreateTopWindow(NULL,&pos,sim_e_h,&sim,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    k=l=0;
    label[k].text = (unichar_t *) _("_Error Limit:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 12;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;
    harray1[0] = &gcd[k-1];

    sprintf( buffer, "%.3g", olderr_rat*sim.em_size );
    label[k].text = (unichar_t *) buffer;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 70; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-6;
    gcd[k].gd.pos.width = 40;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_Error;
    gcd[k++].creator = GTextFieldCreate;
    harray1[1] = &gcd[k-1];

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+gcd[k-1].gd.pos.width+3;
    gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled ;
    label[k].text = (unichar_t *) _("em-units");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GLabelCreate;
    harray1[2] = &gcd[k-1]; harray1[3] = GCD_Glue; harray1[4] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray1;
    boxes[2].creator = GHBoxCreate;
    varray[l][0] = &boxes[2]; varray[l++][1] = NULL;

    label[k].text = (unichar_t *) _("Allow _removal of extrema");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y+24;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    if ( oldextrema )
	gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.popup_msg = _("Normally simplify will not remove points at the extrema of curves\n(both PostScript and TrueType suggest you retain these points)");
    gcd[k].gd.cid = CID_Extrema;
    gcd[k++].creator = GCheckBoxCreate;
    varray[l][0] = &gcd[k-1]; varray[l++][1] = NULL;

    label[k].text = (unichar_t *) _("Allow _slopes to change");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    if ( oldslopes )
	gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Slopes;
    gcd[k].gd.popup_msg = _("Normally simplify will not change the slope of the contour at the points.");
    gcd[k++].creator = GCheckBoxCreate;
    varray[l][0] = &gcd[k-1]; varray[l++][1] = NULL;

    label[k].text = (unichar_t *) _("Start contours at e_xtrema");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    if ( oldstart )
	gcd[k].gd.flags |= gg_cb_on;
    gcd[k].gd.cid = CID_Start;
    gcd[k].gd.popup_msg = _("If the start point of a contour is not an extremum, find a new start point (on the contour) which is.");
    gcd[k++].creator = GCheckBoxCreate;
    varray[l][0] = &gcd[k-1]; varray[l++][1] = NULL;

    gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y + 20;
    gcd[k].gd.pos.width = 150;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLineCreate;
    varray[l][0] = &gcd[k-1]; varray[l++][1] = NULL;

    label[k].text = (unichar_t *) _("Allow _curve smoothing");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+4;
    if ( sf->layers[ly_fore].order2 )
	gcd[k].gd.flags = gg_visible;
    else {
	gcd[k].gd.flags = gg_enabled|gg_visible;
	if ( oldsmooth )
	    gcd[k].gd.flags |= gg_cb_on;
    }
    gcd[k].gd.popup_msg = _("Simplify will examine corner points whose control points are almost\ncolinear and smooth them into curve points");
    gcd[k].gd.cid = CID_Smooth;
    gcd[k++].creator = GCheckBoxCreate;
    varray[l][0] = &gcd[k-1]; varray[l++][1] = NULL;

/* GT: here "tan" means trigonometric tangent */
    label[k].text = (unichar_t *) _("if tan less than");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 20; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    if ( sf->layers[ly_fore].order2 ) gcd[k].gd.flags = gg_visible;
    gcd[k++].creator = GLabelCreate;
    harray2[0] = GCD_HPad10; harray2[1] = &gcd[k-1];

    sprintf( buffer2, "%.3g", oldsmooth_tan );
    label[k].text = (unichar_t *) buffer2;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 94; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-6;
    gcd[k].gd.pos.width = 40;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    if ( sf->layers[ly_fore].order2 ) gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_SmoothTan;
    gcd[k++].creator = GTextFieldCreate;
    harray2[2] = &gcd[k-1]; harray2[3] = GCD_Glue; harray2[4] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray2;
    boxes[3].creator = GHBoxCreate;
    varray[l][0] = &boxes[3]; varray[l++][1] = NULL;

    label[k].text = (unichar_t *) _("S_nap to horizontal/vertical");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 17; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24; 
    if ( sf->layers[ly_fore].order2 )
	gcd[k].gd.flags = gg_visible;
    else {
	gcd[k].gd.flags = gg_enabled|gg_visible;
	if ( oldsmoothhv )
	    gcd[k].gd.flags |= gg_cb_on;
    }
    gcd[k].gd.popup_msg = _("If the slope of an adjusted point is near horizontal or vertical\nsnap to that");
    gcd[k].gd.cid = CID_SmoothHV;
    gcd[k++].creator = GCheckBoxCreate;
    harray3[0] = GCD_HPad10; harray3[1] = &gcd[k-1]; harray3[2] = GCD_Glue; harray3[3] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = harray3;
    boxes[4].creator = GHBoxCreate;
    varray[l][0] = &boxes[4]; varray[l++][1] = NULL;

    label[k].text = (unichar_t *) _("_Flatten bumps on lines");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14; 
    if ( sf->layers[ly_fore].order2 )
	gcd[k].gd.flags = gg_visible;
    else {
	gcd[k].gd.flags = gg_enabled|gg_visible;
	if ( oldlinefix )
	    gcd[k].gd.flags |= gg_cb_on;
    }
    gcd[k].gd.popup_msg = _("If a line has a bump on it then flatten out that bump");
    gcd[k].gd.cid = CID_FlattenBumps;
    gcd[k++].creator = GCheckBoxCreate;
    varray[l][0] = &gcd[k-1]; varray[l++][1] = NULL;

    label[k].text = (unichar_t *) _("if smaller than");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 20; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    if ( sf->layers[ly_fore].order2 ) gcd[k].gd.flags = gg_visible;
    gcd[k++].creator = GLabelCreate;
    harray4[0][0] = GCD_HPad10; harray4[0][1] = &gcd[k-1];

    sprintf( buffer3, "%.3g", oldlinefixup_rat*sim.em_size );
    label[k].text = (unichar_t *) buffer3;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 90; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-6;
    gcd[k].gd.pos.width = 40;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    if ( sf->layers[ly_fore].order2 ) gcd[k].gd.flags = gg_visible;
    gcd[k].gd.cid = CID_FlattenBound;
    gcd[k++].creator = GTextFieldCreate;
    harray4[0][2] = &gcd[k-1];

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+gcd[k-1].gd.pos.width+3;
    gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled ;
    if ( sf->layers[ly_fore].order2 ) gcd[k].gd.flags = gg_visible;
    label[k].text = (unichar_t *) _("em-units");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GLabelCreate;
    harray4[0][3] = &gcd[k-1]; harray4[0][4] = GCD_Glue; harray4[0][5] = NULL;


    label[k].text = (unichar_t *) _("Don't smooth lines");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 8; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+14; 
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;
    harray4[1][0] = &gcd[k-1]; harray4[1][1] = harray4[1][2] = harray4[1][3] = GCD_ColSpan;
    harray4[1][4] = GCD_Glue; harray4[1][5] = NULL;

    label[k].text = (unichar_t *) _("longer than");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 20; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+24;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLabelCreate;
    harray4[2][0] = GCD_HPad10; harray4[2][1] = &gcd[k-1];

    sprintf( buffer4, "%.3g", oldlinelenmax_rat*sim.em_size );
    label[k].text = (unichar_t *) buffer4;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 90; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-6;
    gcd[k].gd.pos.width = 40;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k].gd.cid = CID_LineLenMax;
    gcd[k++].creator = GTextFieldCreate;
    harray4[2][2] = &gcd[k-1];

    gcd[k].gd.pos.x = gcd[k-1].gd.pos.x+gcd[k-1].gd.pos.width+3;
    gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
    gcd[k].gd.flags = gg_visible | gg_enabled ;
    label[k].text = (unichar_t *) _("em-units");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k++].creator = GLabelCreate;
    harray4[2][3] = &gcd[k-1]; harray4[2][4] = GCD_Glue; harray4[2][5] = NULL;
    harray4[3][0] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = harray4[0];
    boxes[5].creator = GHVBoxCreate;
    varray[l][0] = &boxes[5]; varray[l++][1] = NULL;

    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+20;
    gcd[k].gd.flags = gg_visible | gg_enabled | (set_as_default ? gg_cb_on : 0);
    label[k].text = (unichar_t *) _("Set as Default");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = CID_SetAsDefault;
    gcd[k++].creator = GCheckBoxCreate;
    varray[l][0] = &gcd[k-1]; varray[l++][1] = NULL;

    gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y + 20;
    gcd[k].gd.pos.width = 150;
    gcd[k].gd.flags = gg_enabled|gg_visible;
    gcd[k++].creator = GLineCreate;
    varray[l][0] = &gcd[k-1]; varray[l++][1] = NULL;

    gcd[k].gd.pos.x = 20-3; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.mnemonic = 'O';
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = Sim_OK;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[k-1]; barray[2] = GCD_Glue;

    gcd[k].gd.pos.x = -20; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.mnemonic = 'C';
    gcd[k].gd.handle_controlevent = Sim_Cancel;
    gcd[k++].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[k-1]; barray[5] = GCD_Glue; barray[6] = NULL;

    boxes[6].gd.flags = gg_enabled|gg_visible;
    boxes[6].gd.u.boxelements = barray;
    boxes[6].creator = GHBoxCreate;
    varray[l][0] = &boxes[6]; varray[l++][1] = NULL;
    varray[l][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray[0];
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_Error));
    GTextFieldSelect(GWidgetGetControl(gw,CID_Error),0,-1);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[5].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[6].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw,true);
    while ( !sim.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    if ( sim.cancelled )
return( false );

    smpl->flags = sim.flags;
    smpl->err = sim.err;
    smpl->tan_bounds = sim.tan_bounds;
    smpl->linefixup = sim.linefixup;
    smpl->linelenmax = sim.linelenmax;
    smpl->set_as_default = sim.set_as_default;
    set_as_default = sim.set_as_default;
return( true );
}
