/* Copyright (C) 2001-2002 by George Williams */
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
#include "ttfmodui.h"
#include <gkeysym.h>
#include <ustring.h>
#include <utype.h>

#define CID_Version	1000
#define CID_MinorVersion	1001
#define CID_DefYOrig	1002
#define CID_AddOrig	1003

typedef struct vorgview /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
    unsigned int destroyed: 1;		/* window has been destroyed */
/* vorg specials */
} VORGView;

static int vorg_processdata(TableView *tv) {
    int err = false;
    int version, minor, defyorig, addorig;

    version = GetIntR(tv->gw,CID_Version,_STR_Version,&err);
    minor = GetIntR(tv->gw,CID_MinorVersion,_STR_Minor,&err);
    defyorig = GetIntR(tv->gw,CID_DefYOrig,_STR_DefYOrig,&err);
    addorig = GetIntR(tv->gw,CID_AddOrig,_STR_AddOrig,&err);
    if ( err )
return( false );
    ptputushort(tv->table->data,version);
    ptputushort(tv->table->data+2,minor);
    ptputushort(tv->table->data+4,defyorig);
    ptputushort(tv->table->data+6,addorig);
    /* there are also addorig entries of <glyph id, y origin> but I'm not showing those yet */
    if ( !tv->table->changed ) {
	tv->table->changed = true;
	tv->table->container->changed = true;
	GDrawRequestExpose(tv->owner->v,NULL,false);
    }
return( true );
}

static int vorg_close(TableView *tv) {
    if ( vorg_processdata(tv)) {
	tv->destroyed = true;
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs vorgfuncs = { vorg_close, vorg_processdata };

static int VORG_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	((TableView *) GDrawGetUserData(gw))->destroyed = true;
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int VORG_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    VORGView *vv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	vv = GDrawGetUserData(gw);
	vorg_close((TableView *) vv);
    }
return( true );
}

static int vorg_e_h(GWindow gw, GEvent *event) {
    VORGView *vv = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	vv->destroyed = true;
	GDrawDestroyWindow(vv->gw);
    } else if ( event->type == et_destroy ) {
	vv->table->tv = NULL;
	free(vv);
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 ) {
	    TableHelp(vv->table->name);
	    system("netscape http://partners.adobe.com/asn/developer/opentype/vorg.html &");
return( true );
	} else if (( event->u.chr.state&ksm_control ) &&
		(event->u.chr.keysym=='q' || event->u.chr.keysym=='Q')) {
	    MenuExit(NULL,NULL,NULL);
	}
return( false );
    }
return( true );
}

void VORGCreateEditor(Table *tab,TtfView *tfv) {
    VORGView *vv = gcalloc(1,sizeof(VORGView));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[34];
    GTextInfo label[34];
    int i;
    unichar_t title[60];
    char version[20], minor[20], defyorig[20], nummetrics[20];

    vv->table = tab;
    vv->virtuals = &vorgfuncs;
    vv->owner = tfv;
    vv->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) vv;

    TableFillup(tab);

    title[0] = (tab->name>>24)&0xff;
    title[1] = (tab->name>>16)&0xff;
    title[2] = (tab->name>>8 )&0xff;
    title[3] = (tab->name    )&0xff;
    title[4] = ' ';
    u_strncpy(title+5, vv->font->fontname, sizeof(title)/sizeof(title[0])-6);
    title[sizeof(title)/sizeof(title[0])-1] = '\0';
    
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,220);
    pos.height = GDrawPointsToPixels(NULL,125);
    vv->gw = gw = GDrawCreateTopWindow(NULL,&pos,vorg_e_h,vv,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_Version;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 7+6; 
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    sprintf( version, "%d", tgetushort(tab,0) );
    label[1].text = (unichar_t *) version;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 50; gcd[1].gd.pos.y = gcd[0].gd.pos.y-6;  gcd[1].gd.pos.width = 50;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_Version;
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) _STR_Minor;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 5+gcd[1].gd.pos.x+gcd[1].gd.pos.width; gcd[2].gd.pos.y = gcd[0].gd.pos.y; 
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    sprintf( minor, "%d", tgetushort(tab,2) );
    label[3].text = (unichar_t *) minor;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = gcd[2].gd.pos.x+50; gcd[3].gd.pos.y = gcd[2].gd.pos.y-6;  gcd[3].gd.pos.width = 50;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_MinorVersion;
    gcd[3].creator = GTextFieldCreate;

    label[4].text = (unichar_t *) _STR_DefYOrig;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = gcd[3].gd.pos.y+24+6; 
    gcd[4].gd.flags = gg_enabled|gg_visible;
    gcd[4].creator = GLabelCreate;

    sprintf( defyorig, "%d", tgetushort(tab,4) );
    label[5].text = (unichar_t *) defyorig;
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = gcd[1].gd.pos.x+50; gcd[5].gd.pos.y = gcd[4].gd.pos.y-6;  gcd[5].gd.pos.width = 50;
    gcd[5].gd.flags = gg_enabled|gg_visible;
    gcd[5].gd.cid = CID_DefYOrig;
    gcd[5].creator = GTextFieldCreate;

    label[6].text = (unichar_t *) _STR_AddOrig;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = gcd[4].gd.pos.y+24; 
    gcd[6].gd.flags = gg_enabled|gg_visible;
    gcd[6].creator = GLabelCreate;

    sprintf( nummetrics, "%d", tgetushort(tab,6) );
    label[7].text = (unichar_t *) nummetrics;
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = gcd[5].gd.pos.x; gcd[7].gd.pos.y = gcd[6].gd.pos.y-6;  gcd[7].gd.pos.width = 50;
    gcd[7].gd.flags = gg_enabled|gg_visible;
    gcd[7].gd.cid = CID_AddOrig;
    gcd[7].creator = GTextFieldCreate;

    /* there are also addorig entries of <glyph id, y origin> but I'm not showing those yet */

	i = 8;

    gcd[i].gd.pos.x = 20-3; gcd[i].gd.pos.y = gcd[7].gd.pos.y+35-3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _STR_OK;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'O';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = VORG_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 215-GIntGetResource(_NUM_Buttonsize)-20; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _STR_Cancel;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.handle_controlevent = VORG_Cancel;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
    gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
    gcd[i].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[i].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GDrawSetVisible(gw,true);
}
