/* Copyright (C) 2000-2002 by George Williams */
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
#include <gdraw.h>
#include "ggadgetP.h"
#include <gwidget.h>
#include <ustring.h>
#include <gkeysym.h>
#include <utype.h>
#include <gresource.h>

static GBox menubar_box = { /* Don't initialize here */ 0 };
static GBox menu_box = { /* Don't initialize here */ 0 };
static FontInstance *menu_font = NULL;
static int gmenubar_inited = false;
#ifndef _Keyboard
# define _Keyboard 0
#endif
static enum { kb_ibm, kb_mac, kb_sun, kb_ppc } keyboard = _Keyboard;
/* Sigh. In XonX the command key is mapped to 0x20 and Option to 0x8 (meta) */
/*  the option key conversions (option-c => ccidilla) are not done */
/*  While in Suse PPC X, the command key is 0x8 (meta) and option is 0x2000 */
/*  and the standard mac option conversions are done */

static void GMenuBarChangeSelection(GMenuBar *mb, int newsel,GEvent *);
static struct gmenu *GMenuCreateSubMenu(struct gmenu *parent,GMenuItem *mi);
static struct gmenu *GMenuCreatePulldownMenu(GMenuBar *mb,GMenuItem *mi);

static void GMenuInit() {
    FontRequest rq;
    char *keystr, *end;

    GGadgetInit();
    GDrawDecomposeFont(_ggadget_default_font,&rq);
    rq.weight = 700;
    menu_font = GDrawInstanciateFont(screen_display,&rq);
    _GGadgetCopyDefaultBox(&menubar_box);
    _GGadgetCopyDefaultBox(&menu_box);
    menubar_box.border_shape = menu_box.border_shape = bs_rect;
    menubar_box.border_width = 0;
    menu_box.padding = 1;
    menubar_box.flags = box_foreground_border_outer;
    menu_box.flags = box_foreground_border_outer;
    menu_font = _GGadgetInitDefaultBox("GMenuBar.",&menubar_box,menu_font);
    menu_font = _GGadgetInitDefaultBox("GMenu.",&menu_box,menu_font);
    keystr = GResourceFindString("Keyboard");
    if ( keystr!=NULL ) {
	if ( strmatch(keystr,"mac")==0 ) keyboard = kb_mac;
	else if ( strmatch(keystr,"sun")==0 ) keyboard = kb_sun;
	else if ( strmatch(keystr,"ppc")==0 ) keyboard = kb_ppc;
	else if ( strmatch(keystr,"ibm")==0 || strmatch(keystr,"pc")==0 ) keyboard = kb_ibm;
	else if ( strtol(keystr,&end,10), *end=='\0' )
	    keyboard = strtol(keystr,NULL,10);
    }
    gmenubar_inited = true;
    _GGroup_Init();
}

typedef struct gmenu {
    unsigned int hasticks: 1;
    unsigned int pressed: 1;
    unsigned int initial_press: 1;
    unsigned int scrollup: 1;
    unsigned int freemi: 1;
    int bp;
    int tickoff, tioff, rightedge;
    int width, height;
    int line_with_mouse;
    int offtop, lcnt, mcnt;
    GMenuItem *mi;
    int fh, as;
    GWindow w;
    GBox *box;
    struct gmenu *parent, *child;
    struct gmenubar *menubar;
    GWindow owner;
    GTimer *scrollit;
    FontInstance *font;
} GMenu;

static void shorttext(GMenuItem *gi,unichar_t *buf) {
    unichar_t *pt = buf;

    if ( gi->shortcut==0 ) {
	*pt = '\0';
return;
    }
    if ( gi->short_mask&ksm_meta ) {
	uc_strcpy(pt,keyboard==kb_ibm?"Alt+":keyboard==kb_mac?"Opt+":keyboard==kb_ppc?"Cmd+":"Meta+");
	pt += u_strlen(pt);
    }
    if ( gi->short_mask&ksm_control ) {
	uc_strcpy(pt,keyboard!=kb_mac?"Ctl+":"Cmd+");
	pt += u_strlen(pt);
    }
    if ( gi->short_mask&ksm_shift ) {
	uc_strcpy(pt,"Shft+");
	pt += u_strlen(pt);
    }
    if ( gi->shortcut>=0xff00 && GDrawKeysyms[gi->shortcut-0xff00] )
	u_strcpy(pt,GDrawKeysyms[gi->shortcut-0xff00]);
    else {
	*pt++ = islower(gi->shortcut)?toupper(gi->shortcut):gi->shortcut;
	*pt = '\0';
    }
}

static void GMenuDrawCheckMark(struct gmenu *m, Color fg, int ybase) {
    int x = m->tickoff;
    int as = m->as;
    int pt = GDrawPointsToPixels(m->w,1);

    GDrawSetLineWidth(m->w,pt);
    GDrawDrawLine(m->w,x+2*pt,ybase-as/3,x+as/3,ybase-2*pt,fg);
    GDrawDrawLine(m->w,x+2*pt,ybase-as/3-pt,x+as/3,ybase-3*pt,fg);
    GDrawDrawLine(m->w,x+as/3,ybase-2*pt,x+as/3+as/5,ybase-2*pt-as/4,fg);
    GDrawDrawLine(m->w,x+as/3+as/5,ybase-2*pt-as/4,x+as/3+2*as/5,ybase-2*pt-as/3-as/7,fg);
    GDrawDrawLine(m->w,x+as/3+2*as/5,ybase-2*pt-as/3-as/7,x+as/3+3*as/5,ybase-2*pt-as/3-as/7-as/8,fg);
}

static void GMenuDrawUncheckMark(struct gmenu *m, Color fg, int ybase) {
}

static void GMenuDrawArrow(struct gmenu *m, int ybase) {
    int pt = GDrawPointsToPixels(m->w,1);
    int x = m->rightedge-2*pt;
    int as = 2*(m->as/2);
    GPoint p[3];

    p[0].x = x;			p[0].y = ybase-as/2;
    p[1].x = x-3*(as/2);	p[1].y = ybase;
    p[2].x = p[1].x;		p[2].y = ybase-as;

    GDrawSetLineWidth(m->w,pt);
    GDrawDrawLine(m->w,p[0].x,p[0].y,p[2].x,p[2].y,m->box->border_brighter);
    GDrawDrawLine(m->w,p[0].x-pt,p[0].y,p[2].x+pt,p[2].y+pt,m->box->border_brighter);
    GDrawDrawLine(m->w,p[2].x,p[2].y,p[1].x,p[1].y,m->box->border_brightest);
    GDrawDrawLine(m->w,p[2].x+pt,p[2].y+pt,p[1].x+pt,p[1].y-pt,m->box->border_brightest);
    GDrawDrawLine(m->w,p[1].x,p[1].y,p[0].x,p[0].y,m->box->border_darkest);
    GDrawDrawLine(m->w,p[1].x+pt,p[1].y-pt,p[0].x-pt,p[0].y,m->box->border_darkest);
}

static void GMenuDrawUpArrow(struct gmenu *m, int ybase) {
    int pt = GDrawPointsToPixels(m->w,1);
    int x = (m->rightedge+m->tickoff)/2;
    int as = 2*(m->as/2);
    GPoint p[3];

    p[0].x = x;			p[0].y = ybase - as;
    p[1].x = x-as;		p[1].y = ybase;
    p[2].x = x+as;		p[2].y = ybase;

    GDrawSetLineWidth(m->w,pt);
    GDrawDrawLine(m->w,p[0].x,p[0].y,p[1].x,p[1].y,m->box->border_brightest);
    GDrawDrawLine(m->w,p[0].x,p[0].y+pt,p[1].x+pt,p[1].y,m->box->border_brightest);
    GDrawDrawLine(m->w,p[1].x,p[1].y,p[2].x,p[2].y,m->box->border_darker);
    GDrawDrawLine(m->w,p[1].x+pt,p[1].y,p[2].x-pt,p[2].y,m->box->border_darker);
    GDrawDrawLine(m->w,p[2].x,p[2].y,p[0].x,p[0].y,m->box->border_darkest);
    GDrawDrawLine(m->w,p[2].x-pt,p[2].y,p[0].x,p[0].y+pt,m->box->border_darkest);
}

static void GMenuDrawDownArrow(struct gmenu *m, int ybase) {
    int pt = GDrawPointsToPixels(m->w,1);
    int x = (m->rightedge+m->tickoff)/2;
    int as = 2*(m->as/2);
    GPoint p[3];

    p[0].x = x;			p[0].y = ybase;
    p[1].x = x-as;		p[1].y = ybase - as;
    p[2].x = x+as;		p[2].y = ybase - as;

    GDrawSetLineWidth(m->w,pt);
    GDrawDrawLine(m->w,p[0].x,p[0].y,p[1].x,p[1].y,m->box->border_darker);
    GDrawDrawLine(m->w,p[0].x,p[0].y+pt,p[1].x+pt,p[1].y,m->box->border_darker);
    GDrawDrawLine(m->w,p[1].x,p[1].y,p[2].x,p[2].y,m->box->border_brightest);
    GDrawDrawLine(m->w,p[1].x+pt,p[1].y,p[2].x-pt,p[2].y,m->box->border_brightest);
    GDrawDrawLine(m->w,p[2].x,p[2].y,p[0].x,p[0].y,m->box->border_darkest);
    GDrawDrawLine(m->w,p[2].x-pt,p[2].y,p[0].x,p[0].y+pt,m->box->border_darkest);
}

static int GMenuDrawMenuLine(struct gmenu *m, GMenuItem *mi, int y) {
    unichar_t shortbuf[30];
    int as = GTextInfoGetAs(m->w,&mi->ti,m->font);
    int h, width;
    Color fg = m->box->main_foreground;
    GRect old, new;
    int ybase = y+as;

    new.x = m->tickoff; new.width = m->rightedge-m->tickoff;
    new.y = y; new.height = GTextInfoGetHeight(m->w,&mi->ti,m->font);
    GDrawPushClip(m->w,&new,&old);

    if ( mi->ti.fg!=COLOR_DEFAULT && mi->ti.fg!=COLOR_UNKNOWN )
	fg = mi->ti.fg;
    if ( mi->ti.disabled )
	fg = m->box->disabled_foreground;
    if ( fg==COLOR_DEFAULT )
	fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(m->w));

    h = GTextInfoDraw(m->w,m->tioff,y,&mi->ti,m->font,
	    mi->ti.disabled?m->box->disabled_foreground:fg,
	    m->box->active_border,new.y+new.height);

    if ( mi->ti.checkable ) {
	if ( mi->ti.checked )
	    GMenuDrawCheckMark(m,fg,ybase);
	else
	    GMenuDrawUncheckMark(m,fg,ybase);
    }

    if ( mi->sub!=NULL )
	GMenuDrawArrow(m,ybase);
    else if ( mi->shortcut!=0 ) {
	shorttext(mi,shortbuf);

	width = GDrawGetTextWidth(m->w,shortbuf,-1,NULL);
	GDrawDrawText(m->w,m->rightedge-width,ybase,shortbuf,-1,NULL,fg);
    }
    GDrawPopClip(m->w,&old);
return( y + h );
}

static int gmenu_expose(struct gmenu *m, GEvent *event) {
    GRect old1, old2;
    GRect r;
    int i;

    GDrawPushClip(m->w,&event->u.expose.rect,&old1);
    r.x = 0; r.width = m->width; r.y = 0; r.height = m->height;
    GBoxDrawBackground(m->w,&r,m->box,gs_active,false);
    GBoxDrawBorder(m->w,&r,m->box,gs_active,false);
    r.x = m->tickoff; r.width = m->rightedge-m->tickoff;
    r.y = m->bp; r.height = m->height - 2*m->bp;
    GDrawPushClip(m->w,&r,&old2);
    for ( i = event->u.expose.rect.y/m->fh+m->offtop; i<m->mcnt &&
	    i<=(event->u.expose.rect.y+event->u.expose.rect.height)/m->fh+m->offtop;
	    ++i ) {
	if ( i==m->offtop && m->offtop!=0 )
	    GMenuDrawUpArrow(m, m->bp+m->as);
	else if ( m->lcnt!=m->mcnt && i==m->lcnt+m->offtop-1 )
	    GMenuDrawDownArrow(m, m->bp+(i-m->offtop)*m->fh+m->as);
	else
	    GMenuDrawMenuLine(m, &m->mi[i], m->bp+(i-m->offtop)*m->fh);
    }
    GDrawPopClip(m->w,&old2);
    GDrawPopClip(m->w,&old1);
return( true );
}
	    
static void GMenuDrawLines(struct gmenu *m, int ln, int cnt) {
    GRect r, old1, old2, winrect;

    winrect.x = 0; winrect.width = m->width; winrect.y = 0; winrect.height = m->height;
    r = winrect; r.height = cnt*m->fh;
    r.y = (ln-m->offtop)*m->fh+m->bp;
    GDrawPushClip(m->w,&r,&old1);
    GBoxDrawBackground(m->w,&winrect,m->box,gs_active,false);
    GBoxDrawBorder(m->w,&winrect,m->box,gs_active,false);
    r.x = m->tickoff; r.width = m->rightedge-r.x;
    GDrawPushClip(m->w,&r,&old2);
    cnt += ln;
    for ( ; ln<cnt; ++ln )
	GMenuDrawMenuLine(m, &m->mi[ln], m->bp+(ln-m->offtop)*m->fh);
    GDrawPopClip(m->w,&old2);
    GDrawPopClip(m->w,&old1);
}

static void GMenuSetPressed(struct gmenu *m, int pressed) {
    while ( m->child!=NULL ) m = m->child;
    while ( m->parent!=NULL ) {
	m->pressed = pressed;
	m = m->parent;
    }
    m->pressed = pressed;
    if ( m->menubar!=NULL )
	m->menubar->pressed = pressed;
}

static void GMenuDestroy(struct gmenu *m) {
    if ( m->line_with_mouse!=-1 )
	m->mi[m->line_with_mouse].ti.selected = false;
    if ( m->child!=NULL )
	GMenuDestroy(m->child);
    if ( m->parent!=NULL )
	m->parent->child = NULL;
    else if ( m->menubar!=NULL ) {
	m->menubar->child = NULL;
	m->menubar->pressed = false;
	_GWidget_ClearPopupOwner((GGadget *) (m->menubar));
	_GWidget_ClearGrabGadget((GGadget *) (m->menubar));
	GMenuBarChangeSelection(m->menubar,-1,NULL);
	GDrawPointerUngrab(GDrawGetDisplayOfWindow(m->menubar->g.base));
    }
    GDrawDestroyWindow(m->w);
    /* data are freed when we get the destroy event !!!! */
}

static void GMenuHideAll(struct gmenu *m) {
    if ( m!=NULL ) {
	while ( m->parent ) m = m->parent;
	while ( m ) {
	    GDrawSetVisible(m->w,false);
	    m=m->child;
	}
    }
}

static void GMenuDismissAll(struct gmenu *m) {
#if 0
 printf("DismissAll\n");
#endif
    if ( m!=NULL ) {
	while ( m->parent ) m = m->parent;
	GMenuDestroy(m);
    }
}

static void GMenuChangeSelection(struct gmenu *m, int newsel,GEvent *event) {
    int old=m->line_with_mouse;

    if ( old==newsel )
return;
    if ( m->child!=NULL ) {
	GMenuDestroy(m->child);
	m->child = NULL;
    }
    m->initial_press = false;
    m->line_with_mouse = newsel;
    if ( newsel!=-1 )
	m->mi[newsel].ti.selected = true;
    if ( old!=-1 )
	m->mi[old].ti.selected = false;

    if ( newsel==old+1 && old!=-1 ) {
	GMenuDrawLines(m,old,2);
    } else if ( old==newsel+1 && newsel!=-1 ) {
	GMenuDrawLines(m,newsel,2);
    } else {
	if ( newsel!=-1 )
	    GMenuDrawLines(m,newsel,1);
	if ( old!=-1 )
	    GMenuDrawLines(m,old,1);
    }
    if ( newsel!=-1 ) {
	if ( m->mi[newsel].moveto!=NULL )
	    (m->mi[newsel].moveto)(m->owner,&m->mi[newsel],event);
	if ( m->mi[newsel].sub!=NULL )
	    m->child = GMenuCreateSubMenu(m,m->mi[newsel].sub);
    }
}

static void GMenuBarChangeSelection(GMenuBar *mb, int newsel,GEvent *event) {
    int old=mb->entry_with_mouse;
    GMenuItem *mi;

    if ( old==newsel )
return;
    if ( mb->child!=NULL ) {
	int waspressed = mb->pressed;
	GMenuDestroy(mb->child);
	mb->child = NULL;
	mb->pressed = waspressed;
    }
    mb->entry_with_mouse = newsel;
    if ( newsel!=-1 )
	mb->mi[newsel].ti.selected = true;
    if ( old!=-1 )
	mb->mi[old].ti.selected = false;

    _ggadget_redraw(&mb->g);
    if ( newsel!=-1 ) {
	mi = newsel==mb->lastmi ? mb->fake : &mb->mi[newsel];
	if ( mi->moveto!=NULL )
	    (mi->moveto)(mb->g.base,mi,event);
	if ( mi->sub!=NULL )
	    mb->child = GMenuCreatePulldownMenu(mb,mi->sub);
    }
}

static int gmenu_mouse(struct gmenu *m, GEvent *event) {
    GPoint p;

    p.x = event->u.mouse.x; p.y = event->u.mouse.y;

  retry:
    event->u.mouse.x = p.x; event->u.mouse.y = p.y; event->w = m->w;
    if ( m->scrollit ) {
	GDrawCancelTimer(m->scrollit); m->scrollit = NULL;
    }
    if ( event->type == et_crossing && !event->u.crossing.entered )
	m->initial_press = false;
    if (( m->pressed && (event->type==et_mousemove || event->type==et_crossing)) ||
	    event->type == et_mousedown ) {
	if ( m->parent!=NULL && GDrawEventInWindow(m->parent->w,event) ) {
	    GDrawPointerUngrab(GDrawGetDisplayOfWindow(m->w));
	    GDrawPointerGrab(m->parent->w);
	    GDrawTranslateCoordinates(m->w,m->parent->w,&p);
	    m = m->parent;
  goto retry;
	} else if ( m->child!=NULL && GDrawEventInWindow(m->child->w,event) ) {
	    GDrawPointerUngrab(GDrawGetDisplayOfWindow(m->w));
	    GDrawPointerGrab(m->child->w);
	    GDrawTranslateCoordinates(m->w,m->child->w,&p);
	    m = m->child;
  goto retry;
	} else if ( m->menubar!=NULL && event->u.mouse.y<0 ) {
	    GDrawPointerUngrab(GDrawGetDisplayOfWindow(m->w));
	    GDrawPointerGrab(m->menubar->g.base);
	    GDrawTranslateCoordinates(m->w,m->menubar->g.base,&p);
	    event->u.mouse.x = p.x; event->u.mouse.y = p.y;
return( (GDrawGetEH(m->menubar->g.base))(m->menubar->g.base,event));
	} else if ( event->u.mouse.y<m->bp || event->u.mouse.x<0 ||
		event->u.mouse.y>=m->height-m->bp || event->u.mouse.x >= m->width ) {
	    GMenuChangeSelection(m,-1,event);
	    if ( event->type == et_mousedown )
		GMenuDismissAll(m);
	} else {
	    int l = (event->u.mouse.y-m->bp)/m->fh;
	    int i = l + m->offtop;
	    if ( l==0 && m->offtop!=0 ) {
		GMenuChangeSelection(m,-1,event);
		m->scrollit = GDrawRequestTimer(m->w,_GScrollBar_RepeatTime,_GScrollBar_RepeatTime,m);
		m->scrollup = true;
	    } else if ( l==m->lcnt-1 && m->offtop+m->lcnt<m->mcnt ) {
		GMenuChangeSelection(m,-1,event);
		m->scrollit = GDrawRequestTimer(m->w,_GScrollBar_RepeatTime,_GScrollBar_RepeatTime,m);
		m->scrollup = false;
	    } else if ( event->type == et_mousedown && m->child!=NULL &&
		    i == m->line_with_mouse ) {
		GMenuChangeSelection(m,-1,event);
		m->initial_press = true;
	    } else
		GMenuChangeSelection(m,i,event);
	    if ( event->type == et_mousedown ) {
		GMenuSetPressed(m,true);
		if ( m->child )
		    m->initial_press = true;
	    }
	}
    } else if ( event->type == et_mouseup && !m->initial_press ) {
	int dismissed = false;
#if 0
 printf("\nActivate menu\n");
#endif
	if ( event->u.mouse.y>=m->bp && event->u.mouse.x>=0 &&
		event->u.mouse.y<m->height-m->bp && event->u.mouse.x < m->width ) {
	    int l = (event->u.mouse.y-m->bp)/m->fh;
	    int i = l + m->offtop;
	    if ( !( l==0 && m->offtop!=0 ) &&
		    !( l==m->lcnt-1 && m->offtop+m->lcnt<m->mcnt ) &&
		    !m->mi[i].ti.disabled && !m->mi[i].ti.line ) {
		if ( m->mi[i].ti.checkable )
		    m->mi[i].ti.checked = !m->mi[i].ti.checked;
		GMenuDismissAll(m);
		dismissed = true;
		if ( m->mi[i].invoke!=NULL )
		    (m->mi[i].invoke)(m->owner,&m->mi[i],event);
	    }
	}
	if ( !dismissed )
	    GMenuDismissAll(m);
    } else if ( event->type == et_mouseup ) {
	m->initial_press = false;
	GMenuSetPressed(m,false);
    } else
return( false );

return( true );
}

static int gmenu_timer(struct gmenu *m, GEvent *event) {
    if ( m->scrollup ) {
	if ( --m->offtop<0 ) m->offtop = 0;
	if ( m->offtop == 0 ) {
	    GDrawCancelTimer(m->scrollit);
	    m->scrollit = NULL;
	}
    } else {
	++m->offtop;
	if ( m->offtop + m->lcnt > m->mcnt )
	    m->offtop = m->mcnt-m->lcnt;
	if ( m->offtop == m->mcnt-m->lcnt ) {
	    GDrawCancelTimer(m->scrollit);
	    m->scrollit = NULL;
	}
    }
    GDrawRequestExpose(m->w, NULL, false);
return( true );
}

static int GMenuKeyInvoke(struct gmenu *m, int i) {
    GMenuChangeSelection(m,i,NULL);
    if ( m->mi[i].ti.checkable )
	m->mi[i].ti.checked = !m->mi[i].ti.checked;
    if ( m->mi[i].sub==NULL ) {
	GMenuHideAll(m);
    }
    if ( m->mi[i].invoke!=NULL )
	(m->mi[i].invoke)(m->owner,&m->mi[i],NULL);
    if ( m->mi[i].sub==NULL ) {
	GMenuDismissAll(m);
    }
return( true );
}

static int GMenuBarKeyInvoke(struct gmenubar *mb, int i) {
    GMenuBarChangeSelection(mb,i,NULL);
    if ( mb->mi[i].invoke!=NULL )
	(mb->mi[i].invoke)(mb->g.base,&mb->mi[i],NULL);
return( true );
}

static GMenuItem *GMenuSearchShortcut(GMenuItem *mi, GEvent *event) {
    int i;
    unichar_t keysym = event->u.chr.keysym;

    if ( islower(keysym)) keysym = toupper(keysym);
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	if ( mi[i].sub==NULL && mi[i].shortcut == keysym &&
		((ksm_shift|ksm_control|ksm_meta)&event->u.chr.state)==mi[i].short_mask )
return( &mi[i]);
	else if ( mi[i].sub!=NULL ) {
	    GMenuItem *ret = GMenuSearchShortcut(mi[i].sub,event);
	    if ( ret!=NULL )
return( ret );
	}
    }
return( NULL );
}

static int GMenuSpecialKeys(struct gmenu *m, unichar_t keysym, GEvent *event) {
    switch ( keysym ) {
      case GK_Escape:
	GMenuDestroy(m);
return( true );
      case GK_Return:
	if ( m->mi[m->line_with_mouse].sub!=NULL && m->child==NULL ) {
	    m->child = GMenuCreateSubMenu(m,m->mi[m->line_with_mouse].sub);
return( true );
	} else {
	    int i = m->line_with_mouse;
	    if ( !m->mi[i].ti.disabled && !m->mi[i].ti.line ) {
		if ( m->mi[i].ti.checkable )
		    m->mi[i].ti.checked = !m->mi[i].ti.checked;
		GMenuDismissAll(m);
		if ( m->mi[i].invoke!=NULL )
		    (m->mi[i].invoke)(m->owner,&m->mi[i],event);
	    } else
		GMenuDismissAll(m);
return( true );
	}
      case GK_Left: case GK_KP_Left:
	if ( m->parent!=NULL ) {
	    GMenuDestroy(m);
return( true );
	} else if ( m->line_with_mouse==-1 && m->menubar!=NULL ) {
	    GMenuBar *mb = m->menubar;
	    int en = mb->entry_with_mouse;
	    if ( en>0 ) {
		GMenuBarChangeSelection(mb,en-1,event);
	    }
return( true );
	}
	/* Else fall into the "Up" case */
      case GK_Up: case GK_KP_Up: {
	int ns = m->line_with_mouse-1;
	if ( m->line_with_mouse==-1 ) {
	    GMenuDestroy(m);
	} else {
	    while ( ns>=0 && (m->mi[ns].ti.disabled || m->mi[ns].ti.line)) --ns;
	    if ( ns<0 ) ns= -1;
	    GMenuChangeSelection(m,ns,NULL);
	}
return( true );
      }
      case GK_Right: case GK_KP_Right:
	if ( m->line_with_mouse==-1 ) {
	    if ( m->parent==NULL && m->menubar!=NULL ) {
		GMenuBar *mb = m->menubar;
		int en = mb->entry_with_mouse;
		if ( en<mb->lastmi ) {
		    GMenuBarChangeSelection(mb,en+1,event);
		}
return( true );
	    } else if ( m->mcnt!=0 ) {
		GMenuChangeSelection(m,m->line_with_mouse+1,event);
return( true );
	    }
	} else if ( m->mi[m->line_with_mouse].sub!=NULL && m->child==NULL ) {
	    m->child = GMenuCreateSubMenu(m,m->mi[m->line_with_mouse].sub);
return( true );
	}
      /* Fall through into the "Down" case */
      case GK_Down: case GK_KP_Down: {
	int ns = m->line_with_mouse+1;
	while ( ns<m->mcnt && (m->mi[ns].ti.disabled || m->mi[ns].ti.line)) ++ns;
	if ( m->line_with_mouse==-1 && m->parent!=NULL )
	    GMenuDestroy(m);		/*  stay in parent */
	else if ( ns!=m->mcnt )
	    GMenuChangeSelection(m,ns,event);
	else if ( m->parent!=NULL )	/* If we've reached the end of a child */
	    GMenuDestroy(m);		/*  go back to the parent */
return( true );
      }
    }
return( false );
}

static int gmenu_key(struct gmenu *m, GEvent *event) {
    int i;
    GMenuItem *mi;
    GMenu *top;
    unichar_t keysym = event->u.chr.keysym;

    if ( islower(keysym)) keysym = toupper(keysym);
    if ( event->u.chr.state&ksm_meta && !(event->u.chr.state&ksm_control)) {
	/* Only look for mneumonics in the child */
	while ( m->child!=NULL )
	    m = m->child;
	for ( i=0; i<m->mcnt; ++i ) {
	    if ( m->mi[i].ti.mnemonic == keysym &&
			!m->mi[i].ti.disabled ) {
		GMenuKeyInvoke(m,i);
return( true );
	    }
	}
    }

    /* then look for shortcuts everywhere */
    if ( event->u.chr.state&(ksm_control|ksm_meta) ||
	    event->u.chr.keysym>=GK_Special ) {
	for ( top = m; top->parent!=NULL ; top = top->parent );
	if ( top->menubar!=NULL )
	    mi = GMenuSearchShortcut(top->menubar->mi,event);
	else
	    mi = GMenuSearchShortcut(top->mi,event);
	if ( mi!=NULL ) {
	    if ( mi->ti.checkable )
		mi->ti.checked = !mi->ti.checked;
	    GMenuHideAll(top);
	    if ( mi->invoke!=NULL )
		(mi->invoke)(m->owner,mi,event);
	    GMenuDestroy(m);
return( true );
	}
	for ( ; m->child!=NULL ; m = m->child );
return( GMenuSpecialKeys(m,event->u.chr.keysym,event));
    }
    
return( false );
}

static int gmenu_destroy(struct gmenu *m) {
#if 0
 printf("gmenu_destroy\n");
#endif
    if ( m->freemi )
	GMenuItemArrayFree(m->mi);
    free(m);
return( true );
}

static int gmenu_eh(GWindow w,GEvent *ge) {
    GMenu *m = (GMenu *) GDrawGetUserData(w);

    switch ( ge->type ) {
      case et_expose:
return( gmenu_expose(m,ge));
      case et_char:
return( gmenu_key(m,ge));
      case et_mousemove: case et_mousedown: case et_mouseup: case et_crossing:
return( gmenu_mouse(m,ge));
      case et_timer:
return( gmenu_timer(m,ge));
      case et_destroy:
return( gmenu_destroy(m));
      case et_close:
	GMenuDestroy(m);
return( true );
    }
return( false );
}

static GMenu *_GMenu_Create(GWindow owner,GMenuItem *mi, GPoint *where, int awidth, int aheight, GFont *font) {
    GMenu *m = gcalloc(1,sizeof(GMenu));
    GRect pos;
    GDisplay *disp = GDrawGetDisplayOfWindow(owner);
    GWindowAttrs pattrs;
    int i, width, keywidth;
    unichar_t buffer[100];
    FontInstance *old;
    int ds, ld, temp;
    GRect screen;

    m->owner = owner;
    m->mi = mi;
    m->font = font;
    m->box = &menu_box;
    m->tickoff = m->tioff = m->bp = GBoxBorderWidth(owner,m->box);
    GDrawFontMetrics(m->font,&m->as, &ds, &ld);
    m->fh = m->as + ds + 1;		/* I need some extra space, else mneumonic underlines look bad */
    m->line_with_mouse = -1;

    old = GDrawSetFont(owner,m->font);
    m->hasticks = false; width = 0; keywidth = 0;
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	if ( mi[i].ti.checkable ) m->hasticks = true;
	temp = GTextInfoGetWidth(owner,&mi[i].ti,m->font);
	if ( temp>width ) width = temp;
	shorttext(&mi[i],buffer);
	temp = GDrawGetTextWidth(owner,buffer,-1,NULL);
	if ( temp>keywidth ) keywidth=temp;
	if ( mi[i].sub!=NULL && 3*m->as>keywidth )
	    keywidth = 3*m->as;
    }
    m->mcnt = m->lcnt = i;
    if ( keywidth!=0 ) width += keywidth + GDrawPointsToPixels(owner,8);
    if ( m->hasticks ) {
	int ticklen = m->as + GDrawPointsToPixels(owner,5);
	width += ticklen;
	m->tioff += ticklen;
    }
    m->width = pos.width = width + 2*m->bp;
    m->rightedge = m->width - m->bp;
    m->height = pos.height = i*m->fh + 2*m->bp;
    GDrawGetSize(GDrawGetRoot(disp),&screen);
    if ( pos.height > screen.height ) {
	m->lcnt = (screen.height-2*m->bp)/m->fh;
	pos.height = m->lcnt*m->fh + 2*m->bp;
    }

    pos.x = where->x; pos.y = where->y;
    if ( pos.x+pos.width > screen.width ) {
	if ( where->x+awidth-pos.width >= 0 )
	    pos.x = where->x+awidth-pos.width;
	else
	    pos.x = 0;
    }
    if ( pos.y + pos.height > screen.height ) {
	if ( where->y+aheight-pos.height >= 0 )
	    pos.y = where->y+aheight-pos.height;
	else
	    pos.y = 0;
    }

/* Mnemonics in menus don't work under gnome. Turning off nodecor makes them */
/*  work, but that seems a high price to pay */
    pattrs.mask = wam_events|wam_nodecor|wam_positioned|wam_cursor|wam_transient;
    pattrs.event_masks = -1;
    pattrs.nodecoration = true;
    pattrs.positioned = true;
    pattrs.cursor = ct_pointer;
    pattrs.transient = GWidgetGetTopWidget(owner);

    m->w = GDrawCreateTopWindow(disp,&pos,gmenu_eh,m,&pattrs);
    GDrawSetVisible(m->w,true);
return( m );
}

static GMenu *GMenuCreateSubMenu(GMenu *parent,GMenuItem *mi) {
    GPoint p;
    GMenu *m;

    p.x = parent->width;
    p.y = (parent->line_with_mouse-parent->offtop)*parent->fh + parent->bp;
    GDrawTranslateCoordinates(parent->w,GDrawGetRoot(GDrawGetDisplayOfWindow(parent->w)),&p);
    m = _GMenu_Create(parent->owner,mi,&p,-parent->width,parent->fh,parent->font);
    m->parent = parent;
    m->pressed = parent->pressed;
return( m );
}

static GMenu *GMenuCreatePulldownMenu(GMenuBar *mb,GMenuItem *mi) {
    GPoint p;
    GMenu *m;

    p.x = mb->g.inner.x + mb->xs[mb->entry_with_mouse]-
	    GBoxDrawnWidth(mb->g.base,&menu_box );
    p.y = mb->g.r.y + mb->g.r.height;
    GDrawTranslateCoordinates(mb->g.base,GDrawGetRoot(GDrawGetDisplayOfWindow(mb->g.base)),&p);
    m = _GMenu_Create(mb->g.base,mi,&p,
	    mb->xs[mb->entry_with_mouse+1]-mb->xs[mb->entry_with_mouse],
	    -mb->g.r.height,mb->font);
    m->menubar = mb;
    m->pressed = mb->pressed;
    _GWidget_SetPopupOwner((GGadget *) mb);
return( m );
}

GWindow GMenuCreatePopupMenu(GWindow owner,GEvent *event, GMenuItem *mi) {
    GPoint p;
    GMenu *m;
    GEvent e;

    p.x = event->u.mouse.x;
    p.y = event->u.mouse.y;
    GDrawTranslateCoordinates(owner,GDrawGetRoot(GDrawGetDisplayOfWindow(owner)),&p);
    m = _GMenu_Create(owner,GMenuItemArrayCopy(mi,NULL),&p,0,0,menu_font);
    GDrawPointerUngrab(GDrawGetDisplayOfWindow(owner));
    GDrawPointerGrab(m->w);
    GDrawGetPointerPosition(m->w,&e);
    if ( e.u.mouse.state & (ksm_button1|ksm_button2|ksm_button3) )
	m->pressed = m->initial_press = true;
    m->freemi = true;
return( m->w );
}

/* ************************************************************************** */
int GMenuBarCheckKey(GGadget *g, GEvent *event) {
    int i;
    GMenuBar *mb = (GMenuBar *) g;
    GMenuItem *mi;
    unichar_t keysym = event->u.chr.keysym;

    if ( g==NULL )
return( false );

    if ( islower(keysym)) keysym = toupper(keysym);
    if ( event->u.chr.state&ksm_meta && !(event->u.chr.state&ksm_control)) {
	/* Only look for mneumonics in the leaf of the displayed menu structure */
	if ( mb->child!=NULL )
return( gmenu_key(mb->child,event));	/* this routine will do shortcuts too */

	for ( i=0; i<mb->mtot; ++i ) {
	    if ( mb->mi[i].ti.mnemonic == keysym &&
			!mb->mi[i].ti.disabled ) {
		GMenuBarKeyInvoke(mb,i);
return( true );
	    }
	}
    }

    /* then look for shortcuts everywhere */
    if ( event->u.chr.state&(ksm_control|ksm_meta) ||
	    event->u.chr.keysym>=GK_Special ) {
	mi = GMenuSearchShortcut(mb->mi,event);
	if ( mi!=NULL ) {
	    if ( mi->ti.checkable )
		mi->ti.checked = !mi->ti.checked;
	    if ( mi->invoke!=NULL )
		(mi->invoke)(mb->g.base,mi,NULL);
	    if ( mb->child != NULL )
		GMenuDestroy(mb->child);
return( true );
	}
    }
    if ( mb->child!=NULL ) {
	GMenu *m;
	for ( m=mb->child; m->child!=NULL; m = m->child );
return( GMenuSpecialKeys(m,event->u.chr.keysym,event));
    }

    if ( event->u.chr.keysym==GK_Menu )
	GMenuCreatePopupMenu(event->w,event, mb->mi);

return( false );
}

static void GMenuBarDrawDownArrow(GWindow pixmap, GMenuBar *mb, int x) {
    int pt = GDrawPointsToPixels(pixmap,1);
    int size = 2*(mb->g.inner.height/3);
    int ybase = mb->g.inner.y + size + (mb->g.inner.height-size)/2;
    GPoint p[3];

    p[0].x = x+size;		p[0].y = ybase;
    p[1].x = x;			p[1].y = ybase - size;
    p[2].x = x+2*size;		p[2].y = ybase - size;

    GDrawSetLineWidth(pixmap,pt);
    GDrawDrawLine(pixmap,p[0].x,p[0].y,p[1].x,p[1].y,mb->g.box->border_darker);
    GDrawDrawLine(pixmap,p[0].x,p[0].y+pt,p[1].x+pt,p[1].y,mb->g.box->border_darker);
    GDrawDrawLine(pixmap,p[1].x,p[1].y,p[2].x,p[2].y,mb->g.box->border_brightest);
    GDrawDrawLine(pixmap,p[1].x+pt,p[1].y,p[2].x-pt,p[2].y,mb->g.box->border_brightest);
    GDrawDrawLine(pixmap,p[2].x,p[2].y,p[0].x,p[0].y,mb->g.box->border_darkest);
    GDrawDrawLine(pixmap,p[2].x-pt,p[2].y,p[0].x,p[0].y+pt,mb->g.box->border_darkest);
}

static int gmenubar_expose(GWindow pixmap, GGadget *g, GEvent *expose) {
    GMenuBar *mb = (GMenuBar *) g;
    GRect r,old1,old2, old3;
    Color fg = mb->g.box->main_foreground;
    int i;

    if ( fg==COLOR_DEFAULT )
	fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(mb->g.base));

    GDrawPushClip(pixmap,&g->r,&old1);

    GBoxDrawBackground(pixmap,&g->r,g->box, g->state,false);
    GBoxDrawBorder(pixmap,&g->r,g->box,g->state,false);
    GDrawPushClip(pixmap,&g->inner,&old2);
    GDrawSetFont(pixmap,mb->font);

    r = g->inner;
    for ( i=0; i<mb->lastmi; ++i ) {
	r.x = mb->xs[i]+mb->g.inner.x; r.width = mb->xs[i+1]-mb->xs[i];
	GDrawPushClip(pixmap,&r,&old3);
	GTextInfoDraw(pixmap,r.x,r.y,&mb->mi[i].ti,mb->font,
		mb->mi[i].ti.disabled?mb->g.box->disabled_foreground:fg,
		mb->g.box->active_border,r.y+r.height);
	GDrawPopClip(pixmap,&old3);
    }
    if ( i<mb->mtot ) {
	GMenuBarDrawDownArrow(pixmap,mb,mb->xs[i]+mb->g.inner.x);
    }

    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
return( true );
}

static int GMenuBarIndex(GMenuBar *mb, int x ) {
    int i;

    if ( x<0 )
return( -1 );
    for ( i=0; i< mb->lastmi; ++i )
	if ( x<mb->g.inner.x+mb->xs[i+1] )
return( i );
    if ( mb->lastmi!=mb->mtot )
return( mb->lastmi );

return( -1 );
}

static int gmenubar_mouse(GGadget *g, GEvent *event) {
    GMenuBar *mb = (GMenuBar *) g;
    int which;

    if ( event->type == et_mousedown ) {
	mb->pressed = true;
	if ( mb->child==NULL )
	    mb->initial_press = true;
	else
	    GMenuSetPressed(mb->child,true);
	which = GMenuBarIndex(mb,event->u.mouse.x);
	if ( which==mb->entry_with_mouse && mb->child!=NULL )
	    GMenuDestroy(mb->child);
	else
	    GMenuBarChangeSelection(mb,which,event);
    } else if ( event->type == et_crossing && mb->pressed ) {
	mb->initial_press = false;
    } else if ( event->type == et_mousemove && mb->pressed ) {
	if ( GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y))
	    GMenuBarChangeSelection(mb,GMenuBarIndex(mb,event->u.mouse.x),event);
	else if ( mb->child!=NULL ) {
	    GPoint p;

	    p.x = event->u.mouse.x; p.y = event->u.mouse.y;
	    GDrawTranslateCoordinates(mb->g.base,mb->child->w,&p);
	    if ( p.x>=0 && p.y>=0 && p.x<mb->child->width && p.y<mb->child->height ) {
		GDrawPointerUngrab(GDrawGetDisplayOfWindow(mb->g.base));
		GDrawPointerGrab(mb->child->w);
		event->u.mouse.x = p.x; event->u.mouse.y = p.y;
		event->w = mb->child->w;
		gmenu_mouse(mb->child,event);
	    }
	}
    } else if ( event->type == et_mouseup &&
	    (!mb->initial_press ||
	      !GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y))) {
	GMenuBarChangeSelection(mb,-1,event);
	mb->pressed = false;
    } else if ( event->type == et_mouseup ) {
	mb->pressed = false;
	if ( mb->child!=NULL )
	    GMenuSetPressed(mb->child,false);
    }
return( false );
}

static void gmenubar_destroy(GGadget *g) {
    GMenuBar *mb = (GMenuBar *) g;
    if ( g==NULL )
return;
    if ( mb->child!=NULL ) {
	GMenuDestroy(mb->child);
	GDrawSync(NULL);
	GDrawProcessPendingEvents(NULL);	/* popup's destroy routine must execute before we die */
    }
    GMenuItemArrayFree(mb->mi);
    free(mb->xs);
    _ggadget_destroy(g);
}

static void GMenuBarSetFont(GGadget *g,FontInstance *new) {
    GMenuBar *b = (GMenuBar *) g;
    b->font = new;
}

static FontInstance *GMenuBarGetFont(GGadget *g) {
    GMenuBar *b = (GMenuBar *) g;
return( b->font );
}

static void GMenuBarTestSize(GMenuBar *mb) {
    int arrow_size = mb->g.inner.height;
    int i;

    if ( mb->xs[mb->mtot]<=mb->g.inner.width+4 ) {
	mb->lastmi = mb->mtot;
    } else {
	for ( i=mb->mtot-1; i>0 && mb->xs[i]>mb->g.inner.width-arrow_size; --i );
	mb->lastmi = i;
	memset(&mb->fake,0,sizeof(GMenuItem));
	mb->fake[0].sub = mb->mi+mb->lastmi;
    }
}

static void GMenuBarResize(GGadget *g, int32 width, int32 height) {
    _ggadget_resize(g,width,height);
    GMenuBarTestSize((GMenuBar *) g);
}

struct gfuncs gmenubar_funcs = {
    0,
    sizeof(struct gfuncs),

    gmenubar_expose,
    gmenubar_mouse,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    _ggadget_redraw,
    _ggadget_move,
    GMenuBarResize,
    _ggadget_setvisible,
    _ggadget_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    gmenubar_destroy,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    GMenuBarSetFont,
    GMenuBarGetFont
};

static void GMenuBarFit(GMenuBar *mb,GGadgetData *gd) {
    int bp = GBoxBorderWidth(mb->g.base,mb->g.box );
    GRect r;

    if ( gd->pos.x <= 0 )
	mb->g.r.x = 0;
    if ( gd->pos.y <= 0 )
	mb->g.r.y = 0;
    if ( mb->g.r.width == 0 ) {
	GDrawGetSize(mb->g.base,&r);
	mb->g.r.width = r.width-mb->g.r.x;
    }
    if ( mb->g.r.height == 0 ) {
	int as,ds,ld;
	GDrawFontMetrics(mb->font,&as, &ds, &ld);
	mb->g.r.height = as+ds+2*bp;
    }
    mb->g.inner.x = mb->g.r.x + bp;
    mb->g.inner.y = mb->g.r.y + bp;
    mb->g.inner.width = mb->g.r.width - 2*bp;
    mb->g.inner.height = mb->g.r.height - 2*bp;
}

static void GMenuBarFindXs(GMenuBar *mb) {
    int i, wid;

    GDrawSetFont(mb->g.base,mb->font);
#if 1
    wid = GDrawPointsToPixels(mb->g.base,8);
    mb->xs[0] = GDrawPointsToPixels(mb->g.base,2);
    for ( i=0; i<mb->mtot; ++i )
	mb->xs[i+1] = mb->xs[i]+wid+GDrawGetTextWidth(mb->g.base,mb->mi[i].ti.text,-1,NULL);
#else
    for ( i=wid=0; i<mb->mtot; ++i ) {
	temp = GDrawGetTextWidth(mb->g.base,mb->mi[i].ti.text,-1,NULL);
	if ( temp>wid ) wid = temp;
    }
    wid += GDrawPointsToPixels(mb->g.base,5);
    mb->xs[0] = 0;
    for ( i=0; i<mb->mtot; ++i )
	mb->xs[i+1] = mb->xs[i]+wid;
#endif
    GMenuBarTestSize(mb);
}

GGadget *GMenuBarCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GMenuBar *mb = gcalloc(1,sizeof(GMenuBar));

    if ( !gmenubar_inited )
	GMenuInit();
    mb->g.funcs = &gmenubar_funcs;
    _GGadget_Create(&mb->g,base,gd,data,&menubar_box);

    mb->mi = GMenuItemArrayCopy(gd->u.menu,&mb->mtot);
    mb->xs = galloc((mb->mtot+1)*sizeof(uint16));
    mb->entry_with_mouse = -1;
    mb->font = menu_font;

    GMenuBarFit(mb,gd);
    GMenuBarFindXs(mb);

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&mb->g);
    _GWidget_SetMenuBar(&mb->g);

    mb->g.takes_input = true;
return( &mb->g );
}

/* ************************************************************************** */
static GMenuItem *GMenuBarFindMid(GMenuItem *mi, int mid) {
    int i;
    GMenuItem *ret;

    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	if ( mi[i].mid == mid )
return( &mi[i]);
	if ( mi[i].sub!=NULL ) {
	    ret = GMenuBarFindMid( mi[i].sub, mid );
	    if ( ret!=NULL )
return( ret );
	}
    }
return( NULL );
}

void GMenuBarSetItemChecked(GGadget *g, int mid, int check) {
    GMenuBar *mb = (GMenuBar *) g;
    GMenuItem *item;

    item = GMenuBarFindMid(mb->mi,mid);
    if ( item!=NULL )
	item->ti.checked = check;
}

void GMenuBarSetItemEnabled(GGadget *g, int mid, int enabled) {
    GMenuBar *mb = (GMenuBar *) g;
    GMenuItem *item;

    item = GMenuBarFindMid(mb->mi,mid);
    if ( item!=NULL )
	item->ti.disabled = !enabled;
}

void GMenuBarSetItemName(GGadget *g, int mid, const unichar_t *name) {
    GMenuBar *mb = (GMenuBar *) g;
    GMenuItem *item;

    item = GMenuBarFindMid(mb->mi,mid);
    if ( item!=NULL ) {
	free( item->ti.text );
	item->ti.text = u_copy(name);
    }
}

