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
#include <locale.h>

extern struct lconv localeinfo;

#define CID_Version	1000
#define CID_Count	1001
#define CID_OK		1010
#define CID_Cancel	1011
#define CID_Group	1020

typedef struct gaspview /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
    unsigned int destroyed: 1;		/* window has been destroyed */
/* gasp specials */
    uint16 version, cnt;
    struct gasps { uint16 ppem, flags; } *gasps;
    GFont *gfont;
    int as, fh;
    int ybase, yend;
    int width;
    int fieldx, fieldxend;
    int gfx, gfxend;
    int aax, aaxend;
    GGadget *tf;
    int tfpos;
    unsigned int changed: 1;
} gaspView;

static int gvFinishUp(gaspView *gv) {
    const unichar_t *ret = _GGadgetGetTitle(gv->tf); unichar_t *end;
    int val = u_strtol(ret,&end,10);

    if ( gv->tfpos==-1 )
return( true );
    if ( *end!='\0' ) {
	GWidgetErrorR( _STR_BadInteger, _STR_BadInteger );
return( false );
    }
    gv->gasps[gv->tfpos].ppem = val;
    gv->tfpos = -1;
return( true );
}

static int gasp_processdata(TableView *tv) {
    uint8 *data;
    int len,i;
    gaspView *gv = (gaspView *) tv;

    if ( !gvFinishUp(gv))
return( false );

    free(tv->table->data);
    len = 4+4*gv->cnt;
    data = galloc(len);
    ptputushort(data,gv->version);
    ptputushort(data+2,gv->cnt);
    for ( i=0; i<gv->cnt; ++i ) {
	ptputushort(data+4+4*i,gv->gasps[i].ppem);
	ptputushort(data+4+4*i+2,gv->gasps[i].flags);
    }
    tv->table->data = data;
    tv->table->newlen = len;
    if ( !tv->table->changed ) {
	tv->table->changed = true;
	tv->table->container->changed = true;
	GDrawRequestExpose(tv->owner->v,NULL,false);
    }
return( true );
}

static int gasp_close(TableView *tv) {
    if ( gasp_processdata(tv)) {
	tv->destroyed = true;
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs gaspfuncs = { gasp_close, gasp_processdata };

static int gasp_CountChange(GGadget *g, GEvent *e) {
    GWindow gw;
    int val,i;
    gaspView *gv;
    struct gasps last;

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	const unichar_t *ret = _GGadgetGetTitle(g);
	val = u_strtol(ret,NULL,10);
	gw = GGadgetGetWindow(g);
	gv = GDrawGetUserData(gw);
	if ( gv->cnt != val && val>0 ) {
	    last = gv->gasps[gv->cnt-1];
	    gv->gasps = grealloc(gv->gasps,val*sizeof(struct gasps));
	    for ( i=gv->cnt-1 ; i<val; ++i )
		gv->gasps[i].ppem = gv->gasps[i].flags = 0;
	    gv->gasps[val-1] = last;
	    GDrawResize(gw,gv->width,gv->ybase+(val+1)*gv->fh+GDrawPointsToPixels(NULL,34));
	    gv->cnt = val;
	}
    }
return( true );
}

static int gasp_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	((TableView *) GDrawGetUserData(gw))->destroyed = true;
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int gasp_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    gaspView *gv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	gv = GDrawGetUserData(gw);
	gasp_close((TableView *) gv);
    }
return( true );
}

static void DrawCheckBox(GWindow pixmap,gaspView *gv,int x,int y,int set) {
    GRect rect;

    rect.x = x; rect.width = gv->as-1;
    rect.y = y+1; rect.height = rect.width;
    GDrawDrawRect(pixmap,&rect,0x000000);
    if ( set ) {
	GDrawDrawLine(pixmap,rect.x,rect.y,rect.x+rect.width, rect.y+rect.height, 0x000000 );
	GDrawDrawLine(pixmap,rect.x,rect.y+rect.height,rect.x+rect.width, rect.y, 0x000000 );
    }
}

static void gvExpose(GWindow pixmap, gaspView *gv,GRect *exp) {
    GRect sub, old;
    int i, y, x;
    unichar_t ubuf[60]; char buf[60];

    GDrawSetFont(pixmap,gv->gfont);
    sub.x = exp->x; sub.width = exp->width;
    sub.y = gv->ybase; sub.height = gv->yend-gv->ybase;
    GDrawPushClip(pixmap,&sub,&old);
    for ( i=0; i<gv->cnt; ++i ) {
	sprintf( buf, "Sizes less than %6d:", gv->gasps[i].ppem );
	uc_strcpy(ubuf,buf);
	y = gv->ybase+gv->as+i*gv->fh;
	GDrawDrawText(pixmap, 8,y,ubuf,-1,NULL,0x000000);
	x = 8+GDrawGetTextWidth(pixmap,ubuf,-1,NULL)+5;
	DrawCheckBox(pixmap,gv,x,y-gv->as,gv->gasps[i].flags&1);
	GDrawDrawText(pixmap, x+gv->fh,y,GStringGetResource(_STR_GridFit,NULL),-1,NULL,0x000000);
	x += gv->fh + GDrawGetTextWidth(pixmap,GStringGetResource(_STR_GridFit,NULL),-1,NULL)+5;
	DrawCheckBox(pixmap,gv,x,y-gv->as,gv->gasps[i].flags&2);
	GDrawDrawText(pixmap, x+gv->fh,y,GStringGetResource(_STR_AntiAlias,NULL),-1,NULL,0x000000);
    }
    GDrawPopClip(pixmap,&old);
}

static void gvMouseDown(gaspView *gv,GEvent *e) {
    int index = (e->u.mouse.y-gv->ybase)/gv->fh;
    int x, xtf;
    unichar_t ubuf[60]; char buf[60];

    if ( e->u.mouse.y<gv->ybase || index>=gv->cnt )
return;

    GDrawSetFont(gv->gw,gv->gfont);
    uc_strcpy(ubuf, "Sizes less than ");
    xtf = 8+GDrawGetTextWidth(gv->gw,ubuf,-1,NULL);
    if ( e->u.mouse.x<xtf )
return;
    sprintf( buf, "Sizes less than %6d:", gv->gasps[index].ppem );
    uc_strcpy(ubuf,buf);
    x = 8+GDrawGetTextWidth(gv->gw,ubuf,22,NULL);
    if ( e->u.mouse.x<x ) {
	if ( index==gv->cnt-1 ) {	/* Can't change last one, must be 0xffff */
	    GDrawBeep(screen_display);
	} else if ( index!=gv->tfpos ) {
	    if ( gvFinishUp(gv)) {
		GGadgetMove(gv->tf,xtf,gv->ybase+index*gv->fh);
		sprintf( buf, "%6d", gv->gasps[index].ppem );
		uc_strcpy(ubuf,buf);
		GGadgetResize(gv->tf,GDrawGetTextWidth(gv->gw,ubuf,6,NULL),gv->fh);
		sprintf( buf, "%d", gv->gasps[index].ppem );
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(gv->tf,ubuf);
		GGadgetSetVisible(gv->tf,true);
		gv->tfpos = index;
		GDrawRequestExpose(gv->gw,NULL,false);
	    }
	}
return;
    }
    x = 8+GDrawGetTextWidth(gv->gw,ubuf,-1,NULL)+5;
    if ( e->u.mouse.x>=x && e->u.mouse.x<x+gv->fh ) {
	gv->gasps[index].flags ^= 1;
	gv->changed = true;
	GDrawRequestExpose(gv->gw,NULL,false);
return;
    }
    x += gv->fh + GDrawGetTextWidth(gv->gw,GStringGetResource(_STR_GridFit,NULL),-1,NULL)+5;
    if ( e->u.mouse.x>=x && e->u.mouse.x<x+gv->fh ) {
	gv->gasps[index].flags ^= 2;
	gv->changed = true;
	GDrawRequestExpose(gv->gw,NULL,false);
return;
    }
}

static void gvResize(gaspView *gv,GEvent *event) {
    GRect pos;
    int yend;

    yend = event->u.resize.size.height - GDrawPointsToPixels(NULL,34);
    GGadgetGetSize(GWidgetGetControl(gv->gw,CID_OK),&pos);
    GGadgetMove(GWidgetGetControl(gv->gw,CID_OK),pos.x,pos.y+yend-gv->yend);
    GGadgetGetSize(GWidgetGetControl(gv->gw,CID_Cancel),&pos);
    GGadgetMove(GWidgetGetControl(gv->gw,CID_Cancel),pos.x,pos.y+yend-gv->yend);
    GGadgetResize(GWidgetGetControl(gv->gw,CID_Group),event->u.resize.size.width-4,
	    event->u.resize.size.height-4);
    gv->yend = yend;
    gv->width = event->u.resize.size.width;
    GDrawRequestExpose(gv->gw,NULL,false);
}

static int gasp_e_h(GWindow gw, GEvent *event) {
    gaspView *gv = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	gv->destroyed = true;
	GDrawDestroyWindow(gv->gw);
    } else if ( event->type == et_destroy ) {
	gv->table->tv = NULL;
	free(gv->gasps);
	free(gv);
    } else if ( event->type == et_char ) {
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 ) {
	    TableHelp(gv->table->name);
	    system("netscape http://partners.adobe.com/asn/developer/opentype/gasp.html &");
return( true );
	} else if (( event->u.chr.state&ksm_control ) &&
		(event->u.chr.keysym=='q' || event->u.chr.keysym=='Q')) {
	    MenuExit(NULL,NULL,NULL);
	}
return( false );
    } else if ( event->type == et_mousedown ) {
	gvMouseDown(gv,event);
    } else if ( event->type == et_resize ) {
	gvResize(gv,event);
    } else if ( event->type == et_expose ) {
	gvExpose(gw,gv,&event->u.expose.rect);
    }
return( true );
}

static void gaspTableFillup(gaspView *gv) {
    int i;

    TableFillup(gv->table);
    gv->version = ptgetushort(gv->table->data);
    gv->cnt = ptgetushort(gv->table->data+2);
    gv->gasps = galloc(gv->cnt*sizeof(struct gasps));
    for ( i=0; i<gv->cnt; ++i ) {
	gv->gasps[i].ppem = ptgetushort(gv->table->data+4+4*i);
	gv->gasps[i].flags = ptgetushort(gv->table->data+4+4*i+2);
    }
}

void gaspCreateEditor(Table *tab,TtfView *tfv) {
    gaspView *gv = gcalloc(1,sizeof(gaspView));
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[12];
    GTextInfo label[12];
    int i;
    static unichar_t title[60];
    char version[20], cnt[8];
    FontRequest rq;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','u','n','i','f','o','n','t', '\0' };
    int as,ds,ld;
    static GBox tfbox;
    static unichar_t num[] = { '0',  '\0' };

    gv->table = tab;
    gv->virtuals = &gaspfuncs;
    gv->owner = tfv;
    gv->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) gv;

    gaspTableFillup(gv);
    
    title[0] = (tab->name>>24)&0xff;
    title[1] = (tab->name>>16)&0xff;
    title[2] = (tab->name>>8 )&0xff;
    title[3] = (tab->name    )&0xff;
    title[4] = ' ';
    u_strncpy(title+5, gv->font->fontname, sizeof(title)/sizeof(title[0])-6);
    title[sizeof(title)/sizeof(title[0])-1] = '\0';

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,270);
    pos.height = GDrawPointsToPixels(NULL,267);
    gv->gw = gw = GDrawCreateTopWindow(NULL,&pos,gasp_e_h,gv,&wattrs);

    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = -12;
    rq.weight = 400;
    gv->gfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawSetFont(gv->gw,gv->gfont);
    GDrawFontMetrics(gv->gfont,&as,&ds,&ld);
    gv->as = as+1;
    gv->fh = gv->as+ds;

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_Version;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5+6; 
    gcd[0].gd.flags = gg_enabled|gg_visible;
    gcd[0].creator = GLabelCreate;

    sprintf( version, "%d", gv->version );
    label[1].text = (unichar_t *) version;
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 55; gcd[1].gd.pos.y = gcd[0].gd.pos.y-6;  gcd[1].gd.pos.width = 50;
    gcd[1].gd.flags = /*gg_enabled|*/gg_visible;
    gcd[1].gd.cid = CID_Version;
    /*gcd[1].gd.handle_controlevent = gasp_VersionChange;*/
    gcd[1].creator = GTextFieldCreate;

    label[2].text = (unichar_t *) _STR_Count;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 115; gcd[2].gd.pos.y = gcd[0].gd.pos.y; 
    gcd[2].gd.flags = gg_enabled|gg_visible;
    gcd[2].creator = GLabelCreate;

    sprintf( cnt, "%d", gv->cnt );
    label[3].text = (unichar_t *) cnt;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 155; gcd[3].gd.pos.y = gcd[2].gd.pos.y-6;  gcd[3].gd.pos.width = 50;
    gcd[3].gd.flags = gg_enabled|gg_visible;
    gcd[3].gd.cid = CID_Count;
    gcd[3].gd.handle_controlevent = gasp_CountChange;
    gcd[3].creator = GTextFieldCreate;

    gv->ybase = GDrawPointsToPixels(NULL,gcd[3].gd.pos.y+24+8);
    gv->yend = gv->ybase+(gv->cnt+1)*gv->fh;
    gv->width = pos.width;
    i = 4;

    gcd[i].gd.pos.x = 20-3; gcd[i].gd.pos.y = GDrawPixelsToPoints(NULL,gv->yend);
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[i].text = (unichar_t *) _STR_OK;
    label[i].text_in_resource = true;
    gcd[i].gd.mnemonic = 'O';
    gcd[i].gd.label = &label[i];
    gcd[i].gd.handle_controlevent = gasp_OK;
    gcd[i].gd.cid = CID_OK;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 265-GIntGetResource(_NUM_Buttonsize)-20; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
    gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
    gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[i].text = (unichar_t *) _STR_Cancel;
    label[i].text_in_resource = true;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.mnemonic = 'C';
    gcd[i].gd.handle_controlevent = gasp_Cancel;
    gcd[i].gd.cid = CID_Cancel;
    gcd[i++].creator = GButtonCreate;

    gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
    gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
    gcd[i].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[i].gd.cid = CID_Group;
    gcd[i++].creator = GGroupCreate;

    tfbox.main_background = tfbox.main_foreground = COLOR_DEFAULT;
    label[i].text = num+1;
    label[i].font = gv->gfont;
    gcd[i].gd.label = &label[i];
    gcd[i].gd.box = &tfbox;
    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = 10;
    gcd[i].gd.pos.width = 30; gcd[i].gd.pos.height = gv->fh;
    gcd[i].gd.flags = gg_enabled | gg_pos_in_pixels|gg_dontcopybox;
    gcd[i].creator = GTextFieldCreate;

    GGadgetsCreate(gw,gcd);
    gv->tf = gcd[i].ret;
    gv->tfpos = -1;
    GDrawResize(gw,pos.width,gv->yend+GDrawPointsToPixels(NULL,34));
    GDrawSetVisible(gw,true);
}
