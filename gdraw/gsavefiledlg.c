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
#include "ustring.h"
#include "gfile.h"
#include "gdraw.h"
#include "gwidget.h"
#include "ggadget.h"
#include "gio.h"
#include "gicons.h"

struct gfc_data {
    int done;
    unichar_t *ret;
    GGadget *gfc;
};

static unichar_t save[] = { 'S', 'a', 'v', 'e', '\0' };
static unichar_t filter[] = { 'F', 'i', 'l', 't', 'e', 'r', '\0' };
static unichar_t cancel[] = { 'C', 'a', 'n', 'c', 'e', 'l', '\0' };
static unichar_t new[] = { 'N', 'e', 'w', '.', '.', '.', '\0' };
static unichar_t replace[] = { 'R', 'e', 'p', 'l', 'a', 'c', 'e', '\0' };

static unichar_t rcmn[] = { 'R', 'C', '\0' };
static unichar_t *buts[] = { replace, cancel, NULL };

static void GFD_doesnt(GIOControl *gio) {
    /* The filename the user chose doesn't exist, so everything is happy */
    struct gfc_data *d = gio->userdata;
    d->done = true;
    GFileChooserReplaceIO(d->gfc,NULL);
}

static void GFD_exists(GIOControl *gio) {
    /* The filename the user chose exists, ask user if s/he wants to overwrite */
    struct gfc_data *d = gio->userdata;
    unichar_t buffer[200];
    unichar_t title[30];

    uc_strcpy(title, "File Exists");
    uc_strcpy(buffer, "File, ");
    u_strcat(buffer, u_GFileNameTail(d->ret));
    uc_strcat(buffer, ", exists. Replace it?");
    if ( GWidgetAsk(title,buffer,buts,rcmn,0,1)==0 ) {
	d->done = true;
    }
    GFileChooserReplaceIO(d->gfc,NULL);
}

static int GFD_SaveOk(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->ret = GGadgetGetTitle(d->gfc);
	GIOfileExists(GFileChooserReplaceIO(d->gfc,
		GIOCreate(d->ret,d,GFD_exists,GFD_doesnt)));
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

static void GFD_dircreated(GIOControl *gio) {
    struct gfc_data *d = gio->userdata;
    unichar_t *dir = u_copy(gio->path);

    GFileChooserReplaceIO(d->gfc,NULL);
    GFileChooserSetDir(d->gfc,dir);
    free(dir);
}

static void GFD_dircreatefailed(GIOControl *gio) {
    /* We couldn't create the directory */
    struct gfc_data *d = gio->userdata;
    unichar_t buffer[500];
    unichar_t title[30];

    uc_strcpy(title, "Couldn't create directory");
    uc_strcpy(buffer, "Couldn't create directory, ");
    u_strcat(buffer, u_GFileNameTail(gio->path));
    uc_strcat(buffer, ".\n");
    if ( gio->error!=NULL ) {
	u_strcat(buffer,gio->error);
	uc_strcat(buffer, "\n");
    }
    if ( gio->status[0]!='\0' )
	u_strcat(buffer,gio->status);
    GWidgetPostNotice(title,buffer);
    GFileChooserReplaceIO(d->gfc,NULL);
}

static int GFD_NewDir(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfc_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t *newdir;
	unichar_t title[30], buffer[30];
	uc_strcpy(title,"Create directory...");
	uc_strcpy(buffer,"Directory name?");
	newdir = GWidgetAskString(title,buffer,NULL);
	if ( newdir==NULL )
return( true );
	if ( !u_GFileIsAbsolute(newdir)) {
	    unichar_t *temp = u_GFileAppendFile(GFileChooserGetDir(d->gfc),newdir,false);
	    free(newdir);
	    newdir = temp;
	}
	GIOmkDir(GFileChooserReplaceIO(d->gfc,
		GIOCreate(newdir,d,GFD_dircreated,GFD_dircreatefailed)));
	free(newdir);
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
return( true );
}

unichar_t *GWidgetSaveAsFile(const unichar_t *title, const unichar_t *defaultfile,
	const unichar_t *initial_filter, unichar_t **mimetypes) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[7];
    GTextInfo label[5];
    struct gfc_data d;
    GGadget *pulldown, *files, *tf;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = (unichar_t *) title;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,223);
    pos.height = GDrawPointsToPixels(NULL,255);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; gcd[0].gd.pos.width = 200; gcd[0].gd.pos.height = 180;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GFileChooserCreate;

    gcd[1].gd.pos.x = 12; gcd[1].gd.pos.y = 222-3; gcd[1].gd.pos.width = 55; gcd[1].gd.pos.height = 0;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[1].text = save;
    gcd[1].gd.mnemonic = 'S';
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = GFD_SaveOk;
    gcd[1].creator = GButtonCreate;

    gcd[2].gd.pos.x = 84; gcd[2].gd.pos.y = 222; gcd[2].gd.pos.width = 55; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled;
    label[2].text = filter;
    gcd[2].gd.mnemonic = 'F';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = GFileChooserFilterEh;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = 155; gcd[3].gd.pos.y = 222; gcd[3].gd.pos.width = 55; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = cancel;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.mnemonic = 'C';
    gcd[3].gd.handle_controlevent = GFD_Cancel;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = 84; gcd[4].gd.pos.y = 192; gcd[4].gd.pos.width = 55; gcd[4].gd.pos.height = 0;
    gcd[4].gd.flags = gg_visible | gg_enabled;
    label[4].text = new;
    label[4].image = &_GIcon_dir;
    label[4].image_precedes = false;
    gcd[4].gd.mnemonic = 'N';
    gcd[4].gd.label = &label[4];
    gcd[4].gd.handle_controlevent = GFD_NewDir;
    gcd[4].creator = GButtonCreate;

    gcd[5].gd.pos.x = 2; gcd[5].gd.pos.y = 2;
    gcd[5].gd.pos.width = pos.width-4; gcd[5].gd.pos.height = pos.height-4;
    gcd[5].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[5].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GGadgetSetUserData(gcd[2].ret,gcd[0].ret);

    GFileChooserConnectButtons(gcd[0].ret,gcd[1].ret,gcd[2].ret);
    GFileChooserSetFilterText(gcd[0].ret,initial_filter);
    GFileChooserSetMimetypes(gcd[0].ret,mimetypes);
    GGadgetSetTitle(gcd[0].ret,defaultfile);
    GFileChooserGetChildren(gcd[0].ret,&pulldown,&files,&tf);
    GWidgetIndicateFocusGadget(tf);

    memset(&d,'\0',sizeof(d));
    d.gfc = gcd[0].ret;

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
return(d.ret);
}
