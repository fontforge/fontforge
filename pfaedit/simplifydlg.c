/* Copyright (C) 2002 by George Williams */
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
#include "ustring.h"

#define CID_Extrema	1000
#define CID_Slopes	1001
#define CID_Error	1002

static double olderr = .75;
static int oldextrema = false;
static int oldslopes = false;

typedef struct simplifyinfo {
    int flags;
    double err;
    int done;
    int cancelled;
} Simple;

static int Sim_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	Simple *sim = GDrawGetUserData(GGadgetGetWindow(g));
	int badparse=false;
	sim->err = GetRealR(GGadgetGetWindow(g),CID_Error,_STR_ErrorLimit,&badparse);
	if ( badparse )
return( true );
	olderr = sim->err;
	sim->flags = 0;
	if ( GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_Extrema)) )
	    sim->flags = sf_ignoreextremum;
	if ( GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_Slopes)) )
	    sim->flags |= sf_ignoreslopes;
	oldextrema = (sim->flags&sf_ignoreextremum);
	oldslopes = (sim->flags&sf_ignoreslopes);
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

int SimplifyDlg(double *err) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[17];
    GTextInfo label[17];
    Simple sim;
    char buffer[12];

    memset(&sim,0,sizeof(sim));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Reviewhints,NULL);
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,170));
    pos.height = GDrawPointsToPixels(NULL,115);
    gw = GDrawCreateTopWindow(NULL,&pos,sim_e_h,&sim,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_RemoveExtrema;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 8; gcd[0].gd.pos.y = 8; 
    gcd[0].gd.flags = gg_enabled|gg_visible;
    if ( oldextrema )
	gcd[0].gd.flags |= gg_cb_on;
    gcd[0].gd.popup_msg = GStringGetResource(_STR_RemoveExtremaPopup,NULL);
    gcd[0].gd.cid = CID_Extrema;
    gcd[0].creator = GCheckBoxCreate;

    label[1].text = (unichar_t *) _STR_ChangeSlopes;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 8; gcd[1].gd.pos.y = 26;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    if ( oldslopes )
	gcd[1].gd.flags |= gg_cb_on;
    gcd[1].gd.cid = CID_Slopes;
    gcd[1].gd.popup_msg = GStringGetResource(_STR_ChangeSlopesPopup,NULL);
    gcd[1].creator = GCheckBoxCreate;

    label[2].text = (unichar_t *) _STR_ErrorLimit;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 8; gcd[2].gd.pos.y = 44+6;
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    sprintf( buffer, "%g", olderr );
    label[3].text = (unichar_t *) buffer;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 70; gcd[3].gd.pos.y = gcd[2].gd.pos.y-6;
    gcd[3].gd.pos.width = 40;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_Error;
    gcd[3].creator = GTextFieldCreate;

    gcd[4].gd.pos.x = gcd[3].gd.pos.x+gcd[3].gd.pos.width+3;
    gcd[4].gd.pos.y = gcd[2].gd.pos.y;
    gcd[4].gd.flags = gg_visible | gg_enabled ;
    label[4].text = (unichar_t *) _STR_EmUnits;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].creator = GLabelCreate;

    gcd[5].gd.pos.x = 20-3; gcd[5].gd.pos.y = gcd[4].gd.pos.y+30;
    gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[5].text = (unichar_t *) _STR_OK;
    label[5].text_in_resource = true;
    gcd[5].gd.mnemonic = 'O';
    gcd[5].gd.label = &label[5];
    gcd[5].gd.handle_controlevent = Sim_OK;
    gcd[5].creator = GButtonCreate;

    gcd[6].gd.pos.x = -20; gcd[6].gd.pos.y = gcd[5].gd.pos.y+3;
    gcd[6].gd.pos.width = -1; gcd[6].gd.pos.height = 0;
    gcd[6].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[6].text = (unichar_t *) _STR_Cancel;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.mnemonic = 'C';
    gcd[6].gd.handle_controlevent = Sim_Cancel;
    gcd[6].creator = GButtonCreate;

    gcd[7].gd.pos.x = 2; gcd[7].gd.pos.y = 2;
    gcd[7].gd.pos.width = pos.width-4; gcd[7].gd.pos.height = pos.height-4;
    gcd[7].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[7].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GWidgetIndicateFocusGadget(GWidgetGetControl(gw,CID_Error));
    GTextFieldSelect(GWidgetGetControl(gw,CID_Error),0,-1);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !sim.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(gw,false);
    if ( sim.cancelled )
return( -1 );

    *err = sim.err;
return( sim.flags );
}
