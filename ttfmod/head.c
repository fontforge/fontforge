/* Copyright (C) 2001 by George Williams */
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
#include <ustring.h>
#include <utype.h>

#define CID_Version	1001
#define CID_Revision	1002
#define CID_Checksum	1003
#define CID_MagicNumber	1004
#define CID_Flags	1005
#define CID_UnitsPerEm	1006
#define CID_Date	1007
#define CID_xMin	1008
#define CID_yMin	1009
#define CID_xMax	1010
#define CID_yMax	1011
#define CID_MacStyle	1012
#define CID_SmallestSize	1013
#define CID_FontDir	1014
#define CID_LocaFormat	1015
#define CID_GlyphFormat	1015

static unichar_t mnum[] = { '5','f','0','f','3','c','f','5',  '\0' };
static GTextInfo magicnumbers[] = {
    { (unichar_t *) mnum},
    { NULL }};
static GTextInfo flags[] = {
    { (unichar_t *) _STR_Baseliney0, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LBearingx0, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_InstrsDepOnPointsize, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ForcePPEMInt, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_InstrsAlterWidth, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

typedef struct headview /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
/* head specials */
} HeadView;

static int head_processdata(TableView *tv) {
    HeadView *hv = (HeadView *) tv;
    /* Do changes!!! */
return( true );
}

static int head_close(TableView *tv) {
    if ( head_processdata(tv)) {
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs headfuncs = { head_close, head_processdata };

static int head_VersionChange(GGadget *g, GEvent *e) {
    GWindow gw = GGadgetGetWindow(g);
    const unichar_t *ret = _GGadgetGetTitle(g);
    double val = u_strtod(ret,NULL);
    int i;

    if ( val==.5 || val==1.0 ) {
	for ( i=CID_Points; i<=CID_CDepth; ++i ) {
	    GGadgetSetEnabled(GWidgetGetControl(gw,i),val==1.0);
	    GGadgetSetEnabled(GWidgetGetControl(gw,i+1000),val==1.0);
	}
    }
return( true );
}

static int Head_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    HeadView *hv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	hv = GDrawGetUserData(gw);
	head_close((TableView *) hv);
    }
return( true );
}

static int Head_OK(GGadget *g, GEvent *e) {
    GWindow gw;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int head_e_h(GWindow gw, GEvent *event) {
    HeadView *hv = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	GDrawDestroyWindow(hv->gw);
    } else if ( event->type == et_destroy ) {
	hv->table->tv = NULL;
	free(hv);
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 ) {
	    TableHelp(hv->table->name);
	    system("netscape http://partners.adobe.com/asn/developer/opentype/head.html &");
return( true );
	} else if (( event->u.chr.state&ksm_control ) &&
		(event->u.chr.keysym=='q' || event->u.chr.keysym=='Q')) {
	    MenuExit(NULL,NULL,NULL);
	}
return( false );
    }
return( true );
}

void headCreateEditor(Table *tab,TtfView *tfv) {
    HeadView *hv = gcalloc(1,sizeof(HeadView));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[34];
    GTextInfo label[34];
    int short_table, i;
    static unichar_t title[60];
    char version[10], revision[10],

    hv->table = tab;
    hv->virtuals = &headfuncs;
    hv->owner = tfv;
    hv->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) hv;

    TableFillup(tab);

    title[0] = (tab->name>>24)&0xff;
    title[1] = (tab->name>>16)&0xff;
    title[2] = (tab->name>>8 )&0xff;
    title[3] = (tab->name    )&0xff;
    title[4] = ' ';
    u_strncpy(title+5, mv->font->fontname, sizeof(title)/sizeof(title[0])-6);
    
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    u_strncpy(title+5, hv->font->fontname, sizeof(title)/sizeof(title[0])-6);
    title[sizeof(title)/sizeof(title[0])-1] = '\0';
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,270);
    pos.height = GDrawPointsToPixels(NULL,267);
    hv->gw = gw = GDrawCreateTopWindow(NULL,&pos,head_e_h,hv,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_Version;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+6; 
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    sprintf( version, "%.4g", tgetfixed(tab,0) );
    label[1].text = (unichar_t *) version;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 80; gcd[1].gd.pos.y = gcd[0].gd.pos.y-6;  gcd[1].gd.pos.width = 50;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_Version;
    gcd[1].gd.handle_controlevent = head_VersionChange;
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) _STR_FontRevision;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = gcd[1].gd.pos.y+24+6; 
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    sprintf( revision, "%.4g", tgetfixed(tab,4) );
    label[3].text = (unichar_t *) revision;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = gcd[1].gd.pos.x; gcd[3].gd.pos.y = gcd[2].gd.pos.y-6;  gcd[3].gd.pos.width = 50;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_Revision;
    gcd[3].creator = GTextFieldCreate;

    label[4].text = (unichar_t *) _STR_Checksum;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = gcd[3].gd.pos.y+24+6; 
    gcd[4].gd.flags = gg_enabled|gg_visible;
    gcd[4].creator = GLabelCreate;

    sprintf( points, "%08x", tgetlong(tab,8) );
    if ( short_table ) points[0] = '\0';
    label[5].text = (unichar_t *) points;
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = gcd[1].gd.pos.x; gcd[5].gd.pos.y = gcd[4].gd.pos.y-6;  gcd[5].gd.pos.width = 50;
    gcd[5].gd.flags = gg_enabled|gg_visible;
    gcd[5].gd.cid = CID_Checksum;
    gcd[5].creator = GTextFieldCreate;

    label[6].text = (unichar_t *) _STR_MagicNumber;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 5+gcd[1].gd.pos.x+gcd[1].gd.pos.width; gcd[6].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[6].gd.flags = gg_enabled|gg_visible;
    gcd[6].creator = GLabelCreate;

    sprintf( magic, "%x", tgetushort(tab,8) );
    label[7].text = (unichar_t *) magic;
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = gcd[6].gd.pos.x+75; gcd[7].gd.pos.y = gcd[6].gd.pos.y-6;  gcd[7].gd.pos.width = 50;
    gcd[7].gd.flags = gg_enabled|gg_visible;
    gcd[7].gd.cid = CID_MagicNumber;
    gcd[7].gd.u.list = magicnumbers;
    gcd[7].creator = GListFieldCreate;

    label[8].text = (unichar_t *) _STR_CompositPoints;
    label[8].text_in_resource = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.pos.x = 5; gcd[8].gd.pos.y = gcd[5].gd.pos.y+24+6; 
    gcd[8].gd.flags = gg_enabled|gg_visible;
    gcd[8].gd.cid = CID_CPoints+1000;
    gcd[8].creator = GLabelCreate;

    sprintf( cpoints, "%d", tgetushort(tab,10) );
    label[9].text = (unichar_t *) cpoints;
    label[9].text_is_1byte = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.pos.x = gcd[1].gd.pos.x; gcd[9].gd.pos.y = gcd[8].gd.pos.y-6;  gcd[9].gd.pos.width = 50;
    gcd[9].gd.flags = gg_enabled|gg_visible;
    gcd[9].gd.cid = CID_CPoints;
    gcd[9].creator = GTextFieldCreate;

    label[10].text = (unichar_t *) _STR_CompositContours;
    label[10].text_in_resource = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.pos.x = gcd[6].gd.pos.x; gcd[10].gd.pos.y = gcd[8].gd.pos.y; 
    gcd[10].gd.flags = gg_enabled|gg_visible;
    gcd[10].gd.cid = CID_CContours+1000;
    gcd[10].creator = GLabelCreate;

    sprintf( ccontours, "%d", tgetushort(tab,12) );
    label[11].text = (unichar_t *) ccontours;
    label[11].text_is_1byte = true;
    gcd[11].gd.label = &label[11];
    gcd[11].gd.pos.x = gcd[7].gd.pos.x; gcd[11].gd.pos.y = gcd[10].gd.pos.y-6;  gcd[11].gd.pos.width = 50;
    gcd[11].gd.flags = gg_enabled|gg_visible;
    gcd[11].gd.cid = CID_CContours;
    gcd[11].creator = GTextFieldCreate;

    label[12].text = (unichar_t *) _STR_Zones;
    label[12].text_in_resource = true;
    gcd[12].gd.label = &label[12];
    gcd[12].gd.pos.x = 5; gcd[12].gd.pos.y = gcd[9].gd.pos.y+24+6; 
    gcd[12].gd.flags = gg_enabled|gg_visible;
    gcd[12].gd.cid = CID_Zones+1000;
    gcd[12].creator = GLabelCreate;

    sprintf( zones, "%d", tgetushort(tab,14) );
    label[13].text = (unichar_t *) zones;
    label[13].text_is_1byte = true;
    gcd[13].gd.label = &label[13];
    gcd[13].gd.pos.x = gcd[1].gd.pos.x; gcd[13].gd.pos.y = gcd[12].gd.pos.y-6;  gcd[13].gd.pos.width = 50;
    gcd[13].gd.flags = gg_enabled|gg_visible;
    gcd[13].gd.cid = CID_Zones;
    gcd[13].creator = GTextFieldCreate;

    label[14].text = (unichar_t *) _STR_TwilightPoints;
    label[14].text_in_resource = true;
    gcd[14].gd.label = &label[14];
    gcd[14].gd.pos.x = gcd[6].gd.pos.x; gcd[14].gd.pos.y = gcd[12].gd.pos.y; 
    gcd[14].gd.flags = gg_enabled|gg_visible;
    gcd[14].gd.cid = CID_TPoints+1000;
    gcd[14].creator = GLabelCreate;

    sprintf( tpoints, "%d", tgetushort(tab,16) );
    label[15].text = (unichar_t *) tpoints;
    label[15].text_is_1byte = true;
    gcd[15].gd.label = &label[15];
    gcd[15].gd.pos.x = gcd[7].gd.pos.x; gcd[15].gd.pos.y = gcd[14].gd.pos.y-6;  gcd[15].gd.pos.width = 50;
    gcd[15].gd.flags = gg_enabled|gg_visible;
    gcd[15].gd.cid = CID_TPoints;
    gcd[15].creator = GTextFieldCreate;

    label[16].text = (unichar_t *) _STR_Storage;
    label[16].text_in_resource = true;
    gcd[16].gd.label = &label[16];
    gcd[16].gd.pos.x = 5; gcd[16].gd.pos.y = gcd[13].gd.pos.y+24+6; 
    gcd[16].gd.flags = gg_enabled|gg_visible;
    gcd[16].gd.cid = CID_Storage+1000;
    gcd[16].creator = GLabelCreate;

    sprintf( storage, "%d", tgetushort(tab,18) );
    label[17].text = (unichar_t *) storage;
    label[17].text_is_1byte = true;
    gcd[17].gd.label = &label[17];
    gcd[17].gd.pos.x = gcd[1].gd.pos.x; gcd[17].gd.pos.y = gcd[16].gd.pos.y-6;  gcd[17].gd.pos.width = 50;
    gcd[17].gd.flags = gg_enabled|gg_visible;
    gcd[17].gd.cid = CID_Storage;
    gcd[17].creator = GTextFieldCreate;

    label[18].text = (unichar_t *) _STR_FDEFs;
    label[18].text_in_resource = true;
    gcd[18].gd.label = &label[18];
    gcd[18].gd.pos.x = 5; gcd[18].gd.pos.y = gcd[17].gd.pos.y+24+6; 
    gcd[18].gd.flags = gg_enabled|gg_visible;
    gcd[18].gd.cid = CID_FDefs+1000;
    gcd[18].creator = GLabelCreate;

    sprintf( fdefs, "%d", tgetushort(tab,20) );
    label[19].text = (unichar_t *) fdefs;
    label[19].text_is_1byte = true;
    gcd[19].gd.label = &label[19];
    gcd[19].gd.pos.x = gcd[1].gd.pos.x; gcd[19].gd.pos.y = gcd[18].gd.pos.y-6;  gcd[19].gd.pos.width = 50;
    gcd[19].gd.flags = gg_enabled|gg_visible;
    gcd[19].gd.cid = CID_FDefs;
    gcd[19].creator = GTextFieldCreate;

    label[20].text = (unichar_t *) _STR_IDEFs;
    label[20].text_in_resource = true;
    gcd[20].gd.label = &label[20];
    gcd[20].gd.pos.x = gcd[6].gd.pos.x; gcd[20].gd.pos.y = gcd[18].gd.pos.y; 
    gcd[20].gd.flags = gg_enabled|gg_visible;
    gcd[20].gd.cid = CID_IDefs+1000;
    gcd[20].creator = GLabelCreate;

    sprintf( idefs, "%d", tgetushort(tab,22) );
    label[21].text = (unichar_t *) idefs;
    label[21].text_is_1byte = true;
    gcd[21].gd.label = &label[21];
    gcd[21].gd.pos.x = gcd[7].gd.pos.x; gcd[21].gd.pos.y = gcd[20].gd.pos.y-6;  gcd[21].gd.pos.width = 50;
    gcd[21].gd.flags = gg_enabled|gg_visible;
    gcd[21].gd.cid = CID_IDefs;
    gcd[21].creator = GTextFieldCreate;

    label[22].text = (unichar_t *) _STR_StackElements;
    label[22].text_in_resource = true;
    gcd[22].gd.label = &label[22];
    gcd[22].gd.pos.x = 5; gcd[22].gd.pos.y = gcd[19].gd.pos.y+24+6; 
    gcd[22].gd.flags = gg_enabled|gg_visible;
    gcd[22].gd.cid = CID_SEl+1000;
    gcd[22].creator = GLabelCreate;

    sprintf( sels, "%d", tgetushort(tab,24) );
    label[23].text = (unichar_t *) sels;
    label[23].text_is_1byte = true;
    gcd[23].gd.label = &label[23];
    gcd[23].gd.pos.x = gcd[1].gd.pos.x; gcd[23].gd.pos.y = gcd[22].gd.pos.y-6;  gcd[23].gd.pos.width = 50;
    gcd[23].gd.flags = gg_enabled|gg_visible;
    gcd[23].gd.cid = CID_SEl;
    gcd[23].creator = GTextFieldCreate;

    label[24].text = (unichar_t *) _STR_SizeOfInstructions;
    label[24].text_in_resource = true;
    gcd[24].gd.label = &label[24];
    gcd[24].gd.pos.x = gcd[6].gd.pos.x; gcd[24].gd.pos.y = gcd[22].gd.pos.y; 
    gcd[24].gd.flags = gg_enabled|gg_visible;
    gcd[24].gd.cid = CID_SOI+1000;
    gcd[24].creator = GLabelCreate;

    sprintf( soi, "%d", tgetushort(tab,26) );
    label[25].text = (unichar_t *) soi;
    label[25].text_is_1byte = true;
    gcd[25].gd.label = &label[25];
    gcd[25].gd.pos.x = gcd[7].gd.pos.x; gcd[25].gd.pos.y = gcd[24].gd.pos.y-6;  gcd[25].gd.pos.width = 50;
    gcd[25].gd.flags = gg_enabled|gg_visible;
    gcd[25].gd.cid = CID_SOI;
    gcd[25].creator = GTextFieldCreate;

    label[26].text = (unichar_t *) _STR_ComponentElements;
    label[26].text_in_resource = true;
    gcd[26].gd.label = &label[26];
    gcd[26].gd.pos.x = 5; gcd[26].gd.pos.y = gcd[23].gd.pos.y+24+6; 
    gcd[26].gd.flags = gg_enabled|gg_visible;
    gcd[26].gd.cid = CID_CEl+1000;
    gcd[26].creator = GLabelCreate;

    sprintf( cels, "%d", tgetushort(tab,28) );
    label[27].text = (unichar_t *) cels;
    label[27].text_is_1byte = true;
    gcd[27].gd.label = &label[27];
    gcd[27].gd.pos.x = gcd[1].gd.pos.x; gcd[27].gd.pos.y = gcd[26].gd.pos.y-6;  gcd[27].gd.pos.width = 50;
    gcd[27].gd.flags = gg_enabled|gg_visible;
    gcd[27].gd.cid = CID_CEl;
    gcd[27].creator = GTextFieldCreate;

    label[28].text = (unichar_t *) _STR_ComponentDepth;
    label[28].text_in_resource = true;
    gcd[28].gd.label = &label[28];
    gcd[28].gd.pos.x = gcd[6].gd.pos.x; gcd[28].gd.pos.y = gcd[26].gd.pos.y; 
    gcd[28].gd.flags = gg_enabled|gg_visible;
    gcd[28].gd.cid = CID_CDepth+1000;
    gcd[28].creator = GLabelCreate;

    sprintf( cdep, "%d", tgetushort(tab,30) );
    label[29].text = (unichar_t *) cdep;
    label[29].text_is_1byte = true;
    gcd[29].gd.label = &label[29];
    gcd[29].gd.pos.x = gcd[7].gd.pos.x; gcd[29].gd.pos.y = gcd[28].gd.pos.y-6;  gcd[29].gd.pos.width = 50;
    gcd[29].gd.flags = gg_enabled|gg_visible;
    gcd[29].gd.cid = CID_CDepth;
    gcd[29].creator = GTextFieldCreate;

    i = 30;
    gcd[i].gd.pos.x = 20-3; gcd[i].gd.pos.y = gcd[i==30?27:3].gd.pos.y+35-3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _STR_OK;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'O';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = Head_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 265-GIntGetResource(_NUM_Buttonsize)-20; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _STR_Cancel;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.handle_controlevent = Head_Cancel;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
    gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-2;
    gcd[i].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[i].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GDrawSetVisible(gw,true);
}

    TableFillup(tab);
table version = tgetfixed(tab,0);
fontrevision = tgetfixed(tab,4);
checksum = tgetlong(tab,8);
magic = tgetlong(tab,12);
flags = tgetushort(tab,16);
    0 baseline@y==0
    1 lbearing@x==0
    2 instrs depend on pointsize
    3 force ppem to integer
    4 instrs alter advance width
unitsPerEm = tgetushort(tab,18);
quaddate = tgetlong(tab,20)<<32 | tgetlong(tab,24)
xMin = (short) tgetushort(tab,28);
yMin = (short) tgetushort(tab,30);
xMax = (short) tgetushort(tab,32);
yMax = (short) tgetushort(tab,34);
macStyle = tgetushort(tab,36)
    0 bold
    1 italic
lowestReadablePixelSize = tgetushort(tab,38)
fontdirection = (short) tgetushort(tab,40)
    0 fully mixed
    1 only stronly ltor
    2 ltor or neutral
    -1 only strongly rtol
    -2 rtol or neutral
locaformat = (short) tgetushort(tab,42)
    0 shorts, 1 longs
glyphdataformat = (short) tgetushort(tab,44)
