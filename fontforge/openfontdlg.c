/* Copyright (C) 2000-2006 by George Williams */
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <stdlib.h>
#include <string.h>
#include <ustring.h>
#include <gdraw.h>
#include <gwidget.h>
#include <ggadget.h>

extern NameList *force_names_when_opening;

struct gfc_data {
    int done;
    unichar_t *ret;
    GGadget *gfc;
    GGadget *rename;
    int filename_popup_pos;
    unichar_t *lastpopupfontname;
};

static int GFD_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *tf;
	GFileChooserGetChildren(d->gfc,NULL,NULL,&tf);
	if ( *_GGadgetGetTitle(tf)!='\0' ) {
	    extern int allow_utf8_glyphnames;
	    GTextInfo *ti = GGadgetGetListItemSelected(d->rename);
	    char *nlname = u2utf8_copy(ti->text);
	    force_names_when_opening = NameListByName(nlname);
	    free(nlname);
	    if ( force_names_when_opening!=NULL && force_names_when_opening->uses_unicode &&
		    !allow_utf8_glyphnames) {
		gwwv_post_error(_("Namelist contains non-ASCII names"),_("Glyph names should be limited to characters in the ASCII character set, but there are names in this namelist which use characters outside that range."));
return(true);
	    }
	    d->done = true;
	    d->ret = GGadgetGetTitle(d->gfc);
	}
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

static int WithinList(struct gfc_data *d,GEvent *event) {
    GRect size;
    GGadget *list;
    int32 pos;
    unichar_t *ufile;
    char *file, **fontnames;
    int cnt, len;
    unichar_t *msg;

    if ( event->type!=et_mousemove )
return( false );

    GFileChooserGetChildren(d->gfc,NULL, &list, NULL);
    if ( list==NULL )
return( false );
    GGadgetGetSize(list,&size);
    if ( event->u.mouse.x < size.x || event->u.mouse.y <size.y ||
	    event->u.mouse.x >= size.x+size.width ||
	    event->u.mouse.y >= size.y+size.width )
return( false );
    pos = GListIndexFromY(list,event->u.mouse.y);
    if ( pos == d->filename_popup_pos )
return( pos!=-1 );
    if ( pos==-1 || GFileChooserPosIsDir(d->gfc,pos)) {
	d->filename_popup_pos = -1;
return( pos!=-1 );
    }
    ufile = GFileChooserFileNameOfPos(d->gfc,pos);
    if ( ufile==NULL )
return( true );
    file = u2def_copy(ufile);

    fontnames = GetFontNames(file);
    if ( fontnames==NULL || fontnames[0]==NULL )
	msg = uc_copy( "???" );
    else {
	len = 0;
	for ( cnt=0; fontnames[cnt]!=NULL; ++cnt )
	    len += strlen(fontnames[cnt])+1;
	msg = galloc((len+2)*sizeof(unichar_t));
	len = 0;
	for ( cnt=0; fontnames[cnt]!=NULL; ++cnt ) {
	    uc_strcpy(msg+len,fontnames[cnt]);
	    len += strlen(fontnames[cnt]);
	    msg[len++] = '\n';
	}
	msg[len-1] = '\0';
    }
    GGadgetPreparePopup(GGadgetGetWindow(d->gfc),msg);
    free(d->lastpopupfontname);
    d->lastpopupfontname = msg;
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
	if ( !WithinList(d,event) )
	    GFileChooserPopupCheck(d->gfc,event);
    } else if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
	struct gfc_data *d = GDrawGetUserData(gw);
return( GGadgetDispatchEvent((GGadget *) (d->gfc),event));
    } else if ( event->type == et_resize ) {
	GRect r, size;
	struct gfc_data *d = GDrawGetUserData(gw);
	GDrawGetSize(gw,&size);
	GGadgetGetSize(d->gfc,&r);
	GGadgetResize(d->gfc,size.width-2*r.x,r.height);
    }
return( event->type!=et_char );
}

# ifdef FONTFORGE_CONFIG_GTK
char *FVOpenFont(char *title, const char *defaultfile,
	const char *initial_filter, int mult) {
#else
unichar_t *FVOpenFont(char *title, const char *defaultfile,
	const char *initial_filter, unichar_t **mimetypes,int mult) {
#endif
    GRect pos;
    int i, filter;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[9];
    GTextInfo label[8];
    struct gfc_data d;
    int bs = GIntGetResource(_NUM_Buttonsize), bsbigger, totwid, spacing;
    GGadget *tf;
    unichar_t *temp;
    char **nlnames;
    GTextInfo *namelistnames;
    int cnt;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = title;
    pos.x = pos.y = 0;

    totwid = GGadgetScale(295);
    bsbigger = 4*bs+4*14>totwid; totwid = bsbigger?4*bs+4*12:totwid;
    spacing = (totwid-4*bs-2*12)/3;

    pos.width = GDrawPointsToPixels(NULL,totwid);
    pos.height = GDrawPointsToPixels(NULL,247);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    i=0;
    gcd[i].gd.pos.x = 12; gcd[i].gd.pos.y = 6; gcd[i].gd.pos.width = totwid*100/GIntGetResource(_NUM_ScaleFactor)-24; gcd[i].gd.pos.height = 180;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    if ( RecentFiles[0]!=NULL )
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_file_pulldown;
    if ( mult )
	gcd[i].gd.flags |= gg_file_multiple;
    gcd[i++].creator = GFileChooserCreate;

    label[i].text = (unichar_t *) _("Force glyph names to:");
    label[i].text_is_1byte = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.pos.x = 8; gcd[i].gd.pos.y = 188+6;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[i].gd.popup_msg = (unichar_t *) _("In the saved font, force all glyph names to match those in the specified namelist");
    gcd[i++].creator = GLabelCreate;

    gcd[i].gd.pos.x = 0; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y-6;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[i].gd.popup_msg = (unichar_t *) _("In the saved font, force all glyph names to match those in the specified namelist");
    gcd[i].creator = GListButtonCreate;
    nlnames = AllNamelistNames();
    for ( cnt=0; nlnames[cnt]!=NULL; ++cnt);
    namelistnames = gcalloc(cnt+3,sizeof(GTextInfo));
    namelistnames[0].text = (unichar_t *) _("No Rename");
    namelistnames[0].text_is_1byte = true;
    if ( force_names_when_opening==NULL ) {
	namelistnames[0].selected = true;
	gcd[i].gd.label = &namelistnames[0];
    }
    namelistnames[1].line = true;
    for ( cnt=0; nlnames[cnt]!=NULL; ++cnt) {
	namelistnames[cnt+2].text = (unichar_t *) nlnames[cnt];
	namelistnames[cnt+2].text_is_1byte = true;
	if ( force_names_when_opening!=NULL &&
		strcmp(_(force_names_when_opening->title),nlnames[cnt])==0 ) {
	    namelistnames[cnt+2].selected = true;
	    gcd[i].gd.label = &namelistnames[cnt+2];
	}
    }
    gcd[i++].gd.u.list = namelistnames;
    free(nlnames);

    gcd[i].gd.pos.x = 12; gcd[i].gd.pos.y = 216-3;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _("_OK");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'O';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = GFD_Ok;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -(spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)-12; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) _("_New");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'N';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = GFD_New;
    gcd[i++].creator = GButtonCreate;

    filter = i;
    gcd[i].gd.pos.x = (spacing+bs)*100/GIntGetResource(_NUM_ScaleFactor)+12; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled;
    label[i].text = (unichar_t *) _("_Filter");
    label[i].text_is_1byte = true;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'F';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = -12; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
    gcd[i].gd.pos.width = -1;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _("_Cancel");
    label[i].text_is_1byte = true;
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
    free(namelistnames);
    GGadgetSetUserData(gcd[filter].ret,gcd[0].ret);

    GFileChooserConnectButtons(gcd[0].ret,gcd[3].ret,gcd[filter].ret);
    temp = utf82u_copy(initial_filter);
    GFileChooserSetFilterText(gcd[0].ret,temp);
    free(temp);
    GFileChooserSetMimetypes(gcd[0].ret,mimetypes);
    GFileChooserGetChildren(gcd[0].ret,NULL, NULL, &tf);
    if ( RecentFiles[0]!=NULL ) {
	GGadgetSetList(tf,GTextInfoFromChars(RecentFiles,RECENT_MAX),false);
    }
    GGadgetSetTitle8(gcd[0].ret,defaultfile);

    memset(&d,'\0',sizeof(d));
    d.gfc = gcd[0].ret;
    d.rename = gcd[2].ret;

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GDrawProcessPendingEvents(NULL);		/* Give the window a chance to vanish... */
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);		/* Give the window a chance to vanish... */
    free( d.lastpopupfontname );
return(d.ret);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
