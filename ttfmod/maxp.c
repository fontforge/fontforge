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
#include <gkeysym.h>
#include <ustring.h>
#include <utype.h>
#include <locale.h>

extern struct lconv localeinfo;

#define CID_Version	1000
#define CID_Glyphs	1001
#define CID_Points	1002
#define CID_Contours	1003
#define CID_CPoints	1004
#define CID_CContours	1005
#define CID_Zones	1006
#define CID_TPoints	1007
#define CID_Storage	1008
#define CID_FDefs	1009
#define CID_IDefs	1010
#define CID_SEl		1011
#define CID_SOI		1012
#define CID_CEl		1013
#define CID_CDepth	1014

static unichar_t one[] = { '1', '\0' };
static unichar_t pointfive[] = { '.', '5', '\0' };
static GTextInfo maxpversions[] = {
    { (unichar_t *) one, NULL, 0, 0, (void *) 0x10000},
    { (unichar_t *) pointfive, NULL, 0, 0, (void *) 0x5000},
    { NULL }};

typedef struct maxpview /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
    unsigned int destroyed: 1;		/* window has been destroyed */
/* maxp specials */
} MaxpView;

static int maxp_processdata(TableView *tv) {
    int err = false;
    real version;
    int gc, points, contours,cpoints,ccontours,zones,tpoints,store,fdefs,idefs,
	sel,sinstrs,cinstrs,cdepth;
    uint8 *data;
    int len;
    static int buts[] = { _STR_Yes, _STR_Cancel, 0 };

    version = GetRealR(tv->gw,CID_Version,_STR_Version,&err);
    gc = GetIntR(tv->gw,CID_Glyphs,_STR_Glyphs,&err);
    if ( gc!=tv->font->glyph_cnt ) {
	if ( GWidgetAskR(_STR_ChangingGlyphCnt,buts,0,1,_STR_ReallyChangeGlyphCnt)==1 ) {
	    char buf[8]; unichar_t ubuf[8];
	    sprintf( buf, "%d", tv->font->glyph_cnt );
	    uc_strcpy(ubuf,buf);
	    GGadgetSetTitle(GWidgetGetControl(tv->gw,CID_Glyphs),ubuf);
return( false );
	}
	tv->font->glyph_cnt = gc;
	tv->table->container->gcchanged = true;
    }
    len = 6;
    if ( version>=1 ) {
	points = GetIntR(tv->gw,CID_Points,_STR_Points,&err);
	contours = GetIntR(tv->gw,CID_Contours,_STR_Contours,&err);
	cpoints = GetIntR(tv->gw,CID_CPoints,_STR_CompositPoints,&err);
	ccontours = GetIntR(tv->gw,CID_CContours,_STR_CompositContours,&err);
	zones = GetIntR(tv->gw,CID_Zones,_STR_Zones,&err);
	tpoints = GetIntR(tv->gw,CID_TPoints,_STR_TwilightPoints,&err);
	store = GetIntR(tv->gw,CID_Storage,_STR_Storage,&err);
	fdefs = GetIntR(tv->gw,CID_FDefs,_STR_FDEFs,&err);
	idefs = GetIntR(tv->gw,CID_IDefs,_STR_IDEFs,&err);
	sel = GetIntR(tv->gw,CID_SEl,_STR_StackElements,&err);
	sinstrs = GetIntR(tv->gw,CID_SOI,_STR_SizeOfInstructions,&err);
	cinstrs = GetIntR(tv->gw,CID_CEl,_STR_ComponentElements,&err);
	cdepth = GetIntR(tv->gw,CID_CDepth,_STR_ComponentDepth,&err);
	len = 32;
    }
    if ( err )
return( false );
    data = galloc(len);
    ptputvfixed(data,version);
    ptputushort(data+4,gc);
    if ( version>=1 ) {
	ptputushort(data+6,points);
	ptputushort(data+8,contours);
	ptputushort(data+10,cpoints);
	ptputushort(data+12,ccontours);
	ptputushort(data+14,zones);
	ptputushort(data+16,tpoints);
	ptputushort(data+18,store);
	ptputushort(data+20,fdefs);
	ptputushort(data+22,idefs);
	ptputushort(data+24,sel);
	ptputushort(data+26,sinstrs);
	ptputushort(data+28,cinstrs);
	ptputushort(data+30,cdepth);
    }
    free(tv->table->data);
    tv->table->data = data;
    tv->table->newlen = len;
    if ( !tv->table->changed ) {
	tv->table->changed = true;
	tv->table->container->changed = true;
	GDrawRequestExpose(tv->owner->v,NULL,false);
    }
return( true );
}

static int maxp_close(TableView *tv) {
    if ( maxp_processdata(tv)) {
	tv->destroyed = true;
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs maxpfuncs = { maxp_close, maxp_processdata };

static int maxp_VersionChange(GGadget *g, GEvent *e) {
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

static int Maxp_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	((TableView *) GDrawGetUserData(gw))->destroyed = true;
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int Maxp_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    MaxpView *mv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	mv = GDrawGetUserData(gw);
	maxp_close((TableView *) mv);
    }
return( true );
}

static int maxp_e_h(GWindow gw, GEvent *event) {
    MaxpView *mv = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	mv->destroyed = true;
	GDrawDestroyWindow(mv->gw);
    } else if ( event->type == et_destroy ) {
	mv->table->tv = NULL;
	free(mv);
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 ) {
	    TableHelp(mv->table->name);
	    system("netscape http://partners.adobe.com/asn/developer/opentype/maxp.html &");
return( true );
	} else if (( event->u.chr.state&ksm_control ) &&
		(event->u.chr.keysym=='q' || event->u.chr.keysym=='Q')) {
	    MenuExit(NULL,NULL,NULL);
	}
return( false );
    }
return( true );
}

void maxpCreateEditor(Table *tab,TtfView *tfv) {
    MaxpView *mv = gcalloc(1,sizeof(MaxpView));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[34];
    GTextInfo label[34];
    int short_table, i;
    static unichar_t title[60] = { 'm', 'a', 'x', 'p', ' ',  '\0' };
    char version[20], glyphs[8], points[8], contours[8], cpoints[8],
	ccontours[8], zones[8], tpoints[8], storage[8], fdefs[8], idefs[8],
	sels[8], soi[8], cels[8], cdep[8];

    mv->table = tab;
    mv->virtuals = &maxpfuncs;
    mv->owner = tfv;
    mv->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) mv;

    TableFillup(tab);
    
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    u_strncpy(title+5, mv->font->fontname, sizeof(title)/sizeof(title[0])-6);
    title[sizeof(title)/sizeof(title[0])-1] = '\0';
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,270);
    pos.height = GDrawPointsToPixels(NULL,267);
    mv->gw = gw = GDrawCreateTopWindow(NULL,&pos,maxp_e_h,mv,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_Version;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+6; 
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    short_table = tgetvfixed(tab,0)==.5;
    sprintf( version, "%.4g", tgetvfixed(tab,0) );
    pointfive[0] = localeinfo.decimal_point[0];
    label[1].text = (unichar_t *) version;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 80; gcd[1].gd.pos.y = gcd[0].gd.pos.y-6;  gcd[1].gd.pos.width = 50;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_Version;
    gcd[1].gd.u.list = maxpversions;
    gcd[1].gd.handle_controlevent = maxp_VersionChange;
    gcd[1].creator = GListFieldCreate;

    label[2].text = (unichar_t *) _STR_Glyphs;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = gcd[1].gd.pos.y+24+6; 
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    sprintf( glyphs, "%d", tgetushort(tab,4) );
    label[3].text = (unichar_t *) glyphs;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = gcd[1].gd.pos.x; gcd[3].gd.pos.y = gcd[2].gd.pos.y-6;  gcd[3].gd.pos.width = 50;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_Glyphs;
    gcd[3].creator = GTextFieldCreate;

	label[4].text = (unichar_t *) _STR_Points;
	label[4].text_in_resource = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = gcd[3].gd.pos.y+24+6; 
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].gd.cid = CID_Points+1000;
	gcd[4].creator = GLabelCreate;

	sprintf( points, "%d", tgetushort(tab,6) );
	if ( short_table ) points[0] = '\0';
	label[5].text = (unichar_t *) points;
	label[5].text_is_1byte = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.pos.x = gcd[1].gd.pos.x; gcd[5].gd.pos.y = gcd[4].gd.pos.y-6;  gcd[5].gd.pos.width = 50;
	gcd[5].gd.flags = gg_enabled|gg_visible;
	gcd[5].gd.cid = CID_Points;
	gcd[5].creator = GTextFieldCreate;

	label[6].text = (unichar_t *) _STR_Contours;
	label[6].text_in_resource = true;
	gcd[6].gd.label = &label[6];
	gcd[6].gd.pos.x = 5+gcd[1].gd.pos.x+gcd[1].gd.pos.width; gcd[6].gd.pos.y = gcd[4].gd.pos.y; 
	gcd[6].gd.flags = gg_enabled|gg_visible;
	gcd[6].gd.cid = CID_Contours+1000;
	gcd[6].creator = GLabelCreate;

	sprintf( contours, "%d", tgetushort(tab,8) );
	if ( short_table ) contours[0] = '\0';
	label[7].text = (unichar_t *) contours;
	label[7].text_is_1byte = true;
	gcd[7].gd.label = &label[7];
	gcd[7].gd.pos.x = gcd[6].gd.pos.x+75; gcd[7].gd.pos.y = gcd[6].gd.pos.y-6;  gcd[7].gd.pos.width = 50;
	gcd[7].gd.flags = gg_enabled|gg_visible;
	gcd[7].gd.cid = CID_Contours;
	gcd[7].creator = GTextFieldCreate;

	label[8].text = (unichar_t *) _STR_CompositPoints;
	label[8].text_in_resource = true;
	gcd[8].gd.label = &label[8];
	gcd[8].gd.pos.x = 5; gcd[8].gd.pos.y = gcd[5].gd.pos.y+24+6; 
	gcd[8].gd.flags = gg_enabled|gg_visible;
	gcd[8].gd.cid = CID_CPoints+1000;
	gcd[8].creator = GLabelCreate;

	sprintf( cpoints, "%d", tgetushort(tab,10) );
	if ( short_table ) cpoints[0] = '\0';
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
	if ( short_table ) ccontours[0] = '\0';
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
	if ( short_table ) zones[0] = '\0';
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
	if ( short_table ) tpoints[0] = '\0';
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
	if ( short_table ) storage[0] = '\0';
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
	if ( short_table ) fdefs[0] = '\0';
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
	if ( short_table ) idefs[0] = '\0';
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
	if ( short_table ) sels[0] = '\0';
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
	if ( short_table ) soi[0] = '\0';
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
	if ( short_table ) cels[0] = '\0';
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
	if ( short_table ) cdep[0] = '\0';
	label[29].text = (unichar_t *) cdep;
	label[29].text_is_1byte = true;
	gcd[29].gd.label = &label[29];
	gcd[29].gd.pos.x = gcd[7].gd.pos.x; gcd[29].gd.pos.y = gcd[28].gd.pos.y-6;  gcd[29].gd.pos.width = 50;
	gcd[29].gd.flags = gg_enabled|gg_visible;
	gcd[29].gd.cid = CID_CDepth;
	gcd[29].creator = GTextFieldCreate;

	if ( short_table )
	    for ( i=4; i<30; ++i )
		gcd[i].gd.flags &= ~gg_enabled;
	i = 30;

    gcd[i].gd.pos.x = 20-3; gcd[i].gd.pos.y = gcd[i==30?27:3].gd.pos.y+35-3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _STR_OK;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'O';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = Maxp_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 265-GIntGetResource(_NUM_Buttonsize)-20; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _STR_Cancel;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.handle_controlevent = Maxp_Cancel;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
    gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
    gcd[i].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[i].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GDrawSetVisible(gw,true);
}
