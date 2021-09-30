/* Copyright (C) 2000-2012 by George Williams */
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

#include "gdraw.h"
#include "ggadget.h"
#include "ggadgetP.h"
#include "gwidget.h"
#include "ustring.h"

struct gfc_data {
    int done;
    unichar_t *ret;
    GGadget *gfc;
};

static int GFD_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *tf;
	GFileChooserGetChildren(d->gfc,NULL,NULL,&tf);
	if ( *_GGadgetGetTitle(tf)!='\0' ) {
	    d->done = true;
	    d->ret = GGadgetGetTitle(d->gfc);
	}
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
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	struct gfc_data *d = GDrawGetUserData(gw);
return( GGadgetDispatchEvent((GGadget *) (d->gfc),event));
    }
return( true );
}

static unichar_t *GWidgetOpenFileWPath(const unichar_t *title, const unichar_t *defaultfile,
	const unichar_t *initial_filter, unichar_t **mimetypes,
	GFileChooserFilterType filter, const char* const* path) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[7], boxes[3], *varray[5], *harray[8];
    GTextInfo label[4];
    struct gfc_data d;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid;

    GProgressPauseTimer();
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.is_dlg = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = (unichar_t *) title;
    pos.x = pos.y = 0;
    totwid = GGadgetScale(223);
    bsbigger = 3*bs+4*14>totwid; totwid = bsbigger?3*bs+4*12:totwid;
    pos.width = GDrawPointsToPixels(NULL,totwid);
    pos.height = GDrawPointsToPixels(NULL,223);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6;
    gcd[0].gd.pos.width = 223; gcd[0].gd.pos.height = 180;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GFileChooserCreate;
    varray[0] = &gcd[0]; varray[1] = NULL;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = 192-3;
    gcd[1].gd.pos.width = -1;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = (unichar_t *) _("_OK");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.mnemonic = 'O';
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_Ok;
    gcd[1].creator = GButtonCreate;
    harray[0] = GCD_Glue; harray[1] = &gcd[1];

    gcd[2].gd.pos.x = (totwid-bs)*100/GIntGetResource(_NUM_ScaleFactor)/2; gcd[2].gd.pos.y = gcd[1].gd.pos.y+3;
    gcd[2].gd.pos.width = -1;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = (unichar_t *) _("_Filter");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.mnemonic = 'F';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[2].creator = GButtonCreate;
    harray[2] = GCD_Glue; harray[3] = &gcd[2];

    gcd[3].gd.pos.x = -gcd[1].gd.pos.x; gcd[3].gd.pos.y = gcd[2].gd.pos.y;
    gcd[3].gd.pos.width = -1;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _("_Cancel");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.mnemonic = 'C';
    gcd[3].gd.handle_controlevent = GFD_Cancel;
    gcd[3].creator = GButtonCreate;
    harray[4] = GCD_Glue; harray[5] = &gcd[3]; harray[6] = GCD_Glue; harray[7] = NULL;

    boxes[2].gd.flags = gg_visible | gg_enabled;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;
    varray[2] = &boxes[2]; varray[3] = NULL;
    varray[4] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_visible | gg_enabled;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    gcd[4].gd.pos.x = 2; gcd[4].gd.pos.y = 2;
    gcd[4].gd.pos.width = pos.width-4; gcd[4].gd.pos.height = pos.height-4;
    gcd[4].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[4].creator = GGroupCreate;

    GGadgetsCreate(gw,boxes);
    GGadgetSetUserData(gcd[2].ret,gcd[0].ret);
    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[2].ret);
    GFileChooserSetFilterText(gcd[0].ret,initial_filter);
    GFileChooserSetFilterFunc(gcd[0].ret,filter);
    GFileChooserSetMimetypes(gcd[0].ret,mimetypes);
    GFileChooserSetPaths(gcd[0].ret,path);
    GGadgetSetTitle(gcd[0].ret,defaultfile);

    memset(&d,'\0',sizeof(d));
    d.gfc = gcd[0].ret;

    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GDrawProcessPendingEvents(NULL);		/* Give the window a chance to vanish... */
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);		/* Give the window a chance to vanish... */
    GProgressResumeTimer();
return(d.ret);
}

char *GWidgetOpenFileWPath8(const char *title, const char *defaultfile,
	const char *initial_filter, char **mimetypes,
	GFileChooserFilterType filter, const char* const* path) {
    unichar_t *tit=NULL, *def=NULL, *filt=NULL, **mimes=NULL, *ret;
    char *utf8_ret;
    int i;

    if ( title!=NULL )
	tit = utf82u_copy(title);
    if ( defaultfile!=NULL )
	def = utf82u_copy(defaultfile);
    else if ( path!=NULL && path[0]!=NULL )
	def = utf82u_copy(path[0]);
    if ( initial_filter!=NULL )
	filt = utf82u_copy(initial_filter);
    if ( mimetypes!=NULL ) {
	for ( i=0; mimetypes[i]!=NULL; ++i );
	mimes = malloc((i+1)*sizeof(unichar_t *));
	for ( i=0; mimetypes[i]!=NULL; ++i )
	    mimes[i] = utf82u_copy(mimetypes[i]);
	mimes[i] = NULL;
    }
    ret = GWidgetOpenFileWPath(tit,def,filt,mimes,filter,path);
    if ( mimes!=NULL ) {
	for ( i=0; mimes[i]!=NULL; ++i )
	    free(mimes[i]);
	free(mimes);
    }
    free(filt); free(def); free(tit);
    utf8_ret = u2utf8_copy(ret);
    free(ret);
return( utf8_ret );
}

char *GWidgetOpenFile8(const char *title, const char *defaultfile,
	const char *initial_filter, char **mimetypes,
	GFileChooserFilterType filter) {
return( GWidgetOpenFileWPath8(title,defaultfile,initial_filter,mimetypes,filter,NULL));
}
