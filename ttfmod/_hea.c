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
#include <gkeysym.h>

#define CID_Version	1001
#define CID_Ascender	1002
#define CID_Descender	1003
#define CID_LineGap	1004
#define CID_Advance	1005
#define CID_LTBearing	1006
#define CID_RBBearing	1007
#define CID_MaxExtent	1008
#define CID_CaretRise	1009
#define CID_CaretRun	1010
#define CID_CaretOffset	1011
#define CID_DataFormat	1012
#define CID_MetricsCount	1013

typedef struct headview /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
    unsigned int destroyed: 1;		/* window has been destroyed */
/* _head specials */
} HeaView;

static int _hea_processdata(TableView *tv) {
    int err = false;
    real version;
    int as, ds, lg, am, mlb, mrb, me, cr, crun, co, df, count;
    uint8 *data;
    int oldcount = tgetushort(tv->table,34);
    static int buts[] = { _STR_Yes, _STR_No, 0 };

    version = GetRealR(tv->gw,CID_Version,_STR_Version,&err);
    as = GetIntR(tv->gw,CID_Ascender,_STR_Ascent,&err);
    ds = GetIntR(tv->gw,CID_Descender,_STR_Descent,&err);
    lg = GetIntR(tv->gw,CID_LineGap,_STR_TypoLineGap,&err);
    am = GetIntR(tv->gw,CID_Advance,_STR_AdvanceMax,&err);
    mlb = GetIntR(tv->gw,CID_LTBearing,_STR_MinLeftBearing,&err);
    mrb = GetIntR(tv->gw,CID_RBBearing,_STR_MinRightBearing,&err);
    me = GetIntR(tv->gw,CID_MaxExtent,_STR_XMaxExtent,&err);
    cr = GetIntR(tv->gw,CID_CaretRise,_STR_CaretRise,&err);
    crun = GetIntR(tv->gw,CID_CaretRun,_STR_CaretRun,&err);
    co = GetIntR(tv->gw,CID_CaretOffset,_STR_CaretOffset,&err);
    df = GetIntR(tv->gw,CID_DataFormat,_STR_DataFormat,&err);
    count = GetIntR(tv->gw,CID_MetricsCount,_STR_MetricsCount,&err);
    if ( err )
return( false );
    if ( count!=oldcount ) {
	if ( GWidgetAskR(_STR_ChangingMetricsCount,buts,0,1,_STR_ReallyChangeMetricsCnt)==1 ) {
	    char buf[8]; unichar_t ubuf[8];
	    sprintf( buf, "%d", oldcount);
	    uc_strcpy(ubuf,buf);
	    GGadgetSetTitle(GWidgetGetControl(tv->gw,CID_MetricsCount),ubuf);
return( false );
	}
    }
    if ( tv->table->newlen<36 ) {
	free(tv->table->data);
	tv->table->data = gcalloc(36,1);
	tv->table->newlen = 36;
    }
    data = tv->table->data;
    ptputvfixed(data,version);
    ptputushort(data+4,as);
    ptputushort(data+6,ds);
    ptputushort(data+8,lg);
    ptputushort(data+10,am);
    ptputushort(data+12,mlb);
    ptputushort(data+14,mrb);
    ptputushort(data+16,me);
    ptputushort(data+18,cr);
    ptputushort(data+20,crun);
    ptputushort(data+22,co);
    ptputushort(data+32,df);
    ptputushort(data+34,count);
    ptputushort(data+6,lg);
    if ( !tv->table->changed ) {
	tv->table->changed = true;
	tv->table->container->changed = true;
	GDrawRequestExpose(tv->owner->v,NULL,false);
    }
return( true );
}

static int _hea_close(TableView *tv) {
    if ( _hea_processdata(tv)) {
	tv->destroyed = true;
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs _heafuncs = { _hea_close, _hea_processdata };

static int _Hea_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	((TableView *) GDrawGetUserData(gw))->destroyed = true;
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int _Hea_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    HeaView *hv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	hv = GDrawGetUserData(gw);
	_hea_close((TableView *) hv);
    }
return( true );
}

static int _hea_e_h(GWindow gw, GEvent *event) {
    HeaView *hv = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	hv->destroyed = true;
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

void _heaCreateEditor(Table *tab,TtfView *tfv) {
    HeaView *hv = gcalloc(1,sizeof(HeaView));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[34];
    GTextInfo label[34];
    int i;
    static unichar_t title[60];
    char version[8], asc[8], dsc[8], lg[8], advance[8], ltb[8], rb[8], extent[8],
	    crise[8], crun[8], coffset[8], df[8], mc[8];

    hv->table = tab;
    hv->virtuals = &_heafuncs;
    hv->owner = tfv;
    hv->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) hv;

    TableFillup(tab);

    title[0] = (tab->name>>24)&0xff;
    title[1] = (tab->name>>16)&0xff;
    title[2] = (tab->name>>8 )&0xff;
    title[3] = (tab->name    )&0xff;
    title[4] = ' ';
    u_strncpy(title+5, hv->font->fontname, sizeof(title)/sizeof(title[0])-6);
    
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
    pos.height = GDrawPointsToPixels(NULL,269);
    hv->gw = gw = GDrawCreateTopWindow(NULL,&pos,_hea_e_h,hv,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_Version;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 8+6; 
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    sprintf( version, "%.4g", tgetvfixed(tab,0) );
    label[1].text = (unichar_t *) version;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 80; gcd[1].gd.pos.y = gcd[0].gd.pos.y-6;  gcd[1].gd.pos.width = 50;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_Version;
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) (title[0]=='h'?_STR_TypoAscender:_STR_Ascent);
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = gcd[1].gd.pos.y+24+6; 
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    sprintf( asc, "%d", tgetushort(tab,4) );
    label[3].text = (unichar_t *) asc;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = gcd[1].gd.pos.x; gcd[3].gd.pos.y = gcd[2].gd.pos.y-6;  gcd[3].gd.pos.width = 50;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_Ascender;
    gcd[3].creator = GTextFieldCreate;

    label[4].text = (unichar_t *) (title[0]=='h'?_STR_TypoDescender:_STR_Descent);
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = 5+gcd[1].gd.pos.x+gcd[1].gd.pos.width; gcd[4].gd.pos.y = gcd[2].gd.pos.y; 
    gcd[4].gd.flags = gg_enabled|gg_visible;
    gcd[4].creator = GLabelCreate;

    sprintf( dsc, "%d", (short) tgetushort(tab,6) );
    label[5].text = (unichar_t *) dsc;
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = gcd[4].gd.pos.x+75; gcd[5].gd.pos.y = gcd[4].gd.pos.y-6;  gcd[5].gd.pos.width = 50;
    gcd[5].gd.flags = gg_enabled|gg_visible;
    gcd[5].gd.cid = CID_Descender;
    gcd[5].creator = GTextFieldCreate;

    label[6].text = (unichar_t *) _STR_TypoLineGap;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = gcd[4].gd.pos.y+24; 
    gcd[6].gd.flags = (title[0]=='h'?gg_enabled|gg_visible:gg_visible);
    gcd[6].creator = GLabelCreate;

    sprintf( lg, "%d", (short) tgetushort(tab,8) );
    label[7].text = (unichar_t *) lg;
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = gcd[1].gd.pos.x; gcd[7].gd.pos.y = gcd[6].gd.pos.y-6;  gcd[7].gd.pos.width = 50;
    gcd[7].gd.flags = (title[0]=='h'?gg_enabled|gg_visible:gg_visible);
    gcd[7].gd.cid = CID_LineGap;
    gcd[7].creator = GTextFieldCreate;

    label[8].text = (unichar_t *) _STR_AdvanceMax;
    label[8].text_in_resource = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.pos.x = 5; gcd[8].gd.pos.y = gcd[7].gd.pos.y+24+6; 
    gcd[8].gd.flags = gg_enabled|gg_visible;
    gcd[8].creator = GLabelCreate;

    sprintf( advance, "%d", tgetushort(tab,10) );
    label[9].text = (unichar_t *) advance;
    label[9].text_is_1byte = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.pos.x = gcd[1].gd.pos.x; gcd[9].gd.pos.y = gcd[8].gd.pos.y-6;  gcd[9].gd.pos.width = 50;
    gcd[9].gd.flags = gg_enabled|gg_visible;
    gcd[9].gd.cid = CID_Advance;
    gcd[9].creator = GTextFieldCreate;

    label[10].text = (unichar_t *) (title[0]=='h'?_STR_MinLeftBearing:_STR_MinTopBearing);
    label[10].text_in_resource = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.pos.x = gcd[0].gd.pos.x; gcd[10].gd.pos.y = gcd[8].gd.pos.y+24; 
    gcd[10].gd.flags = gg_enabled|gg_visible;
    gcd[10].creator = GLabelCreate;

    sprintf( ltb, "%d", (short) tgetushort(tab,12) );
    label[11].text = (unichar_t *) ltb;
    label[11].text_is_1byte = true;
    gcd[11].gd.label = &label[11];
    gcd[11].gd.pos.x = gcd[1].gd.pos.x; gcd[11].gd.pos.y = gcd[10].gd.pos.y-6;  gcd[11].gd.pos.width = 50;
    gcd[11].gd.flags = gg_enabled|gg_visible;
    gcd[11].gd.cid = CID_LTBearing;
    gcd[11].creator = GTextFieldCreate;

    label[12].text = (unichar_t *) (title[0]=='h'?_STR_MinRightBearing:_STR_MinBottomBearing);
    label[12].text_in_resource = true;
    gcd[12].gd.label = &label[12];
    gcd[12].gd.pos.x = gcd[4].gd.pos.x; gcd[12].gd.pos.y = gcd[9].gd.pos.y+24+6; 
    gcd[12].gd.flags = gg_enabled|gg_visible;
    gcd[12].creator = GLabelCreate;

    sprintf( rb, "%d", (short) tgetushort(tab,14) );
    label[13].text = (unichar_t *) rb;
    label[13].text_is_1byte = true;
    gcd[13].gd.label = &label[13];
    gcd[13].gd.pos.x = gcd[5].gd.pos.x; gcd[13].gd.pos.y = gcd[12].gd.pos.y-6;  gcd[13].gd.pos.width = 50;
    gcd[13].gd.flags = gg_enabled|gg_visible;
    gcd[13].gd.cid = CID_RBBearing;
    gcd[13].creator = GTextFieldCreate;

    label[14].text = (unichar_t *) (title[0]=='h'?_STR_XMaxExtent:_STR_YMaxExtent);
    label[14].text_in_resource = true;
    gcd[14].gd.label = &label[14];
    gcd[14].gd.pos.x = gcd[0].gd.pos.x; gcd[14].gd.pos.y = gcd[12].gd.pos.y+24; 
    gcd[14].gd.flags = gg_enabled|gg_visible;
    gcd[14].creator = GLabelCreate;

    sprintf( extent, "%d", tgetushort(tab,16) );
    label[15].text = (unichar_t *) extent;
    label[15].text_is_1byte = true;
    gcd[15].gd.label = &label[15];
    gcd[15].gd.pos.x = gcd[1].gd.pos.x; gcd[15].gd.pos.y = gcd[14].gd.pos.y-6;  gcd[15].gd.pos.width = 50;
    gcd[15].gd.flags = gg_enabled|gg_visible;
    gcd[15].gd.cid = CID_MaxExtent;
    gcd[15].creator = GTextFieldCreate;

    label[16].text = (unichar_t *) _STR_CaretRise;
    label[16].text_in_resource = true;
    gcd[16].gd.label = &label[16];
    gcd[16].gd.pos.x = 5; gcd[16].gd.pos.y = gcd[15].gd.pos.y+24+6; 
    gcd[16].gd.flags = gg_enabled|gg_visible;
    gcd[16].creator = GLabelCreate;

    sprintf( crise, "%d", tgetushort(tab,18) );
    label[17].text = (unichar_t *) crise;
    label[17].text_is_1byte = true;
    gcd[17].gd.label = &label[17];
    gcd[17].gd.pos.x = gcd[1].gd.pos.x; gcd[17].gd.pos.y = gcd[16].gd.pos.y-6;  gcd[17].gd.pos.width = 50;
    gcd[17].gd.flags = gg_enabled|gg_visible;
    gcd[17].gd.cid = CID_CaretRise;
    gcd[17].creator = GTextFieldCreate;

    label[18].text = (unichar_t *) _STR_CaretRun;
    label[18].text_in_resource = true;
    gcd[18].gd.label = &label[18];
    gcd[18].gd.pos.x = gcd[4].gd.pos.x; gcd[18].gd.pos.y = gcd[16].gd.pos.y;
    gcd[18].gd.flags = gg_enabled|gg_visible;
    gcd[18].creator = GLabelCreate;

    sprintf( crun, "%d", tgetushort(tab,20) );
    label[19].text = (unichar_t *) crun;
    label[19].text_is_1byte = true;
    gcd[19].gd.label = &label[19];
    gcd[19].gd.pos.x = gcd[5].gd.pos.x; gcd[19].gd.pos.y = gcd[18].gd.pos.y-6;  gcd[19].gd.pos.width = 50;
    gcd[19].gd.flags = gg_enabled|gg_visible;
    gcd[19].gd.cid = CID_CaretRun;
    gcd[19].creator = GTextFieldCreate;

    label[20].text = (unichar_t *) _STR_CaretOffset;
    label[20].text_in_resource = true;
    gcd[20].gd.label = &label[20];
    gcd[20].gd.pos.x = gcd[0].gd.pos.x; gcd[20].gd.pos.y = gcd[18].gd.pos.y+24; 
    gcd[20].gd.flags = gg_enabled|gg_visible;
    gcd[20].creator = GLabelCreate;

    sprintf( coffset, "%d", tgetushort(tab,22) );
    label[21].text = (unichar_t *) coffset;
    label[21].text_is_1byte = true;
    gcd[21].gd.label = &label[21];
    gcd[21].gd.pos.x = gcd[1].gd.pos.x; gcd[21].gd.pos.y = gcd[20].gd.pos.y-6;  gcd[21].gd.pos.width = 50;
    gcd[21].gd.flags = gg_enabled|gg_visible;
    gcd[21].gd.cid = CID_CaretOffset;
    gcd[21].creator = GTextFieldCreate;

    label[22].text = (unichar_t *) _STR_DataFormat;
    label[22].text_in_resource = true;
    gcd[22].gd.label = &label[22];
    gcd[22].gd.pos.x = 5; gcd[22].gd.pos.y = gcd[21].gd.pos.y+24+6; 
    gcd[22].gd.flags = gg_enabled|gg_visible;
    gcd[22].creator = GLabelCreate;

    sprintf( df, "%d", tgetushort(tab,32) );
    label[23].text = (unichar_t *) df;
    label[23].text_is_1byte = true;
    gcd[23].gd.label = &label[23];
    gcd[23].gd.pos.x = gcd[1].gd.pos.x; gcd[23].gd.pos.y = gcd[22].gd.pos.y-6;  gcd[23].gd.pos.width = 50;
    gcd[23].gd.flags = gg_enabled|gg_visible;
    gcd[23].gd.cid = CID_DataFormat;
    gcd[23].creator = GTextFieldCreate;

    label[24].text = (unichar_t *) _STR_MetricsCount;
    label[24].text_in_resource = true;
    gcd[24].gd.label = &label[24];
    gcd[24].gd.pos.x = gcd[4].gd.pos.x; gcd[24].gd.pos.y = gcd[22].gd.pos.y; 
    gcd[24].gd.flags = gg_enabled|gg_visible;
    gcd[24].creator = GLabelCreate;

    sprintf( mc, "%d", tgetushort(tab,34) );
    label[25].text = (unichar_t *) mc;
    label[25].text_is_1byte = true;
    gcd[25].gd.label = &label[25];
    gcd[25].gd.pos.x = gcd[5].gd.pos.x; gcd[25].gd.pos.y = gcd[24].gd.pos.y-6;  gcd[25].gd.pos.width = 50;
    gcd[25].gd.flags = gg_enabled|gg_visible;
    gcd[25].gd.cid = CID_MetricsCount;
    gcd[25].creator = GTextFieldCreate;

    i = 26;
    gcd[i].gd.pos.x = 20-3; gcd[i].gd.pos.y = gcd[25].gd.pos.y+35-3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _STR_OK;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'O';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = _Hea_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 265-GIntGetResource(_NUM_Buttonsize)-20; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _STR_Cancel;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.handle_controlevent = _Hea_Cancel;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
    gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
    gcd[i].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[i].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GDrawSetVisible(gw,true);
}

void _heaChangeLongMetrics(Table *_hea,int newlongcnt) {
    char buf[8]; unichar_t ubuf[8];

    TableFillup(_hea);
    if ( _hea->tv!=NULL ) {
	sprintf( buf, "%d", newlongcnt );
	uc_strcpy(ubuf, buf);
	GGadgetSetTitle(GWidgetGetControl(_hea->tv->gw,CID_MetricsCount),ubuf);
    }
    ptputushort(_hea->data+34,newlongcnt);
    if ( !_hea->changed ) {
	_hea->changed = true;
	_hea->container->changed = true;
	GDrawRequestExpose(_hea->container->tfv->v,NULL,false);
    }
}
