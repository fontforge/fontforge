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
 * dermved from this software without specific prior written permission.

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
#define UNIT_SPACER	2

typedef struct metricmview /* : tableview */ {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
    unsigned int destroyed: 1;		/* window has been destroyed */
/* instrs specials */
    GGadget *mb, *vsb, *tf;
    int lpos, lheight;
    int16 as, fh;
    int16 vheight, vwidth;
    int16 mbh, sbw;
    GFont *gfont, *ifont;
    int chrlen, addrend, widthend;
    int longmetricscnt, oldlongmetricscnt;
    int lastwidth;
    int active, islb;
    unsigned int changed: 1;
    uint8 *edits;
    int elen;
} MetricsView;

#define MID_Revert	2702
#define MID_Recent	2703
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_SelAll	2106

static void GrowLongMetrics(MetricsView *mv,int newlongcnt) {
    TtfFont *font = mv->font;
    Table *table = mv->table;
    int i, tag, newlen, gc, oldlen;
    uint8 *data, *olddata;

    if ( (table->name>>24)=='v' )
	tag = CHR('v','m','t','x');
    else
	tag = CHR('h','m','t','x');
    for ( i=0; i<font->tbl_cnt; ++i )
	if ( font->tbls[i]->name==tag )
    break;
    _heaChangeLongMetrics(font->tbls[i],newlongcnt);

    oldlen = mv->edits ? mv->elen : table->newlen;
    olddata = mv->edits!=NULL?mv->edits:table->data;
    gc = (oldlen-4*mv->longmetricscnt)/2 + mv->longmetricscnt;
    newlen = 4*newlongcnt + 2*(gc-newlongcnt);
    data = galloc(newlen);
    memcpy(data,olddata,4*mv->longmetricscnt);
    for ( i=0; i<newlongcnt; ++i ) {
	ptputushort(data+4*mv->longmetricscnt+4*i,mv->lastwidth);
	ptputushort(data+4*mv->longmetricscnt+4*i+2,
		ptgetushort(olddata+4*mv->longmetricscnt+2*i));
    }
    for ( i=0; i<gc-newlongcnt; ++i ) {
	ptputushort(data+4*newlongcnt+2*i,
		ptgetushort(olddata+4*mv->longmetricscnt+2*(i+newlongcnt-mv->longmetricscnt)));
    }
    free(mv->edits);
    mv->edits = data;
    mv->elen = newlen;
    mv->longmetricscnt = newlongcnt;
}

static int mfinishup(MetricsView *mv,int showerr) {
    const unichar_t *ret = _GGadgetGetTitle(mv->tf);
    unichar_t *end;
    int val, oldval;
    Table *table = mv->table;
    static int buts[] = { _STR_Yes, _STR_Cancel, 0 };
    uint8 *data = mv->edits ? mv->edits : table->data;

    if ( mv->active==-1 )
return( true );

    val = u_strtol(ret,&end,10);
    if ( *ret=='\0' || *end!='\0' || val<-32768 || val>32767 ) {
	if ( showerr )
	    GWidgetErrorR(_STR_BadNumber,_STR_BadNumber);
return( false );
    }
    if ( mv->islb ) {
	if ( mv->active<mv->longmetricscnt )
	    oldval = (short) ptgetushort(data+4*mv->active+2);
	else
	    oldval = (short) ptgetushort(data+4*mv->longmetricscnt+2*(mv->active-mv->longmetricscnt));
    } else {
	if ( mv->active<mv->longmetricscnt )
	    oldval = (short) ptgetushort(data+4*mv->active);
	else
	    oldval = mv->lastwidth;
    }
    if ( val != oldval ) {
	if ( !mv->islb && mv->active>=mv->longmetricscnt ) {
	    if ( GWidgetAskR(_STR_GrowLongMetrics,buts,0,1,_STR_GrowLongMetricsSize)==1 ) {
		mv->active = -1;
		GGadgetMove(mv->tf,mv->addrend,-100);
		GDrawRequestExpose(mv->v,NULL,false);
return( -1 );
	    }
	    GrowLongMetrics(mv,mv->active);
	}
	if ( mv->edits==NULL ) {
	    mv->elen = table->len;
	    mv->edits = galloc(mv->elen);
	    memcpy(mv->edits,table->data,mv->elen);
	}
	if ( !mv->islb )
	    ptputushort(mv->edits+4*mv->active,val);
	else if ( mv->active<mv->longmetricscnt )
	    ptputushort(mv->edits+4*mv->active+2,val);
	else
	    ptputushort(mv->edits+4*mv->longmetricscnt+2*(mv->active-mv->longmetricscnt),val);
	mv->changed = true;
    }
    mv->active = -1;
    GGadgetMove(mv->tf,mv->addrend,-100);
return( true );
}

static int metrics_processdata(TableView *tv) {
    MetricsView *mv = (MetricsView *) tv;
    int ret = mfinishup(mv,true);

    if ( ret>0 && !tv->table->changed ) {
	free(tv->table->data);
	tv->table->data = mv->edits;
	tv->table->newlen = mv->elen;
	tv->table->changed = true;
	tv->table->container->changed = true;
	GDrawRequestExpose(tv->owner->v,NULL,false);
    }
return( ret>0 );
}

static int metrics_close(TableView *tv) {
    static int yesnocancel[] = { _STR_Yes, _STR_No, _STR_Cancel, 0 };
    static int closecancel[] = { _STR_Close, _STR_Cancel, 0 };
    MetricsView *mv = (MetricsView *) tv;
    int ret = mfinishup((MetricsView *) mv,false);
    int copyedits = true;

    if ( ret>0 ) {
	if ( mv->changed ) {
	    ret = GWidgetAskR(_STR_RetainChanges,yesnocancel,0,2,_STR_RetainChanges);
	    if ( ret==2 )
return( false );
	    if ( ret==1 )
		copyedits = false;
	}
    } else if ( ret<0 )
return( false );
    else {
	ret = GWidgetAskR(_STR_BadNumber,closecancel,0,1,_STR_BadNumberCloseAnyway);
	if ( ret==1 )
return( false );
    }
    if ( copyedits && mv->edits ) {
	free(mv->table->data);
	mv->table->data = mv->edits;
	mv->table->newlen = mv->elen;
	if ( !mv->table->changed ) {
	    mv->table->changed = true;
	    mv->table->td_changed = true;
	    mv->table->container->changed = true;
	    GDrawRequestExpose(mv->owner->v,NULL,false);
	}
    }
    mv->destroyed = true;
    GDrawDestroyWindow(mv->gw);
return( true );
}

static struct tableviewfuncs instrfuncs = { metrics_close, metrics_processdata };

static void metrics_resize(MetricsView *mv,GEvent *event) {
    GRect pos;
    int top = mv->mbh + mv->fh + EDGE_SPACER;

    /* height must be a multiple of the line height */
    if ( (event->u.resize.size.height-top-2*EDGE_SPACER)%mv->fh!=0 ||
	    (event->u.resize.size.height-top-2*EDGE_SPACER-mv->fh)<0 ) {
	int lc = (event->u.resize.size.height-top+mv->fh/2-2*EDGE_SPACER)/mv->fh;
	if ( lc<=0 ) lc = 1;
	GDrawResize(mv->gw, event->u.resize.size.width,
		lc*mv->fh+top+2*EDGE_SPACER);
return;
    }

    pos.width = GDrawPointsToPixels(mv->gw,_GScrollBar_Width);
    pos.height = event->u.resize.size.height-top;
    pos.x = event->u.resize.size.width-pos.width; pos.y = top;
    GGadgetResize(mv->vsb,pos.width,pos.height);
    GGadgetMove(mv->vsb,pos.x,pos.y);
    pos.width = pos.x; pos.x = 0;
    GDrawResize(mv->v,pos.width,pos.height);

    mv->vheight = pos.height; mv->vwidth = pos.width;
    mv->lheight = mv->longmetricscnt + (mv->table->newlen-mv->longmetricscnt*4)/2;

    GScrollBarSetBounds(mv->vsb,0,mv->lheight,mv->vheight/mv->fh);
    if ( mv->lpos + mv->vheight/mv->fh > mv->lheight ) {
	mv->lpos = mv->lheight-mv->vheight/mv->fh;
	if ( mv->lpos<0 ) mv->lpos = 0;
	if ( mv->active!=-1 ) {
	    GRect tp;
	    GGadgetGetSize(mv->tf,&tp);
	    GGadgetMove(mv->tf,tp.x,(mv->active-mv->lpos)*mv->fh);
	}
    }
    GScrollBarSetPos(mv->vsb,mv->lpos);

    if ( mv->islb ) {
	/* The tf's width isn't going to change if !mv->islb */
	int width = pos.width-mv->addrend;
	if ( width < 5 ) width = 5;
	GGadgetResize(mv->tf,width,mv->fh);
    }
    GDrawRequestExpose(mv->gw,NULL,false);
    GDrawRequestExpose(mv->v,NULL,false);
}

static void metrics_expose(MetricsView *mv,GWindow pixmap,GRect *rect) {
    int low, high;
    int x,y;
    Table *table = mv->table;
    char cval[8], caddr[8]; unichar_t uval[8], uaddr[12];
    int index;
    int col;
    uint8 *data = mv->edits ? mv->edits : table->data;

    GDrawSetFont(pixmap,mv->gfont);

    low = ( (rect->y-EDGE_SPACER)/mv->fh ) * mv->fh +EDGE_SPACER;
    high = ( (rect->y+rect->height+mv->fh-1-EDGE_SPACER)/mv->fh ) * mv->fh +EDGE_SPACER;
    if ( high>mv->vheight-EDGE_SPACER ) high = mv->vheight-EDGE_SPACER;

    GDrawDrawLine(pixmap,mv->addrend-ADDR_SPACER/2,rect->y,mv->addrend-ADDR_SPACER/2,rect->y+rect->height,0x000000);

    index = (mv->lpos+(low-EDGE_SPACER)/mv->fh);
    y = low;
    for ( ; y<=high && index<mv->lheight; ++index ) {
	sprintf( caddr, "%d", index );
	uc_strcpy(uaddr,caddr);
	col = 0x000000;
	if ( mv->font->enc!=NULL ) {
	    int p = strlen(caddr);
	    uaddr[p] = ' ';
	    if ( mv->font->enc->uenc[index]!=0xffff )
		uaddr[p+1] = mv->font->enc->uenc[index];
	    else {
		uaddr[p+1] = '?';
		col = 0xff0000;
	    }
	    uaddr[p+2] = '\0';
	}
	x = mv->addrend - ADDR_SPACER - GDrawGetTextWidth(pixmap,uaddr,-1,NULL);
	GDrawDrawText(pixmap,x,y+mv->as,uaddr,-1,NULL,col);

	if ( index<mv->longmetricscnt ) {
	    sprintf( cval, "%d", (short) ptgetushort(data+4*index) );
	    uc_strcpy(uval,cval);
	    GDrawDrawText(pixmap,mv->addrend,y+mv->as,uval,-1,NULL,0x000000);
	    sprintf( cval, "%d", (short) ptgetushort(data+4*index+2) );
	    uc_strcpy(uval,cval);
	    GDrawDrawText(pixmap,mv->widthend,y+mv->as,uval,-1,NULL,0x000000);
	} else {
	    GDrawSetFont(pixmap,mv->ifont);
	    sprintf( cval, "%d", mv->lastwidth );
	    uc_strcpy(uval,cval);
	    GDrawDrawText(pixmap,mv->addrend,y+mv->as,uval,-1,NULL,0x000000);
	    GDrawSetFont(pixmap,mv->gfont);
	    sprintf( cval, "%d", (short) ptgetushort(data+
		    4*mv->longmetricscnt+2*(index-mv->longmetricscnt)) );
	    uc_strcpy(uval,cval);
	    GDrawDrawText(pixmap,mv->widthend,y+mv->as,uval,-1,NULL,0x000000);
	}
	y += mv->fh;
    }
}

static void metrics_mousemove(MetricsView *mv,int pos) {
    int index = (mv->lpos+(pos-EDGE_SPACER)/mv->fh);
    char buffer[20];
    static unichar_t ubuf[60];

    if ( mv->font->enc!=NULL && index>=0 && index<mv->font->enc->cnt ) {
	int val = mv->font->enc->uenc[index];
	if ( index==0 )
	    uc_strcpy(ubuf,".notdef");
	if ( val==0xffff ) {
	    if ( index==0 )
		uc_strcpy(ubuf,".notdef");
	    else if ( index==1 )
		uc_strcpy(ubuf,".null");
	    else if ( index==2 )
		uc_strcpy(ubuf,"nonmarkingreturn");
	    else
		uc_strcpy(ubuf,"???");	/* No idea */
	} else if ( psunicodenames[val]!=NULL ) {
	    uc_strcpy(ubuf,psunicodenames[val]);
	} else {
	    sprintf( buffer, "uni%04X", val );
	    uc_strcpy(ubuf,buffer);
	}
	GGadgetPreparePopup(mv->gw,ubuf);
    }
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

static void metrics_scroll(MetricsView *mv,struct sbevent *sb) {
    int newpos = mv->lpos;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= mv->vheight/mv->fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += mv->vheight/mv->fh;
      break;
      case et_sb_bottom:
        newpos = mv->lheight-mv->vheight/mv->fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>mv->lheight-mv->vheight/mv->fh )
        newpos = mv->lheight-mv->vheight/mv->fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=mv->lpos ) {
	int diff = newpos-mv->lpos;
	mv->lpos = newpos;
	GScrollBarSetPos(mv->vsb,mv->lpos);
	if ( mv->active!=-1 ) {
	    GRect pos;
	    GGadgetGetSize(mv->tf,&pos);
	    GGadgetMove(mv->tf,pos.x,pos.y+diff*mv->fh);
	}
	GDrawScroll(mv->v,NULL,0,diff*mv->fh);
    }
}

static void MetricsViewFree(MetricsView *mv) {
    mv->table->tv = NULL;
    free(mv);
}

static int mv_v_e_h(GWindow gw, GEvent *event) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int val;

    switch ( event->type ) {
      case et_expose:
	metrics_expose(mv,gw,&event->u.expose.rect);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    TableHelp(mv->table->name);
      break;
      case et_mousemove: case et_mousedown: case et_mouseup:
	GGadgetEndPopup();
	if ( event->type==et_mousemove )
	    metrics_mousemove(mv,event->u.mouse.y);
	else if ( event->type == et_mousedown ) {
	    int l = (event->u.mouse.y-EDGE_SPACER)/mv->fh + mv->lpos;
	    unichar_t ubuf[20]; char buf[20];
	    uint8 *data = mv->edits ? mv->edits : mv->table->data;
	    if ( mfinishup(mv,true)>0 && event->u.mouse.x>mv->addrend &&
		    l<mv->font->glyph_cnt && (l!=mv->active ||
		     (mv->islb && event->u.mouse.x<mv->widthend) ||
		     (!mv->islb && event->u.mouse.x>=mv->widthend) )) {
		mv->active = l;
		mv->islb = event->u.mouse.x>=mv->widthend;
		if ( mv->islb ) {
		    GGadgetMove(mv->tf, mv->widthend,
				    (l-mv->lpos)*mv->fh+EDGE_SPACER+1);
		    GGadgetResize(mv->tf, mv->vwidth-mv->widthend,mv->fh);
		    if ( l<mv->longmetricscnt )
			val = (short) ptgetushort(data+4*l+2);
		    else
			val = (short) ptgetushort(data+4*mv->longmetricscnt+2*(l-mv->longmetricscnt));
		} else {
		    GGadgetMove(mv->tf, mv->addrend,
				    (l-mv->lpos)*mv->fh+EDGE_SPACER+1);
		    GGadgetResize(mv->tf, mv->widthend-mv->addrend-UNIT_SPACER,mv->fh);
		    if ( l<mv->longmetricscnt )
			val = (short) ptgetushort(data+4*l);
		    else
			val = mv->lastwidth;
		}
		sprintf( buf, "%d", val );
		uc_strcpy(ubuf,buf);
		GGadgetSetTitle(mv->tf,ubuf);
		GDrawPostEvent(event);	/* And we hope the tf catches it this time */
	    }
	}
      break;
      case et_timer:
      break;
      case et_focus:
      break;
    }
return( true );
}

static int mv_e_h(GWindow gw, GEvent *event) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    static unichar_t addr[] = { 'g','l','y','p','h',  '\0' },
		     width[] = { 'w','i','d','t','h',  '\0' },
		     bearing[] = { 'b','e','a','r','i','n','g',  '\0' };
    int x;

    switch ( event->type ) {
      case et_expose:
	GDrawSetFont(gw,mv->gfont);
	x = mv->addrend - ADDR_SPACER - GDrawGetTextWidth(gw,addr,-1,NULL);
	GDrawDrawText(gw,x,mv->mbh+mv->as,addr,-1,NULL,0x000000);
	GDrawDrawText(gw,mv->addrend,mv->mbh+mv->as,width,-1,NULL,0x000000);
	GDrawDrawText(gw,mv->widthend,mv->mbh+mv->as,bearing,-1,NULL,0x000000);
      break;
      case et_resize:
	metrics_resize(mv,event);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_Help || event->u.chr.keysym == GK_F1 )
	    TableHelp(mv->table->name);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    metrics_scroll(mv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	metrics_close((TableView *) mv);
      break;
      case et_destroy:
	MetricsViewFree(mv);
      break;
    }
return( true );
}

static void IVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    DelayEvent((void (*)(void *)) metrics_close, mv);
}

static void IVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((MetricsView *) GDrawGetUserData(gw))->owner;

    _TFVMenuSaveAs(tfv);
}

static void IVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((MetricsView *) GDrawGetUserData(gw))->owner;
    _TFVMenuSave(tfv);
}

static void IVMenuRevert(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    TtfView *tfv = ((MetricsView *) GDrawGetUserData(gw))->owner;
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

static int GetMetricsCnt(TtfFont *tf,Table *tab) {
    int32 name;
    Table *header;
    int i;

    if ( tab->name==CHR('h','m','t','x'))
	name = CHR('h','h','e','a');
    else
	name = CHR('v','h','e','a');

    for ( i=0; i<tf->tbl_cnt; ++i )
	if ( tf->tbls[i]->name==name )
    break;

    if ( i==tf->tbl_cnt )		/* Should we create a dummy vmtx table*/
return( tf->glyph_cnt );		/* (there better be a hmtx) with glyph_cnt in it? */

    header = tf->tbls[i];
    TableFillup(header);

    if ( header->newlen<36 )		/* Can't be */
return( tf->glyph_cnt );

return( tgetushort(header,34));		/* location of the numMetrics field */
}

/* hmtx, vmtx */
void metricsCreateEditor(Table *tab,TtfView *tfv) {
    MetricsView *mv = gcalloc(1,sizeof(MetricsView));
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
    static GBox tfbox;
    GTextInfo lab;

    mv->table = tab;
    mv->virtuals = &instrfuncs;
    mv->owner = tfv;
    mv->font = tfv->ttf->fonts[tfv->selectedfont];
    tab->tv = (TableView *) mv;

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
    wattrs.window_title = title;
    wattrs.icon = ttf_icon;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,200);
    pos.height = GDrawPointsToPixels(NULL,100);
    mv->gw = gw = GDrawCreateTopWindow(NULL,&pos,mv_e_h,mv,&wattrs);

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    gd.u.menu = mblist;
    mv->mb = GMenuBarCreate( gw, &gd, NULL);
    GGadgetGetSize(mv->mb,&gsize);
    mv->mbh = gsize.height;

    gd.pos.y = mv->mbh; gd.pos.height = pos.height-mv->mbh;
    gd.pos.width = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    mv->vsb = sb = GScrollBarCreate(gw,&gd,mv);
    GGadgetGetSize(mv->vsb,&gsize);
    mv->sbw = gsize.width;

    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = -12;
    rq.weight = 400;
    mv->gfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawFontMetrics(mv->gfont,&as,&ds,&ld);
    mv->as = as+1;
    mv->fh = mv->as+ds;
    rq.style = fs_italic;
    mv->ifont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);

    wattrs.mask = wam_events|wam_cursor;
    pos.x = 0; pos.y = mv->mbh + mv->fh + EDGE_SPACER;
    pos.width = gd.pos.x; pos.height -= mv->mbh;
    mv->v = GWidgetCreateSubWindow(gw,&pos,mv_v_e_h,mv,&wattrs);
    GDrawSetVisible(mv->v,true);
    GDrawSetFont(mv->v,mv->gfont);

    mv->longmetricscnt = GetMetricsCnt(mv->font,tab);
    mv->oldlongmetricscnt = mv->longmetricscnt;
    mv->lastwidth = tgetushort(tab,(mv->longmetricscnt-1)*4);

    mv->chrlen = numlen = GDrawGetTextWidth(mv->v,num,1,NULL);
    mv->addrend = 8*numlen + ADDR_SPACER + EDGE_SPACER;
    mv->widthend = mv->addrend + 6*mv->chrlen + UNIT_SPACER;

    tfbox.main_background = tfbox.main_foreground = COLOR_DEFAULT;
    gd.pos.y = -100; gd.pos.height = mv->fh;
    gd.pos.x = mv->addrend;
    memset(&lab,'\0',sizeof(lab));
    lab.text = num+1;
    lab.font = mv->gfont;
    gd.label = &lab;
    gd.box = &tfbox;
    gd.flags = gg_visible|gg_enabled|gg_sb_vert|gg_dontcopybox;
    mv->tf = GTextFieldCreate(mv->v,&gd,NULL);
    mv->active = -1; mv->islb=0;

    lh = mv->lheight = mv->longmetricscnt + (tab->newlen-mv->longmetricscnt*4)/2;
    if ( lh>39 ) lh = 39;
    if ( lh<4 ) lh = 4;
    GDrawResize(mv->gw,mv->addrend+12*numlen+UNIT_SPACER+EDGE_SPACER+mv->sbw,
	mv->mbh+mv->fh+EDGE_SPACER+lh*mv->fh+2*EDGE_SPACER);

    GDrawSetVisible(gw,true);
}
