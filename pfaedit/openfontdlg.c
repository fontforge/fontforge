/* Copyright (C) 2000,2001 by George Williams */
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
    }
return( event->type!=et_char );
}

unichar_t *FVOpenFont(const unichar_t *title, const unichar_t *defaultfile,
	const unichar_t *initial_filter, unichar_t **mimetypes,int mult) {
    GRect pos;
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
    bsbigger = 4*bs+4*14>295; totwid = bsbigger?4*bs+4*12:295;
    spacing = (totwid-4*bs-2*12)/3;
    pos.width = GDrawPointsToPixels(NULL,totwid);
    pos.height = GDrawPointsToPixels(NULL,223);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = totwid-24; gcd[0].gd.pos.height = 180;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    if ( RecentFiles[0]!=NULL )
	gcd[0].gd.flags = gg_visible | gg_enabled | gg_file_pulldown;
    if ( mult )
	gcd[0].gd.flags |= gg_file_multiple;
    gcd[0].creator = GFileChooserCreate;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = 192-3;
    gcd[1].gd.pos.width = GIntGetResource(_NUM_Buttonsize);
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _STR_OK;
    label[1].text_in_resource = true;
    gcd[1].gd.mnemonic = 'O';
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_Ok;
    gcd[1].creator = GButtonCreate;

    gcd[2].gd.pos.x = (totwid-spacing)/2-bs; gcd[2].gd.pos.y = 192;
    gcd[2].gd.pos.width = GIntGetResource(_NUM_Buttonsize);
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = (unichar_t *) _STR_New;
    label[2].text_in_resource = true;
    gcd[2].gd.mnemonic = 'N';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFD_New;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = (totwid+spacing)/2; gcd[3].gd.pos.y = 192;
    gcd[3].gd.pos.width = GIntGetResource(_NUM_Buttonsize);
    gcd[3].gd.flags = gg_visible | gg_enabled;
    label[3].text = (unichar_t *) _STR_Filter;
    label[3].text_in_resource = true;
    gcd[3].gd.mnemonic = 'F';
    gcd[3].gd.label = &label[3];
    gcd[3].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = totwid-bs-12; gcd[4].gd.pos.y = 192;
    gcd[4].gd.pos.width = GIntGetResource(_NUM_Buttonsize);
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[4].text = (unichar_t *) _STR_Cancel;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.mnemonic = 'C';
    gcd[4].gd.handle_controlevent = GFD_Cancel;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = 2; gcd[5].gd.pos.y = 2;
    gcd[5].gd.pos.width = pos.width-4; gcd[5].gd.pos.height = pos.height-4;
    gcd[5].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[5].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GGadgetSetUserData(gcd[3].ret,gcd[0].ret);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[3].ret);
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
