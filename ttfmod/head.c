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
#include <ustring.h>
#include <utype.h>
#include <gkeysym.h>
#include <time.h>

#define CID_Version	1001
#define CID_Revision	1002
#define CID_Checksum	1003
#define CID_MagicNumber	1004
#define CID_Flags	1005
#define CID_FlagsList	1105
#define CID_UnitsPerEm	1006
#define CID_Created	1007
#define CID_Modified	1107
#define CID_xMin	1008
#define CID_yMin	1009
#define CID_xMax	1010
#define CID_yMax	1011
#define CID_MacStyle	1012
#define CID_MacStyleList	1112
#define CID_SmallestSize	1013
#define CID_FontDir	1014
#define CID_LocaFormat	1015
#define CID_GlyphFormat	1016

static unichar_t mnum[] = { '5','f','0','f','3','c','f','5',  '\0' };
static GTextInfo magicnumbers[] = {
    { (unichar_t *) mnum},
    { NULL }};
static GTextInfo flagslist[] = {
    { (unichar_t *) _STR_Baseliney0, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LBearingx0, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_InstrsDepOnPointsize, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ForcePPEMInt, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_InstrsAlterWidth, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
/* Apple only */
    { (unichar_t *) _STR_VerticalLayout, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
	/* 6 MBZ */
    { (unichar_t *) _STR_Arabic, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GXWithMetamorphosis, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ContainsRightToLeft, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Indic, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
/* Adobe only */
    { (unichar_t *) _STR_Lossless, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_FontConverted, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ClearType, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo macstylelist[] = {
    { (unichar_t *) _STR_Bold, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Italic, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
/* Apple only */
    { (unichar_t *) _STR_Underline, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Outline, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Shadow, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Condensed, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Extended, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo directionlist[] = {
    { (unichar_t *) _STR_RToLOrNeutral, NULL, 0, 0, (void *) -2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StronglyRightToLeft, NULL, 0, 0, (void *) -1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_FullyMixed, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StronglyLeftToRight, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LtoROrNeutral, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo localist[] = {
    { (unichar_t *) _STR_16bit, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_32bit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

typedef struct headview /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
    unsigned int destroyed: 1;		/* window has been destroyed */
/* head specials */
} HeadView;

static int head_processdata(TableView *tv) {
    int err = false;
    real version, revision;
    int magic, flags, ppem, xmin, ymin, xmax, ymax, macstyle, small, fdir,
	locaform, glyfform;
    uint32 created[2];
    uint8 *data;

    version = GetRealR(tv->gw,CID_Version,_STR_Version,&err);
    revision = GetRealR(tv->gw,CID_Revision,_STR_FontRevision,&err);
    /* Ignore checksum, that has to be calculated automatically */
    magic = GetHexR(tv->gw,CID_MagicNumber,_STR_MagicNumber,&err);
    flags = GetHexR(tv->gw,CID_Flags,_STR_Flags,&err);
    ppem = GetIntR(tv->gw,CID_UnitsPerEm,_STR_UnitsPerEm,&err);
    GetDateR(tv->gw,CID_Created,_STR_Created,created,&err);
    /* modified date, like checksum, must be set on save */
    xmin = GetIntR(tv->gw,CID_xMin,_STR_XMin,&err);
    ymin = GetIntR(tv->gw,CID_yMin,_STR_YMin,&err);
    xmax = GetIntR(tv->gw,CID_xMax,_STR_XMax,&err);
    ymax = GetIntR(tv->gw,CID_yMax,_STR_YMax,&err);
    macstyle = GetHexR(tv->gw,CID_MacStyle,_STR_MacStyle,&err);
    small = GetIntR(tv->gw,CID_SmallestSize,_STR_SmallestSize,&err);
    fdir = GetListR(tv->gw,CID_FontDir,_STR_FontDirection,&err);
    locaform = GetListR(tv->gw,CID_LocaFormat,_STR_LocaFormat,&err);
    glyfform = GetIntR(tv->gw,CID_GlyphFormat,_STR_GlyfFormat,&err);
    if ( err )
return( false );
    if ( tv->table->newlen<54 ) {
	free(tv->table->data);
	tv->table->data = gcalloc(54,1);
	tv->table->newlen = 54;
    }
    data = tv->table->data;
    ptputvfixed(data,version);
    ptputfixed(data+4,revision);
    /* checksum */
    ptputlong(data+12,magic);
    ptputushort(data+16,flags);
    ptputushort(data+18,ppem);
    ptputlong(data+20,created[1]); ptputlong(data+24,created[0]);
    /* modified */
    ptputushort(data+36,xmin);
    ptputushort(data+38,ymin);
    ptputushort(data+40,xmax);
    ptputushort(data+42,ymax);
    ptputushort(data+44,macstyle);
    ptputushort(data+46,small);
    ptputushort(data+48,fdir);
    ptputushort(data+50,locaform);
    ptputushort(data+52,glyfform);
    if ( !tv->table->changed ) {
	tv->table->changed = true;
	tv->table->container->changed = true;
	GDrawRequestExpose(tv->owner->v,NULL,false);
    }

return( true );
}

static int head_close(TableView *tv) {
    if ( head_processdata(tv)) {
	tv->destroyed = true;
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs headfuncs = { head_close, head_processdata };

static int head_FlagsChange(GGadget *g, GEvent *e) {
    int32 flags;
    int len,i,bit,set;

    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_textchanged )) {
	const unichar_t *ret;
	unichar_t *end;
	GWindow gw = GGadgetGetWindow(g);
	GGadget *list;
	int othercid = GGadgetGetCid(g)==CID_Flags?CID_FlagsList:CID_MacStyleList;
	GTextInfo *tilist = GGadgetGetCid(g)==CID_Flags?flagslist:macstylelist;

	ret = _GGadgetGetTitle(g);
	flags = u_strtoul(ret,&end,16);

	list = GWidgetGetControl(gw,othercid);

	for ( i=0; tilist[i].text!=NULL; ++i ) {
	    bit = (int) (tilist[i].userdata);
	    set = (flags&(1<<bit))?1 : 0;
	    GGadgetSelectListItem(list,i,set);
	}
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GWindow gw = GGadgetGetWindow(g);
	char ranges[40]; unichar_t ur[40];
	GTextInfo **list = GGadgetGetList(g,&len);
	int othercid = GGadgetGetCid(g)==CID_FlagsList?CID_Flags:CID_MacStyle;
	GGadget *field = GWidgetGetControl(gw,othercid);

	flags = 0;
	for ( i=0; i<len; ++i )
	    if ( list[i]->selected ) {
		bit = ((int) (list[i]->userdata));
		flags |= (1<<bit);
	    }

	sprintf( ranges, "%04x", flags);
	uc_strcpy(ur,ranges);
	GGadgetSetTitle(field,ur);
    }
return( true );
}

static int Head_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	((TableView *) GDrawGetUserData(gw))->destroyed = true;
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int Head_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    HeadView *hv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	hv = GDrawGetUserData(gw);
	head_close((TableView *) hv);
    }
return( true );
}

static int head_e_h(GWindow gw, GEvent *event) {
    HeadView *hv = GDrawGetUserData(gw);
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

static void quad2date(char str[80], int32 date1, int32 date2 ) {
    time_t date;
    struct tm *tm;
    /* convert from a time based on 1904 to a unix time based on 1970 */
    if ( sizeof(time_t)>32 ) {
	/* as unixes switch over to 64 bit times, this will be the better */
	/*  solution */
	date = ((((time_t) date1)<<16)<<16) | date2;
	date -= ((time_t) 60)*60*24*365*(70-4);
	date -= 60*60*24*(70-4)/4;		/* leap years */
    } else {
	/* But for now, we're stuck with this on most machines */
	int year[2], date1904[4], i;
	date1904[0] = date1>>16;
	date1904[1] = date1&0xffff;
	date1904[2] = date2>>16;
	date1904[3] = date2&0xffff;
	year[0] = 60*60*24*365;
	year[1] = year[0]>>16; year[0] &= 0xffff;
	for ( i=4; i<70; ++i ) {
	    date1904[3] -= year[0];
	    date1904[2] -= year[1];
	    if ( (i&3)==0 )
		date1904[3] -= 60*60*24;
	    date1904[2] += date1904[3]>>16;
	    date1904[3] &= 0xffff;
	    date1904[1] += date1904[2]>>16;
	    date1904[2] &= 0xffff;
	}
	date = ((date1904[2]<<16) | date1904[3]);
    }
    tm = localtime(&date);
    sprintf( str, "%d-%d-%d %d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1,
	    tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );
}

void headViewUpdateModifiedCheck(Table *tab) {
    char modified[100];
    unichar_t ubuf[100];

    if ( tab->tv!=NULL ) {
	quad2date(modified, tgetlong(tab,28), tgetlong(tab,32));
	uc_strcpy(ubuf,modified);
	GGadgetSetTitle(GWidgetGetControl(tab->tv->gw,CID_Modified),ubuf);
	sprintf( modified, "%08x", tgetlong(tab,8) );
	uc_strcpy(ubuf,modified);
	GGadgetSetTitle(GWidgetGetControl(tab->tv->gw,CID_Checksum),ubuf);
    }
}

void headCreateEditor(Table *tab,TtfView *tfv) {
    HeadView *hv = gcalloc(1,sizeof(HeadView));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[41];
    GTextInfo label[41];
    int i;
    static unichar_t title[60];
    char version[10], revision[10], checksum[10], magic[10], flags[10], em[10],
	    created[80], modified[80], xmin[10], ymin[10], xmax[10], ymax[10],
	    macstyle[8], smallest[8], glyf[8];

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
    u_strncpy(title+5, hv->font->fontname, sizeof(title)/sizeof(title[0])-6);
    title[sizeof(title)/sizeof(title[0])-1] = '\0';
    
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,370);
    pos.height = GDrawPointsToPixels(NULL,384);
    hv->gw = gw = GDrawCreateTopWindow(NULL,&pos,head_e_h,hv,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_Version;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+6; 
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    sprintf( version, "%.4g", tgetvfixed(tab,0) );
    label[1].text = (unichar_t *) version;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 80; gcd[1].gd.pos.y = gcd[0].gd.pos.y-6;  gcd[1].gd.pos.width = 100;
    gcd[1].gd.flags = gg_enabled|gg_visible;
    gcd[1].gd.cid = CID_Version;
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
    gcd[3].gd.pos.x = gcd[1].gd.pos.x; gcd[3].gd.pos.y = gcd[2].gd.pos.y-6;  gcd[3].gd.pos.width = gcd[1].gd.pos.width;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_Revision;
    gcd[3].creator = GTextFieldCreate;

    label[4].text = (unichar_t *) _STR_Checksum;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = gcd[3].gd.pos.y+24+6; 
    gcd[4].gd.flags = gg_visible;
    gcd[4].creator = GLabelCreate;

    sprintf( checksum, "%08x", tgetlong(tab,8) );
    label[5].text = (unichar_t *) checksum;
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = gcd[1].gd.pos.x; gcd[5].gd.pos.y = gcd[4].gd.pos.y-6;  gcd[5].gd.pos.width = gcd[1].gd.pos.width;
    gcd[5].gd.flags = gg_visible;
    gcd[5].gd.cid = CID_Checksum;
    gcd[5].creator = GTextFieldCreate;

    label[6].text = (unichar_t *) _STR_MagicNumber;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 5+gcd[1].gd.pos.x+gcd[1].gd.pos.width; gcd[6].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[6].gd.flags = gg_enabled|gg_visible;
    gcd[6].creator = GLabelCreate;

    sprintf( magic, "%x", tgetlong(tab,12) );
    label[7].text = (unichar_t *) magic;
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = gcd[6].gd.pos.x+75; gcd[7].gd.pos.y = gcd[6].gd.pos.y-6;  gcd[7].gd.pos.width = gcd[1].gd.pos.width;
    gcd[7].gd.flags = gg_enabled|gg_visible;
    gcd[7].gd.cid = CID_MagicNumber;
    gcd[7].gd.u.list = magicnumbers;
    gcd[7].creator = GListFieldCreate;

    label[8].text = (unichar_t *) _STR_Flags;
    label[8].text_in_resource = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.pos.x = 5; gcd[8].gd.pos.y = gcd[5].gd.pos.y+24+6; 
    gcd[8].gd.flags = gg_enabled|gg_visible;
    gcd[8].creator = GLabelCreate;

    sprintf( flags, "%04x", tgetushort(tab,16) );
    label[9].text = (unichar_t *) flags;
    label[9].text_is_1byte = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.pos.x = gcd[1].gd.pos.x; gcd[9].gd.pos.y = gcd[8].gd.pos.y-6;  gcd[9].gd.pos.width = gcd[1].gd.pos.width;
    gcd[9].gd.flags = gg_enabled|gg_visible;
    gcd[9].gd.handle_controlevent = head_FlagsChange;
    gcd[9].gd.cid = CID_Flags;
    gcd[9].creator = GTextFieldCreate;

    gcd[34].gd.pos.x = gcd[9].gd.pos.x+gcd[9].gd.pos.width+10;
    gcd[34].gd.pos.y = gcd[9].gd.pos.y;
    gcd[34].gd.pos.height = 57; gcd[34].gd.pos.width = 120;
    gcd[34].gd.flags = gg_enabled|gg_visible | gg_list_multiplesel;
    gcd[34].gd.u.list = flagslist;
    gcd[34].gd.cid = CID_FlagsList;
    gcd[34].gd.handle_controlevent = head_FlagsChange;
    gcd[34].creator = GListCreate;

    label[10].text = (unichar_t *) _STR_UnitsPerEm;
    label[10].text_in_resource = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.pos.x = gcd[0].gd.pos.x; gcd[10].gd.pos.y = gcd[8].gd.pos.y+60;
    gcd[10].gd.flags = gg_enabled|gg_visible;
    gcd[10].creator = GLabelCreate;

    sprintf( em, "%d", tgetushort(tab,18) );
    label[11].text = (unichar_t *) em;
    label[11].text_is_1byte = true;
    gcd[11].gd.label = &label[11];
    gcd[11].gd.pos.x = gcd[1].gd.pos.x; gcd[11].gd.pos.y = gcd[10].gd.pos.y-6;  gcd[11].gd.pos.width = gcd[1].gd.pos.width;
    gcd[11].gd.flags = gg_enabled|gg_visible;
    gcd[11].gd.cid = CID_UnitsPerEm;
    gcd[11].creator = GTextFieldCreate;

    label[12].text = (unichar_t *) _STR_Created;
    label[12].text_in_resource = true;
    gcd[12].gd.label = &label[12];
    gcd[12].gd.pos.x = 5; gcd[12].gd.pos.y = gcd[11].gd.pos.y+24+6; 
    gcd[12].gd.flags = gg_enabled|gg_visible;
    gcd[12].creator = GLabelCreate;

    quad2date(created, tgetlong(tab,20), tgetlong(tab,24));
    label[13].text = (unichar_t *) created;
    label[13].text_is_1byte = true;
    gcd[13].gd.label = &label[13];
    gcd[13].gd.pos.x = gcd[1].gd.pos.x-30; gcd[13].gd.pos.y = gcd[12].gd.pos.y-6;  gcd[13].gd.pos.width = gcd[1].gd.pos.width+30;
    gcd[13].gd.flags = gg_enabled|gg_visible;
    gcd[13].gd.cid = CID_Created;
    gcd[13].creator = GTextFieldCreate;

    label[14].text = (unichar_t *) _STR_Modified;
    label[14].text_in_resource = true;
    gcd[14].gd.label = &label[14];
    gcd[14].gd.pos.x = gcd[6].gd.pos.x; gcd[14].gd.pos.y = gcd[12].gd.pos.y; 
    gcd[14].gd.flags = gg_visible;
    gcd[14].creator = GLabelCreate;

    quad2date(modified, tgetlong(tab,28), tgetlong(tab,32));
    label[15].text = (unichar_t *) modified;
    label[15].text_is_1byte = true;
    gcd[15].gd.label = &label[15];
    gcd[15].gd.pos.x = gcd[7].gd.pos.x-30; gcd[15].gd.pos.y = gcd[14].gd.pos.y-6;  gcd[15].gd.pos.width = gcd[1].gd.pos.width+30;
    gcd[15].gd.flags = gg_visible;
    gcd[15].gd.cid = CID_Modified;
    gcd[15].creator = GTextFieldCreate;

    label[16].text = (unichar_t *) _STR_XMin;
    label[16].text_in_resource = true;
    gcd[16].gd.label = &label[16];
    gcd[16].gd.pos.x = gcd[0].gd.pos.x; gcd[16].gd.pos.y = gcd[15].gd.pos.y+24+6; 
    gcd[16].gd.flags = gg_enabled|gg_visible;
    gcd[16].creator = GLabelCreate;

    sprintf( xmin, "%d", (short) tgetushort(tab,36) );
    label[17].text = (unichar_t *) xmin;
    label[17].text_is_1byte = true;
    gcd[17].gd.label = &label[17];
    gcd[17].gd.pos.x = gcd[1].gd.pos.x; gcd[17].gd.pos.y = gcd[16].gd.pos.y-6;  gcd[17].gd.pos.width = gcd[1].gd.pos.width;
    gcd[17].gd.flags = gg_enabled|gg_visible;
    gcd[17].gd.cid = CID_xMin;
    gcd[17].creator = GTextFieldCreate;

    label[18].text = (unichar_t *) _STR_YMin;
    label[18].text_in_resource = true;
    gcd[18].gd.label = &label[18];
    gcd[18].gd.pos.x = gcd[6].gd.pos.x; gcd[18].gd.pos.y = gcd[16].gd.pos.y;
    gcd[18].gd.flags = gg_enabled|gg_visible;
    gcd[18].creator = GLabelCreate;

    sprintf( ymin, "%d", (short) tgetushort(tab,38) );
    label[19].text = (unichar_t *) ymin;
    label[19].text_is_1byte = true;
    gcd[19].gd.label = &label[19];
    gcd[19].gd.pos.x = gcd[7].gd.pos.x; gcd[19].gd.pos.y = gcd[18].gd.pos.y-6;  gcd[19].gd.pos.width = gcd[1].gd.pos.width;
    gcd[19].gd.flags = gg_enabled|gg_visible;
    gcd[19].gd.cid = CID_yMin;
    gcd[19].creator = GTextFieldCreate;

    label[20].text = (unichar_t *) _STR_XMax;
    label[20].text_in_resource = true;
    gcd[20].gd.label = &label[20];
    gcd[20].gd.pos.x = gcd[0].gd.pos.x; gcd[20].gd.pos.y = gcd[18].gd.pos.y+24; 
    gcd[20].gd.flags = gg_enabled|gg_visible;
    gcd[20].creator = GLabelCreate;

    sprintf( xmax, "%d", (short) tgetushort(tab,40) );
    label[21].text = (unichar_t *) xmax;
    label[21].text_is_1byte = true;
    gcd[21].gd.label = &label[21];
    gcd[21].gd.pos.x = gcd[1].gd.pos.x; gcd[21].gd.pos.y = gcd[20].gd.pos.y-6;  gcd[21].gd.pos.width = gcd[1].gd.pos.width;
    gcd[21].gd.flags = gg_enabled|gg_visible;
    gcd[21].gd.cid = CID_xMax;
    gcd[21].creator = GTextFieldCreate;

    label[22].text = (unichar_t *) _STR_YMax;
    label[22].text_in_resource = true;
    gcd[22].gd.label = &label[22];
    gcd[22].gd.pos.x = gcd[6].gd.pos.x; gcd[22].gd.pos.y = gcd[20].gd.pos.y;
    gcd[22].gd.flags = gg_enabled|gg_visible;
    gcd[22].creator = GLabelCreate;

    sprintf( ymax, "%d", (short) tgetushort(tab,42) );
    label[23].text = (unichar_t *) ymax;
    label[23].text_is_1byte = true;
    gcd[23].gd.label = &label[23];
    gcd[23].gd.pos.x = gcd[7].gd.pos.x; gcd[23].gd.pos.y = gcd[22].gd.pos.y-6;  gcd[23].gd.pos.width = gcd[1].gd.pos.width;
    gcd[23].gd.flags = gg_enabled|gg_visible;
    gcd[23].gd.cid = CID_yMax;
    gcd[23].creator = GTextFieldCreate;

    label[24].text = (unichar_t *) _STR_MacStyle;
    label[24].text_in_resource = true;
    gcd[24].gd.label = &label[24];
    gcd[24].gd.pos.x = gcd[0].gd.pos.x; gcd[24].gd.pos.y = gcd[22].gd.pos.y+24; 
    gcd[24].gd.flags = gg_enabled|gg_visible;
    gcd[24].creator = GLabelCreate;

    sprintf( macstyle, "%04x", tgetushort(tab,44) );
    label[25].text = (unichar_t *) macstyle;
    label[25].text_is_1byte = true;
    gcd[25].gd.label = &label[25];
    gcd[25].gd.pos.x = gcd[1].gd.pos.x; gcd[25].gd.pos.y = gcd[24].gd.pos.y-6;  gcd[25].gd.pos.width = gcd[1].gd.pos.width;
    gcd[25].gd.flags = gg_enabled|gg_visible;
    gcd[25].gd.cid = CID_MacStyle;
    gcd[25].gd.handle_controlevent = head_FlagsChange;
    gcd[25].creator = GTextFieldCreate;

    gcd[35].gd.pos.x = gcd[34].gd.pos.x;
    gcd[35].gd.pos.y = gcd[25].gd.pos.y;
    gcd[35].gd.pos.height = 58; gcd[35].gd.pos.width = gcd[34].gd.pos.width;
    gcd[35].gd.flags = gg_enabled|gg_visible | gg_list_multiplesel;
    gcd[35].gd.u.list = macstylelist;
    gcd[35].gd.cid = CID_MacStyleList;
    gcd[35].gd.handle_controlevent = head_FlagsChange;
    gcd[35].creator = GListCreate;

    label[26].text = (unichar_t *) _STR_SmallestSize;
    label[26].text_in_resource = true;
    gcd[26].gd.label = &label[26];
    gcd[26].gd.pos.x = 5; gcd[26].gd.pos.y = gcd[25].gd.pos.y+60+6;
    gcd[26].gd.flags = gg_enabled|gg_visible;
    gcd[26].creator = GLabelCreate;

    sprintf( smallest, "%d", tgetushort(tab,46) );
    label[27].text = (unichar_t *) smallest;
    label[27].text_is_1byte = true;
    gcd[27].gd.label = &label[27];
    gcd[27].gd.pos.x = gcd[1].gd.pos.x; gcd[27].gd.pos.y = gcd[26].gd.pos.y-6;  gcd[27].gd.pos.width = gcd[1].gd.pos.width;
    gcd[27].gd.flags = gg_enabled|gg_visible;
    gcd[27].gd.cid = CID_SmallestSize;
    gcd[27].creator = GTextFieldCreate;

    label[28].text = (unichar_t *) _STR_FontDirection;
    label[28].text_in_resource = true;
    gcd[28].gd.label = &label[28];
    gcd[28].gd.pos.x = gcd[6].gd.pos.x; gcd[28].gd.pos.y = gcd[26].gd.pos.y; 
    gcd[28].gd.flags = gg_enabled|gg_visible;
    gcd[28].creator = GLabelCreate;

    gcd[29].gd.pos.x = gcd[7].gd.pos.x; gcd[29].gd.pos.y = gcd[28].gd.pos.y-6;  gcd[29].gd.pos.width = gcd[1].gd.pos.width;
    gcd[29].gd.flags = gg_enabled|gg_visible;
    gcd[29].gd.cid = CID_FontDir;
    gcd[29].gd.u.list = directionlist;
    gcd[29].creator = GListButtonCreate;
    for ( i=0; directionlist[i].text!=NULL; ++i )
	directionlist[i].selected = (tgetushort(tab,48)==(int) (directionlist[i].userdata));

    label[30].text = (unichar_t *) _STR_LocaFormat;
    label[30].text_in_resource = true;
    gcd[30].gd.label = &label[30];
    gcd[30].gd.pos.x = gcd[0].gd.pos.x; gcd[30].gd.pos.y = gcd[28].gd.pos.y+24; 
    gcd[30].gd.flags = gg_enabled|gg_visible;
    gcd[30].creator = GLabelCreate;

    gcd[31].gd.pos.x = gcd[1].gd.pos.x; gcd[31].gd.pos.y = gcd[30].gd.pos.y-6;  gcd[31].gd.pos.width = gcd[1].gd.pos.width;
    gcd[31].gd.flags = gg_enabled|gg_visible;
    gcd[31].gd.cid = CID_LocaFormat;
    gcd[31].gd.u.list = localist;
    gcd[31].creator = GListButtonCreate;
    for ( i=0; localist[i].text!=NULL; ++i )
	localist[i].selected = false;
    localist[ tgetushort(tab,50) ].selected = true;

    label[32].text = (unichar_t *) _STR_GlyfFormat;
    label[32].text_in_resource = true;
    gcd[32].gd.label = &label[32];
    gcd[32].gd.pos.x = gcd[6].gd.pos.x; gcd[32].gd.pos.y = gcd[30].gd.pos.y;
    gcd[32].gd.flags = gg_enabled|gg_visible;
    gcd[32].creator = GLabelCreate;

    sprintf( glyf, "%d", tgetushort(tab,52) );
    label[33].text = (unichar_t *) glyf;
    label[33].text_is_1byte = true;
    gcd[33].gd.label = &label[33];
    gcd[33].gd.pos.x = gcd[7].gd.pos.x; gcd[33].gd.pos.y = gcd[32].gd.pos.y-6;  gcd[33].gd.pos.width = gcd[1].gd.pos.width;
    gcd[33].gd.flags = gg_enabled|gg_visible;
    gcd[33].gd.cid = CID_GlyphFormat;
    gcd[33].creator = GTextFieldCreate;

    /* 2 extra entries for lists */

    i = 36;
    gcd[i].gd.pos.x = 20-3; gcd[i].gd.pos.y = gcd[33].gd.pos.y+35-3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _STR_OK;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'O';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = Head_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 370-GIntGetResource(_NUM_Buttonsize)-20; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _STR_Cancel;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.handle_controlevent = Head_Cancel;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
    gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
    gcd[i].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[i].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    head_FlagsChange(gcd[9].ret,NULL);
    head_FlagsChange(gcd[25].ret,NULL);
    GDrawSetVisible(gw,true);
}
