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
 * dersved from this software without specific prior written permission.

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

#define ADDR_SPACER	4
#define EDGE_SPACER	2

typedef struct shortview /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
/* instrs specials */
    GGadget *mb, *vsb;
    int16 lpos, lheight;
    int16 as, fh;
    int16 vheight, vwidth;
    int16 mbh, sbw;
    GFont *gfont;
    int16 chrlen, addrend, hexend;
} ShortView;

#define MID_Revert	2702
#define MID_Recent	2703
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_SelAll	2106

static int short_processdata(TableView *tv) {
    ShortView *sv = (ShortView *) tv;
    /* Do changes!!! */
return( true );
}

static int short_close(TableView *tv) {
    if ( short_processdata(tv)) {
	GDrawDestroyWindow(tv->gw);
return( true );
    }
return( false );
}

static struct tableviewfuncs instrfuncs = { short_close, short_processdata };

static void short_resize(ShortView *sv,GEvent *event) {
    GRect pos;
    int lh;

    /* height must be a multiple of the line height */
    if ( (event->u.resize.size.height-sv->mbh-2*EDGE_SPACER)%sv->fh!=0 ||
	    (event->u.resize.size.height-sv->mbh-2*EDGE_SPACER-sv->fh)<0 ) {
	int lc = (event->u.resize.size.height-sv->mbh+sv->fh/2-EDGE_SPACER)/sv->fh;
	if ( lc<=0 ) lc = 1;
	GDrawResize(sv->gw, event->u.resize.size.width,
		lc*sv->fh+sv->mbh+2*EDGE_SPACER);
return;
    }

    pos.width = GDrawPointsToPixels(sv->gw,_GScrollBar_Width);
    pos.height = event->u.resize.size.height-sv->mbh;
    pos.x = event->u.resize.size.width-pos.width; pos.y = sv->mbh;
    GGadgetResize(sv->vsb,pos.width,pos.height);
    GGadgetMove(sv->vsb,pos.x,pos.y);
    pos.width = pos.x; pos.x = 0;
    GDrawResize(sv->v,pos.width,pos.height);

    sv->vheight = pos.height; sv->vwidth = pos.width;
    sv->lheight = lh = sv->table->newlen/2;

    GScrollBarSetBounds(sv->vsb,0,lh,sv->vheight/sv->fh);
    if ( sv->lpos + sv->vheight/sv->fh > lh )
	sv->lpos = lh-sv->vheight/sv->fh;
    GScrollBarSetPos(sv->vsb,sv->lpos);
}

static void short_expose(ShortView *sv,GWindow pixmap,GRect *rect) {
    int low, high;
    int x,y;
    Table *table = sv->table;
    char cval[8], caddr[8]; unichar_t uval[8], uaddr[8];
    int index;

    GDrawSetFont(pixmap,sv->gfont);

    low = ( (rect->y-EDGE_SPACER)/sv->fh ) * sv->fh +2;
    high = ( (rect->y+rect->height+sv->fh-1-EDGE_SPACER)/sv->fh ) * sv->fh +EDGE_SPACER;
    if ( high>sv->vheight-EDGE_SPACER ) high = sv->vheight-EDGE_SPACER;

    GDrawDrawLine(pixmap,sv->addrend-ADDR_SPACER/2,rect->y,sv->addrend-ADDR_SPACER/2,rect->y+rect->height,0x000000);

    index = (sv->lpos+(low-EDGE_SPACER)/sv->fh);
    y = low;
    for ( ; y<=high && index<table->newlen/2; ++index ) {
	sprintf( caddr, "%d", index );
	uc_strcpy(uaddr,caddr);
	x = sv->addrend - ADDR_SPACER - GDrawGetTextWidth(pixmap,uaddr,-1,NULL);
	GDrawDrawText(pixmap,x,y+sv->as,uaddr,-1,NULL,0x000000);

	sprintf( cval, "%d", (short) tgetushort(table,2*index) );
	uc_strcpy(uval,cval);
	GDrawDrawText(pixmap,sv->addrend,y+sv->as,uval,-1,NULL,0x000000);
	y += sv->fh;
    }
}

static void short_mousemove(ShortView *sv,int pos) {
    /*GGadgetPreparePopup(sv->gw,msg);*/
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

static void short_scroll(ShortView *sv,struct sbevent *sb) {
    int newpos = sv->lpos;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= sv->vheight/sv->fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += sv->vheight/sv->fh;
      break;
      case et_sb_bottom:
        newpos = sv->lheight-sv->vheight/sv->fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>sv->lheight-sv->vheight/sv->fh )
        newpos = sv->lheight-sv->vheight/sv->fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=sv->lpos ) {
	int diff = newpos-sv->lpos;
	sv->lpos = newpos;
	GScrollBarSetPos(sv->vsb,sv->lpos);
	GDrawScroll(sv->v,NULL,0,diff*sv->fh);
    }
}

static void ShortViewFree(ShortView *sv) {
    sv->table->tv = NULL;
    free(sv);
}

static int sv_v_e_h(GWindow gw, GEvent *event) {
    ShortView *sv = (ShortView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	short_expose(sv,gw,&event->u.expose.rect);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    TableHelp(sv->table->name);
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	GGadgetEndPopup();
	if ( event->type==et_mousemove )
	    short_mousemove(sv,event->u.mouse.y);
      break;
      case et_timer:
      break;
      case et_focus:
      break;
    }
return( true );
}

static int sv_e_h(GWindow gw, GEvent *event) {
    ShortView *sv = (ShortView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
      break;
      case et_resize:
	short_resize(sv,event);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    TableHelp(sv->table->name);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    short_scroll(sv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	short_close((TableView *) sv);
      break;
      case et_destroy:
	ShortViewFree(sv);
      break;
    }
return( true );
}

static void IVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    ShortView *sv = (ShortView *) GDrawGetUserData(gw);

    DelayEvent((void (*)(void *)) short_close, sv);
}

static void IVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((ShortView *) GDrawGetUserData(gw))->owner;

    _TFVMenuSaveAs(tfv);
}

static void IVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((ShortView *) GDrawGetUserData(gw))->owner;
    _TFVMenuSave(tfv);
}

static void IVMenuRevert(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((ShortView *) GDrawGetUserData(gw))->owner;
    _TFVMenuRevert(tfv);
}

static GMenuItem dummyitem[] = { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'N' }, NULL };
static GMenuItem fllist[] = {
    { { (unichar_t *) _STR_Open, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'O' }, 'O', ksm_control, NULL, NULL, MenuOpen },
    { { (unichar_t *) _STR_Recent, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 't' }, '\0', ksm_control, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) _STR_Close, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'C' }, 'Q', ksm_control|ksm_shift, NULL, NULL, IVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Save, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'S' }, 'S', ksm_control, NULL, NULL, IVMenuSave },
    { { (unichar_t *) _STR_Saveas, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'a' }, 'S', ksm_control|ksm_shift, NULL, NULL, IVMenuSaveAs },
    { { (unichar_t *) _STR_Revertfile, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'R' }, 'R', ksm_control|ksm_shift, NULL, NULL, IVMenuRevert, MID_Revert },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) _STR_Prefs, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1, 0, 'e' }, '\0', ksm_control, NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
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
    { { (unichar_t *) _STR_Selectall, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 0, 1, 0, 'A' }, 'A', ksm_control, NULL, NULL, NULL },
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


/* cvt table */
void shortCreateEditor(Table *tab,TtfView *tfv) {
    ShortView *sv = gcalloc(1,sizeof(ShortView));
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
    int numlen;

    sv->table = tab;
    sv->virtuals = &instrfuncs;
    sv->owner = tfv;
    sv->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) sv;

    TableFillup(tab);

    title[0] = (tab->name>>24)&0xff;
    title[1] = (tab->name>>16)&0xff;
    title[2] = (tab->name>>8 )&0xff;
    title[3] = (tab->name    )&0xff;
    title[4] = ' ';
    u_strncpy(title+5, sv->font->fontname, sizeof(title)/sizeof(title[0])-6);

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,200);
    pos.height = GDrawPointsToPixels(NULL,100);
    sv->gw = gw = GDrawCreateTopWindow(NULL,&pos,sv_e_h,sv,&wattrs);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    gd.u.menu = mblist;
    sv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(sv->mb,&gsize);
    sv->mbh = gsize.height;

    gd.pos.y = sv->mbh; gd.pos.height = pos.height-sv->mbh;
    gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    sv->vsb = sb = GScrollBarCreate(gw,&gd,sv);
    GGadgetGetSize(sv->vsb,&gsize);
    sv->sbw = gsize.width;

    wattrs.mask = wam_events|wam_cursor;
    pos.x = 0; pos.y = sv->mbh;
    pos.width = gd.pos.x; pos.height -= sv->mbh;
    sv->v = GWidgetCreateSubWindow(gw,&pos,sv_v_e_h,sv,&wattrs);
    GDrawSetVisible(sv->v,true);

    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = -12;
    rq.weight = 400;
    sv->gfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawSetFont(sv->v,sv->gfont);
    GDrawFontMetrics(sv->gfont,&as,&ds,&ld);
    sv->as = as+1;
    sv->fh = sv->as+ds;

    sv->chrlen = numlen = GDrawGetTextWidth(sv->v,num,1,NULL);
    sv->addrend = 6*numlen + ADDR_SPACER + EDGE_SPACER;

    lh = sv->table->newlen/2;
    if ( lh>40 ) lh = 40;
    if ( lh<4 ) lh = 4;
    GDrawResize(sv->gw,sv->addrend+6*numlen+EDGE_SPACER+sv->sbw,sv->mbh+lh*sv->fh+2*EDGE_SPACER);

    GDrawSetVisible(gw,true);
}
