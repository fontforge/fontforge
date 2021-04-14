/* Copyright (C) 2009-2012 by George Williams */
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

#include "autowidth2.h"
#include "fontforgeui.h"
#include "gkeysym.h"
#include "ustring.h"
#include "utype.h"

#include <math.h>

static int width_last_em_size=1000;
static int width_separation=150;
static int width_min_side_bearing=10;
static int width_max_side_bearing=300;
static int width_chunk_height=10;
static int width_loop_cnt=1;

#define CID_Separation	1001
#define CID_MinSep	1002
#define CID_MaxSep	1003
#define CID_Height	1004
#define CID_Loop	1005

struct widthinfo {
    int done;
    GWindow gw;
    FontView *fv;
    SplineFont *sf;
};

static int AW2_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct widthinfo *wi = GDrawGetUserData(gw);
	int err = false;
	int separation, min_side, max_side, height, loop;

	separation = GetInt8(gw,CID_Separation, _("Separation"),&err);
	min_side = GetInt8(gw,CID_MinSep, _("Min Bearing"),&err);
	max_side = GetInt8(gw,CID_MaxSep, _("Max Bearing"),&err);
	height = GetInt8(gw,CID_Height, _("Height"),&err);
	loop = GetInt8(gw,CID_Loop, _("Loop Count"),&err);
	if ( err )
return( true );

	GDrawSetVisible(gw,false);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);

	width_last_em_size     = wi->sf->ascent + wi->sf->descent;
	width_separation       = separation;
	wi->sf->width_separation=separation;
	if ( wi->sf->italicangle==0 )
	    width_min_side_bearing = min_side;
	width_max_side_bearing = max_side;
	width_chunk_height     = height;
	width_loop_cnt         = loop;

	AutoWidth2((FontViewBase *) wi->fv,separation,min_side,max_side,
		height, loop);
	wi->done = true;
    }
return( true );
}

static int AW2_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct widthinfo *wi = GDrawGetUserData(gw);
	wi->done = true;
    }
return( true );
}

static int AW2_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct widthinfo *wi = GDrawGetUserData(gw);
	wi->done = true;
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    /*struct widthinfo *wi = GDrawGetUserData(gw);*/
	    help("ui/dialogs/autowidth.html", NULL);
return( true );
	}
return( false );
    }
return( true );
}

void FVAutoWidth2(FontView *fv) {
    struct widthinfo wi;
    GWindow gw;
    GWindowAttrs wattrs;
    GRect pos;
    GGadgetCreateData *harray1[4], *harray2[6], *harray3[6], *barray[9], *varray[14];
    GGadgetCreateData gcd[29], boxes[6];
    GTextInfo label[29];
    int i,v;
    char sepbuf[20], minbuf[20], maxbuf[20], hbuf[20], lbuf[20];
    SplineFont *sf = fv->b.sf;
    double emsize = (sf->ascent + sf->descent);

    memset(&wi,0,sizeof(wi));
    wi.fv = fv;
    wi.sf = sf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.is_dlg = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Auto Width");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,200));
    pos.height = GDrawPointsToPixels(NULL,180);
    wi.gw = gw = GDrawCreateTopWindow(NULL,&pos,AW2_e_h,&wi,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&boxes,0,sizeof(boxes));
    memset(&gcd,0,sizeof(gcd));

    i = v = 0;

    label[i].text = (unichar_t *) _(
	"FontForge will attempt to adjust the left and right\n"
	"sidebearings of the selected glyphs so that the average\n"
	"separation between glyphs in a script will be the\n"
	"specified amount. You may also specify a minimum and\n"
	"maximum value for each glyph's sidebearings." );
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i++].creator = GLabelCreate;
    varray[v++] = &gcd[i-1]; varray[v++] = NULL;

    label[i].text = (unichar_t *) _( "_Separation:" );
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i++].creator = GLabelCreate;
    harray1[0] = &gcd[i-1];

    if ( sf->width_separation>0 )
	sprintf( sepbuf, "%d", sf->width_separation );
    else
	sprintf( sepbuf, "%d", (int) rint( width_separation * emsize / width_last_em_size ));
    label[i].text = (unichar_t *) sepbuf;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.cid = CID_Separation;
    gcd[i++].creator = GTextFieldCreate;
    harray1[1] = &gcd[i-1]; harray1[2] = GCD_Glue; harray1[3] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray1;
    boxes[2].creator = GHBoxCreate;
    varray[v++] = &boxes[2]; varray[v++] = NULL;

    label[i].text = (unichar_t *) _( "_Min:" );
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i++].creator = GLabelCreate;
    harray2[0] = &gcd[i-1];

    if ( sf->italicangle<0 )
	sprintf( minbuf, "%d", (int) rint( sf->descent*tan(sf->italicangle*FF_PI/180 )) );
    else if ( sf->italicangle>0 )
	sprintf( minbuf, "%d", (int) -rint( sf->ascent*tan(sf->italicangle*FF_PI/180 )) );
    else
	sprintf( minbuf, "%d", (int) rint( width_min_side_bearing * emsize / width_last_em_size ));
    label[i].text = (unichar_t *) minbuf;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.cid = CID_MinSep;
    gcd[i++].creator = GTextFieldCreate;
    harray2[1] = &gcd[i-1];

    label[i].text = (unichar_t *) _( "Ma_x:" );
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i++].creator = GLabelCreate;
    harray2[2] = &gcd[i-1];

    sprintf( maxbuf, "%d", (int) rint( width_max_side_bearing * emsize / width_last_em_size ));
    label[i].text = (unichar_t *) maxbuf;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    gcd[i].gd.cid = CID_MaxSep;
    gcd[i++].creator = GTextFieldCreate;
    harray2[3] = &gcd[i-1]; harray2[4] = GCD_Glue; harray2[5] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = harray2;
    boxes[3].creator = GHBoxCreate;
    varray[v++] = &boxes[3]; varray[v++] = NULL;

    label[i].text = (unichar_t *) _( "_Height:" );
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = /* gg_visible |*/ gg_enabled;
    gcd[i++].creator = GLabelCreate;
    harray3[0] = &gcd[i-1];

    sprintf( hbuf, "%d", (int) rint( width_chunk_height * emsize / width_last_em_size ));
    label[i].text = (unichar_t *) hbuf;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = /* gg_visible |*/ gg_enabled;
    gcd[i].gd.cid = CID_Height;
    gcd[i++].creator = GTextFieldCreate;
    harray3[1] = &gcd[i-1];

    label[i].text = (unichar_t *) _( "_Loops:" );
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = /* gg_visible |*/ gg_enabled;
    gcd[i++].creator = GLabelCreate;
    harray3[2] = &gcd[i-1];

    sprintf( lbuf, "%d", (int) rint( width_loop_cnt * emsize / width_last_em_size ));
    label[i].text = (unichar_t *) lbuf;
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 6;
    gcd[i].gd.flags = /* gg_visible |*/ gg_enabled;
    gcd[i].gd.cid = CID_Loop;
    gcd[i++].creator = GTextFieldCreate;
    harray3[3] = &gcd[i-1]; harray3[4] = GCD_Glue; harray3[5] = NULL;

    boxes[4].gd.flags = gg_enabled/*|gg_visible*/;
    boxes[4].gd.u.boxelements = harray3;
    boxes[4].creator = GHBoxCreate;
    varray[v++] = &boxes[4]; varray[v++] = NULL;

    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = AW2_OK;
    gcd[i++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[i-1]; barray[2] = GCD_Glue;

    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = AW2_Cancel;
    gcd[i++].creator = GButtonCreate;
    barray[3] = barray[4] = GCD_Glue;
    barray[5] = &gcd[i-1]; barray[6] = GCD_Glue; barray[7] = NULL;

    boxes[5].gd.flags = gg_enabled|gg_visible;
    boxes[5].gd.u.boxelements = barray;
    boxes[5].creator = GHBoxCreate;
    varray[v++] = GCD_Glue; varray[v++] = NULL;
    varray[v++] = &boxes[5]; varray[v++] = NULL; varray[v++] = NULL;

    boxes[0].gd.pos.x = gcd[i].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[5].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);

    GWidgetIndicateFocusGadget(gcd[2].ret);
    GTextFieldSelect(gcd[2].ret,0,-1);
    GDrawSetVisible(gw,true);
    while ( !wi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}
