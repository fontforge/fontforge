/* Copyright (C) 2000-2004 by George Williams */
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
#include <stdlib.h>
#include <string.h>
#include "gdraw.h"
#include "gwidget.h"
#include "ggadget.h"
#include "pfaeditui.h"

struct gfc_data {
    int done;
    unichar_t *ret;
    GGadget *gfc;
};

static int GFD_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
	d->ret = GGadgetGetTitle(d->gfc);
    }
return( true );
}

static int GFD_New(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
	GDrawSetVisible(GGadgetGetWindow(g),false);
	FontNew();
    }
return( true );
}

static int GFD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfc_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_mousemove ||
	    (event->type==et_mousedown && event->u.mouse.button==3 )) {
	struct gfc_data *d = GDrawGetUserData(gw);
	GFileChooserPopupCheck(d->gfc,event);
    } else if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
	struct gfc_data *d = GDrawGetUserData(gw);
return( GGadgetDispatchEvent((GGadget *) (d->gfc),event));
    }
return( event->type!=et_char );
}

unichar_t *FVOpenFont(const unichar_t *title, const unichar_t *defaultfile,
	const unichar_t *initial_filter, unichar_t **mimetypes,int mult,
	int newok) {
    GRect pos;
    int i, filter;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[7];
    GTextInfo label[5];
    struct gfc_data d;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid, spacing;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = (unichar_t *) title;
    pos.x = pos.y = 0;
    if ( newok ) {
	totwid = GGadgetScale(295);
	bsbigger = 4*bs+4*14>totwid; totwid = bsbigger?4*bs+4*12:totwid;
	spacing = (totwid-4*bs-2*12)/3;
    } else {
	totwid = GGadgetScale(230);
	bsbigger = 3*bs+3*14>totwid; totwid = bsbigger?3*bs+3*12:totwid;
	spacing = (totwid-3*bs-2*12)/2;
    }
    pos.width = GDrawPointsToPixels(NULL,totwid);
    pos.height = GDrawPointsToPixels(NULL,223);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = totwid*100/GIntGetResource(_NUM_ScaleFactor)-24; gcd[0].gd.pos.height = 180;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    if ( RecentFiles[0]!=NULL )
	gcd[0].gd.flags = gg_visible | gg_enabled | gg_file_pulldown;
    if ( mult )
	gcd[0].gd.flags |= gg_file_multiple;
    gcd[0].creator = GFileChooserCreate;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = 192-3;
    gcd[1].gd.pos.width = -1;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _STR_OK;
    label[1].text_in_resource = true;
    gcd[1].gd.mnemonic = 'O';
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_Ok;
    gcd[1].creator = GButtonCreate;

    i=2;
    if ( newok ) {
	gcd[2].gd.pos.x = -(spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)-12; gcd[2].gd.pos.y = 192;
	gcd[2].gd.pos.width = -1;
	gcd[2].gd.flags = gg_visible | gg_enabled;
	label[2].text = (unichar_t *) _STR_New;
	label[2].text_in_resource = true;
	gcd[2].gd.mnemonic = 'N';
	gcd[2].gd.label = &label[2];
	gcd[2].gd.handle_controlevent = GFD_New;
	gcd[2].creator = GButtonCreate;
	i=3;
    }

    filter = i;
    gcd[i].gd.pos.x = (spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)+12; gcd[i].gd.pos.y = 192;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) _STR_Filter;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'F';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -12; gcd[i].gd.pos.y = 192;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _STR_Cancel;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.handle_controlevent = GFD_Cancel;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
    gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
    gcd[i].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[i++].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GGadgetSetUserData(gcd[filter].ret,gcd[0].ret);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[filter].ret);
    GFileChooserSetFilterText(gcd[0].ret,initial_filter);
    GFileChooserSetMimetypes(gcd[0].ret,mimetypes);
    if ( RecentFiles[0]!=NULL ) {
	GGadget *tf; 
	GFileChooserGetChildren(gcd[0].ret,NULL, NULL, &tf);
	GGadgetSetList(tf,GTextInfoFromChars(RecentFiles,RECENT_MAX),false);
    }
    GGadgetSetTitle(gcd[0].ret,defaultfile);

    memset(&d,'\0',sizeof(d));
    d.gfc = gcd[0].ret;

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GDrawProcessPendingEvents(NULL);		/* Give the window a chance to vanish... */
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);		/* Give the window a chance to vanish... */
return(d.ret);
}
