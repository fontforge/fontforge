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
 * derbved from this software without specific prior written permission.

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

extern int _GScrollBar_Width;

#define UNIT_SPACER	2
#define ADDR_SPACER	4
#define HEX_SPACER	4
#define EDGE_SPACER	2

typedef struct binview /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
    unsigned int destroyed: 1;		/* window has been destroyed */
/* instrs specials */
    GGadget *mb, *vsb;
    int lpos, lheight;
    int16 as, fh;
    int16 vheight, vwidth;
    int16 mbh, sbw;
    GFont *gfont;
    int16 chrlen, addrend, hexend, bytes_per_line;
} BinView;

#define MID_Revert	2702
#define MID_Recent	2703
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_SelAll	2106

static int binary_processdata(TableView *tv) {
    /*BinView *bv = (BinView *) tv;*/
    /* Do changes!!! */
return( true );
}

static int binary_close(TableView *tv) {
    if ( binary_processdata(tv)) {
	tv->destroyed = true;
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs instrfuncs = { binary_close, binary_processdata };

static void binary_resize(BinView *bv,GEvent *event) {
    GRect pos;
    int lh, unitlen;

    /* height must be a multiple of the line height */
    /* width must also fit naturally */
    unitlen = 4*bv->chrlen+UNIT_SPACER +	/* four chars of hex with spacer */
	    2*bv->chrlen;			/* Plus two chars of ASCII */
    if ( (event->u.resize.size.height-bv->mbh-2*EDGE_SPACER)%bv->fh!=0 ||
	    (event->u.resize.size.height-bv->mbh-2*EDGE_SPACER-bv->fh)<0 ||
	    (event->u.resize.size.width-bv->sbw-bv->addrend-HEX_SPACER+UNIT_SPACER-unitlen-EDGE_SPACER)<0 ||
	    (event->u.resize.size.width-bv->sbw-bv->addrend-HEX_SPACER+UNIT_SPACER-EDGE_SPACER)%unitlen!=0 ) {
	int lc = (event->u.resize.size.height-bv->mbh+bv->fh/2-EDGE_SPACER)/bv->fh;
	int unitcnt = (event->u.resize.size.width-bv->addrend-HEX_SPACER-2*EDGE_SPACER+UNIT_SPACER)/unitlen;
	if ( lc<=0 ) lc = 1;
	if ( unitcnt<=0 ) unitcnt=1;
	GDrawResize(bv->gw, unitcnt*unitlen-UNIT_SPACER+bv->sbw+bv->addrend+HEX_SPACER+EDGE_SPACER,
		lc*bv->fh+bv->mbh+2*EDGE_SPACER);
return;
    }

    pos.width = GDrawPointsToPixels(bv->gw,_GScrollBar_Width);
    pos.height = event->u.resize.size.height-bv->mbh;
    pos.x = event->u.resize.size.width-pos.width; pos.y = bv->mbh;
    GGadgetResize(bv->vsb,pos.width,pos.height);
    GGadgetMove(bv->vsb,pos.x,pos.y);
    pos.width = pos.x; pos.x = 0;
    GDrawResize(bv->v,pos.width,pos.height);

    bv->bytes_per_line = 2*((pos.width-bv->addrend-HEX_SPACER-EDGE_SPACER)/unitlen);

    bv->vheight = pos.height; bv->vwidth = pos.width;
    bv->lheight = lh = (bv->table->newlen+bv->bytes_per_line-1)/bv->bytes_per_line;

    GScrollBarSetBounds(bv->vsb,0,lh,bv->vheight/bv->fh);
    if ( bv->lpos + bv->vheight/bv->fh > lh )
	bv->lpos = lh-bv->vheight/bv->fh;
    GScrollBarSetPos(bv->vsb,bv->lpos);
    GDrawRequestExpose(bv->v,NULL,false);
}

static void binary_expose(BinView *bv,GWindow pixmap,GRect *rect) {
    int low, high;
    int i,x,y;
    Table *table = bv->table;
    char chex[8], caddr[8]; unichar_t uhex[8], uaddr[8];
    unichar_t *ulist, ubuf[81];
    int hex_end;
    int addr, temp;
    uint8 *data = table->data;

    GDrawSetFont(pixmap,bv->gfont);

/* I tried displaying unicode rather than ASCII, but half the time the */
/*  unicode wasn't aligned properly and I just got junk */
    low = ( (rect->y-EDGE_SPACER)/bv->fh ) * bv->fh +2;
    high = ( (rect->y+rect->height+bv->fh-1-EDGE_SPACER)/bv->fh ) * bv->fh +EDGE_SPACER;
    if ( high>bv->vheight-EDGE_SPACER ) high = bv->vheight-EDGE_SPACER;

    GDrawDrawLine(pixmap,bv->addrend-ADDR_SPACER/2,rect->y,bv->addrend-ADDR_SPACER/2,rect->y+rect->height,0x000000);
    hex_end = bv->addrend + (bv->bytes_per_line/2)*(4*bv->chrlen+UNIT_SPACER)-UNIT_SPACER + HEX_SPACER;
    GDrawDrawLine(pixmap,hex_end-HEX_SPACER/2,rect->y,hex_end-HEX_SPACER/2,rect->y+rect->height,0x808080);

    if ( bv->bytes_per_line>=sizeof(ubuf)/sizeof(ubuf[0]) )
	ulist = galloc(bv->bytes_per_line*sizeof(unichar_t));
    else
	ulist = ubuf;

    addr = (bv->lpos+(low-EDGE_SPACER)/bv->fh)*bv->bytes_per_line;
    y = low;
    for ( ; y<=high && addr<table->newlen; addr += bv->bytes_per_line ) {
	sprintf( caddr, "%04x", addr );
	uc_strcpy(uaddr,caddr);
	x = bv->addrend - ADDR_SPACER - GDrawGetTextWidth(pixmap,uaddr,-1,NULL);
	GDrawDrawText(pixmap,x,y+bv->as,uaddr,-1,NULL,0x000000);

	for ( i=0; i<bv->bytes_per_line && addr+i<table->newlen; i+=2 ) {
	    temp = (data[addr+i]<<8) | data[addr+i+1];
	    ulist[i] = temp>>8; ulist[i+1] = temp&0xff;
	    sprintf( chex, "%04x", temp);
	    uc_strcpy(uhex,chex);
	    x = bv->addrend + (i/2)*(4*bv->chrlen+UNIT_SPACER);
	    GDrawDrawText(pixmap,x,y+bv->as,uhex,-1,NULL,0x000000);
	}
	for ( i=0; i<bv->bytes_per_line && addr+i<table->newlen; ++i )
	    if ( ulist[i]<' ' || (ulist[i]>=0x7f && ulist[i]<0xa0) )
		ulist[i] = '.';
	GDrawDrawText(pixmap,hex_end,y+bv->as,ulist,i,NULL,0x000000);
	y += bv->fh;
    }

    if ( ulist!=ubuf )
	free(ulist);
}

static void binary_mousemove(BinView *bv,int pos) {
    /*GGadgetPreparePopup(bv->gw,msg);*/
}

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	}
    }
}

static void binary_scroll(BinView *bv,struct sbevent *sb) {
    int newpos = bv->lpos;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= bv->vheight/bv->fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += bv->vheight/bv->fh;
      break;
      case et_sb_bottom:
        newpos = bv->lheight-bv->vheight/bv->fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>bv->lheight-bv->vheight/bv->fh )
        newpos = bv->lheight-bv->vheight/bv->fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=bv->lpos ) {
	int diff = newpos-bv->lpos;
	bv->lpos = newpos;
	GScrollBarSetPos(bv->vsb,bv->lpos);
	GDrawScroll(bv->v,NULL,0,diff*bv->fh);
    }
}

static void BinViewFree(BinView *bv) {
    bv->table->tv = NULL;
    free(bv);
}

static int bv_v_e_h(GWindow gw, GEvent *event) {
    BinView *bv = (BinView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	binary_expose(bv,gw,&event->u.expose.rect);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    TableHelp(bv->table->name);
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	GGadgetEndPopup();
	if ( event->type==et_mousemove )
	    binary_mousemove(bv,event->u.mouse.y);
      break;
      case et_timer:
      break;
      case et_focus:
      break;
    }
return( true );
}

static int bv_e_h(GWindow gw, GEvent *event) {
    BinView *bv = (BinView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
      break;
      case et_resize:
	binary_resize(bv,event);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    TableHelp(bv->table->name);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    binary_scroll(bv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	binary_close((TableView *) bv);
      break;
      case et_destroy:
	BinViewFree(bv);
      break;
    }
return( true );
}

static void IVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    BinView *bv = (BinView *) GDrawGetUserData(gw);

    DelayEvent((void (*)(void *)) binary_close, bv);
}

static void IVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((BinView *) GDrawGetUserData(gw))->owner;

    _TFVMenuSaveAs(tfv);
}

static void IVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((BinView *) GDrawGetUserData(gw))->owner;
    _TFVMenuSave(tfv);
}

static void IVMenuRevert(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((BinView *) GDrawGetUserData(gw))->owner;
    _TFVMenuRevert(tfv);
}

static GMenuItem dummyitem[] = { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, NULL };
static GMenuItem fllist[] = {
    { { (unichar_t *) _STR_Open, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) _STR_Close, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, IVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, IVMenuSave },
    { { (unichar_t *) _STR_SaveAs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, IVMenuSaveAs },
    { { (unichar_t *) _STR_Revertfile, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'R', ksm_control|ksm_shift, NULL, NULL, IVMenuRevert, MID_Revert },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
/*    { { (unichar_t *) _STR_Prefs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },*/
/*    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},*/
    { { (unichar_t *) _STR_Quit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'Q' }, 'Q', ksm_control, NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem edlist[] = {
    { { (unichar_t *) _STR_Undo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'U' }, 'Z', ksm_control, NULL, NULL },
    { { (unichar_t *) _STR_Redo, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'Y', ksm_control, NULL, NULL },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Cut, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, 'X', ksm_control, NULL, NULL, NULL, MID_Cut },
    { { (unichar_t *) _STR_Copy, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'C', ksm_control, NULL, NULL, NULL, MID_Copy },
    { { (unichar_t *) _STR_Paste, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'P' }, 'V', ksm_control, NULL, NULL, NULL, MID_Paste },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_SelectAll, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, 'A', ksm_control, NULL, NULL, NULL },
    { NULL }
};

extern GMenuItem helplist[];

static GMenuItem mblist[] = {
    { { (unichar_t *) _STR_File, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'F' }, 0, 0, fllist, fllistcheck },
    { { (unichar_t *) _STR_Edit, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'E' }, 0, 0, edlist },
    { { (unichar_t *) _STR_Window, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'W' }, 0, 0, NULL, WindowMenuBuild, NULL },
    { { (unichar_t *) _STR_Help, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'H' }, 0, 0, helplist, NULL },
    { NULL }
};


/* any table we don't understand... */
void binaryCreateEditor(Table *tab,TtfView *tfv) {
    BinView *bv = gcalloc(1,sizeof(BinView));
    unichar_t title[60];
    GRect pos, gsize;
    GWindow gw;
    GWindowAttrs wattrs;
    FontRequest rq;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','u','n','i','f','o','n','t', '\0' };
    int as,ds,ld, lh;
    GGadgetData gd;
    GGadget *sb;
    static unichar_t num[] = { '0',  '\0' };
    int numlen, unitlen;

    bv->table = tab;
    bv->virtuals = &instrfuncs;
    bv->owner = tfv;
    bv->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) bv;

    TableFillup(tab);

    title[0] = (tab->name>>24)&0xff;
    title[1] = (tab->name>>16)&0xff;
    title[2] = (tab->name>>8 )&0xff;
    title[3] = (tab->name    )&0xff;
    title[4] = ' ';
    u_strncpy(title+5, bv->font->fontname, sizeof(title)/sizeof(title[0])-6);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width = 300;
    pos.height = GDrawPointsToPixels(NULL,100);
    bv->gw = gw = GDrawCreateTopWindow(NULL,&pos,bv_e_h,bv,&wattrs);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    gd.u.menu = mblist;
    bv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(bv->mb,&gsize);
    bv->mbh = gsize.height;

    gd.pos.y = bv->mbh; gd.pos.height = pos.height-bv->mbh;
    gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    bv->vsb = sb = GScrollBarCreate(gw,&gd,bv);
    GGadgetGetSize(bv->vsb,&gsize);
    bv->sbw = gsize.width;

    wattrs.mask = wam_events|wam_cursor;
    pos.x = 0; pos.y = bv->mbh;
    pos.width = gd.pos.x; pos.height -= bv->mbh;
    bv->v = GWidgetCreateSubWindow(gw,&pos,bv_v_e_h,bv,&wattrs);
    GDrawSetVisible(bv->v,true);

    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = -12;
    rq.weight = 400;
    bv->gfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawSetFont(bv->v,bv->gfont);
    GDrawFontMetrics(bv->gfont,&as,&ds,&ld);
    bv->as = as+1;
    bv->fh = bv->as+ds;

    bv->chrlen = numlen = GDrawGetTextWidth(bv->v,num,1,NULL);
    bv->addrend = 6*numlen + ADDR_SPACER + EDGE_SPACER;
    unitlen = 3*numlen+UNIT_SPACER;		/* two chars of hex and one of unicode */
    bv->bytes_per_line = 2*((pos.width-bv->addrend-HEX_SPACER-2*EDGE_SPACER)/unitlen);
    if ( bv->bytes_per_line <= 1 ) bv->bytes_per_line = 1;

    lh = (bv->table->newlen+bv->bytes_per_line-1)/bv->bytes_per_line;
    if ( lh>40 ) lh = 40;
    if ( lh<4 ) lh = 4;
    GDrawResize(bv->gw,pos.width+gd.pos.width,bv->mbh+lh*bv->fh+2*EDGE_SPACER);

    GDrawSetVisible(gw,true);
}
