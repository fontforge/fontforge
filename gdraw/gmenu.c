/* Copyright (C) 2000-2009 by George Williams */
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
static FontInstance *menu_font = NULL, *menubar_font = NULL;
static int gmenubar_inited = false;
#ifdef __Mac
static int mac_menu_icons = true;
#else
static int mac_menu_icons = false;
#endif
static int mask_set=0;
static int menumask = ksm_control|ksm_meta|ksm_shift;		/* These are the modifier masks expected in menus. Will be overridden by what's actually there */
#ifndef _Keyboard
# define _Keyboard 0
#endif
static enum { kb_ibm, kb_mac, kb_sun, kb_ppc } keyboard = _Keyboard;
/* Sigh. In old XonX the command key is mapped to 0x20 and Option to 0x8 (meta) */
/*  the option key conversions (option-c => ccidilla) are not done */
/*  In the next X, the command key is mapped to 0x10 and Option to 0x2000 */
/*  (again option key conversion are not done) */
/*  In 10.3, the command key is mapped to 0x10 and Option to 0x8 */
/*  In 10.5 the command key is mapped to 0x10 and Option to 0x8 */
/*   (and option conversions are done) */
/*  While in Suse PPC X, the command key is 0x8 (meta) and option is 0x2000 */
/*  and the standard mac option conversions are done */

static GResInfo gmenu_ri;
static GResInfo gmenubar_ri = {
    &gmenu_ri, &ggadget_ri,&gmenu_ri, NULL,
    &menubar_box,
    &menubar_font,
    NULL,
    NULL,
    N_("Menu Bar"),
    N_("Menu Bar"),
    "GMenuBar",
    "Gdraw",
    false,
    omf_border_shape|omf_border_width|box_foreground_border_outer
};
static struct resed menu_re[] = {
    {N_("MacIcons"), "MacIcons", rt_bool, &mac_menu_icons, N_("Whether to use mac-like icons to indicate modifiers (for instance ^ for Control)\nor to use an abreviation (for instance \"Cnt-\")")},
    { NULL }};
static GResInfo gmenu_ri = {
    NULL, &ggadget_ri,&gmenubar_ri, NULL,
    &menu_box,
    &menu_font,
    NULL,
    menu_re,
    N_("Menu"),
    N_("Menu"),
    "GMenu",
    "Gdraw",
    false,
    omf_border_shape|omf_padding|box_foreground_border_outer    
};

static void GMenuBarChangeSelection(GMenuBar *mb, int newsel,GEvent *);
static struct gmenu *GMenuCreateSubMenu(struct gmenu *parent,GMenuItem *mi,int disable);
static struct gmenu *GMenuCreatePulldownMenu(GMenuBar *mb,GMenuItem *mi, int disabled);

static int menu_grabs=true;
static struct gmenu *most_recent_popup_menu = NULL;

static void GMenuInit() {
    FontRequest rq;
    char *keystr, *end;

    GGadgetInit();
    memset(&rq,0,sizeof(rq));
    GDrawDecomposeFont(_ggadget_default_font,&rq);
    rq.weight = 400;
    menu_font = menubar_font = GDrawInstanciateFont(screen_display,&rq);
    _GGadgetCopyDefaultBox(&menubar_box);
    _GGadgetCopyDefaultBox(&menu_box);
    menubar_box.border_shape = menu_box.border_shape = bs_rect;
    menubar_box.border_width = 0;
    menu_box.padding = 1;
    menubar_box.flags |= box_foreground_border_outer;
    menu_box.flags |= box_foreground_border_outer;
    menubar_font = _GGadgetInitDefaultBox("GMenuBar.",&menubar_box,menubar_font);
    menu_font = _GGadgetInitDefaultBox("GMenu.",&menu_box,menubar_font);
    keystr = GResourceFindString("Keyboard");
    if ( keystr!=NULL ) {
	if ( strmatch(keystr,"mac")==0 ) keyboard = kb_mac;
	else if ( strmatch(keystr,"sun")==0 ) keyboard = kb_sun;
	else if ( strmatch(keystr,"ppc")==0 ) keyboard = kb_ppc;
	else if ( strmatch(keystr,"ibm")==0 || strmatch(keystr,"pc")==0 ) keyboard = kb_ibm;
	else if ( strtol(keystr,&end,10), *end=='\0' )
	    keyboard = strtol(keystr,NULL,10);
    }
    menu_grabs = GResourceFindBool("GMenu.Grab",menu_grabs);
    mac_menu_icons = GResourceFindBool("GMenu.MacIcons",mac_menu_icons);
    gmenubar_inited = true;
    _GGroup_Init();
}

typedef struct gmenu {
    unsigned int hasticks: 1;
    unsigned int pressed: 1;
    unsigned int initial_press: 1;
    unsigned int scrollup: 1;
    unsigned int freemi: 1;
    unsigned int disabled: 1;
    unsigned int dying: 1;
    unsigned int hidden: 1;
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
    void (*donecallback)(GWindow owner);
    GIC *gic;
} GMenu;

static void _shorttext(int shortcut, int short_mask, unichar_t *buf) {
    unichar_t *pt = buf;
    static int initted = false;
    struct { int mask; char *modifier; } mods[8] = {
	{ ksm_shift, H_("Shft+") },
	{ ksm_capslock, H_("CapsLk+") },
	{ ksm_control, H_("Ctl+") },
	{ ksm_meta, H_("Alt+") },
	{ 0x10, H_("Flag0x10+") },
	{ 0x20, H_("Flag0x20+") },
	{ 0x40, H_("Flag0x40+") },
	{ 0x80, H_("Flag0x80+") }
	};
    int i;
    char buffer[32];

    if ( !initted ) {
	char *temp;
	for ( i=0; i<8; ++i ) {
	    sprintf( buffer,"Flag0x%02x", 1<<i );
	    temp = dgettext(GMenuGetShortcutDomain(),buffer);
	    if ( strcmp(temp,buffer)!=0 )
		mods[i].modifier = temp;
	    else
		mods[i].modifier = dgettext(GMenuGetShortcutDomain(),mods[i].modifier);
	}
	/* It used to be that the Command key was available to X on the mac */
	/*  but no longer. So we used to use it, but we can't now */
	/* It's sort of available. X11->Preferences->Input->Enable Keyboard shortcuts under X11 needs to be OFF */
	/* if ( strcmp(mods[2].modifier,"Ctl+")==0 ) */
	    /* mods[2].modifier = keyboard!=kb_mac?"Ctl+":"Cmd+"; */
	if ( strcmp(mods[3].modifier,"Alt+")==0 )
	    mods[3].modifier = keyboard==kb_ibm?"Alt+":keyboard==kb_mac?"Opt+":keyboard==kb_ppc?"Cmd+":"Meta+";
    }

    if ( shortcut==0 ) {
	*pt = '\0';
return;
    }

    for ( i=7; i>=0 ; --i ) {
	if ( short_mask&(1<<i) ) {
	    uc_strcpy(pt,mods[i].modifier);
	    pt += u_strlen(pt);
	}
    }

    if ( shortcut>=0xff00 && GDrawKeysyms[shortcut-0xff00] ) {
	cu_strcpy(buffer,GDrawKeysyms[shortcut-0xff00]);
	utf82u_strcpy(pt,dgettext(GMenuGetShortcutDomain(),buffer));
    } else {
	*pt++ = islower(shortcut)?toupper(shortcut):shortcut;
	*pt = '\0';
    }
}

static void shorttext(GMenuItem *gi,unichar_t *buf) {
    _shorttext(gi->shortcut,gi->short_mask,buf);
}

static int GMenuDrawMacIcons(struct gmenu *m, Color fg, int ybase, int x, int mask ) {
    int h = 3*(m->as/3);
    int seg = h/3;

    if ( mask&ksm_cmdmacosx ) {
	GDrawDrawLine(m->w,x,ybase-1,x,ybase-(seg-1),fg);
	GDrawDrawLine(m->w,x,ybase-(2*seg),x,ybase-(h-2),fg);
	GDrawDrawLine(m->w,x+h-1,ybase-1,x+h-1,ybase-(seg-1),fg);
	GDrawDrawLine(m->w,x+h-1,ybase-(2*seg),x+h-1,ybase-(h-2),fg);
	GDrawDrawLine(m->w,x+1,ybase,x+seg-1,ybase,fg);
	GDrawDrawLine(m->w,x+2*seg,ybase,x+h-2,ybase,fg);
	GDrawDrawLine(m->w,x+1,ybase-(h-1),x+seg-1,ybase-(h-1),fg);
	GDrawDrawLine(m->w,x+2*seg,ybase-(h-1),x+h-2,ybase-(h-1),fg);

	GDrawDrawLine(m->w,x+seg,ybase-1,x+seg,ybase-(h-2),fg);
	GDrawDrawLine(m->w,x+2*seg-1,ybase-1,x+2*seg-1,ybase-(h-2),fg);
	GDrawDrawLine(m->w,x+1,ybase-seg,x+h-2,ybase-seg,fg);
	GDrawDrawLine(m->w,x+1,ybase-(2*seg-1),x+h-2,ybase-(2*seg-1),fg);
	x += h+seg-1;
    }
    if ( mask&ksm_control ) {
	int half = h/2;
	int top = h-1, midy = top-half;
	GPoint pts[3];
	GDrawSetLineWidth(m->w,seg-1);
	pts[0].x = x;          pts[0].y = ybase-midy;
	pts[1].x = x+half;     pts[1].y = ybase-top;
	pts[2].x = x+2*half;   pts[2].y = ybase-midy;
	GDrawDrawPoly(m->w,pts,3,fg);
	GDrawSetLineWidth(m->w,0);
	x += h+seg-1;
    }
    if ( mask&ksm_meta ) {
	int off = (seg-1)/2;
	GDrawSetLineWidth(m->w,seg-1);
	GDrawDrawLine(m->w,x,ybase-off,x+seg+1,ybase-off,fg);
	GDrawDrawLine(m->w,x+seg+1,ybase-off,x+2*seg+1,ybase-(h-off-1),fg);
	GDrawDrawLine(m->w,x+2*seg+1,ybase-(h-off-1),x+4*seg-1,ybase-(h-off-1),fg);
	GDrawDrawLine(m->w,x+2*seg+1,ybase-off,x+4*seg-1,ybase-off,fg);
	GDrawSetLineWidth(m->w,0);
	x += h+2*seg-1;
    }
    if ( mask&ksm_shift ) {
	int half = h/2;
	int top = h-1, midy = top-half;
	GDrawDrawLine(m->w,x,ybase-midy,x+half,ybase-top,fg);
	GDrawDrawLine(m->w,x+2*half,ybase-midy,x+half,ybase-top,fg);
	GDrawDrawLine(m->w,x,ybase-midy,x+seg-1,ybase-midy,fg);
	GDrawDrawLine(m->w,x+2*half,ybase-midy,x+2*half-(seg-1),ybase-midy,fg);
	GDrawDrawLine(m->w,x+seg-1,ybase-midy,x+seg-1,ybase,fg);
	GDrawDrawLine(m->w,x+2*half-(seg-1),ybase-midy,x+2*half-(seg-1),ybase,fg);
	GDrawDrawLine(m->w,x+seg-1,ybase,x+2*half-(seg-1),ybase,fg);
	x += h+seg-1;
    }
return( x );
}

static int GMenuMacIconsWidth(struct gmenu *m, int mask ) {
    int h = 3*(m->as/3);
    int seg = h/3;
    int x=0;

    if ( mask&ksm_cmdmacosx ) {
	x += h+seg-1;
    }
    if ( mask&ksm_shift ) {
	x += h+seg-1;
    }
    if ( mask&ksm_control ) {
	x += h+seg-1;
    }
    if ( mask&ksm_meta ) {
	x += h+2*seg-1;
    }
return( x );
}
	
static void GMenuDrawCheckMark(struct gmenu *m, Color fg, int ybase, int r2l) {
    int as = m->as;
    int pt = GDrawPointsToPixels(m->w,1);
    int x = r2l ? m->width-m->tioff+2*pt : m->tickoff;

    while ( pt>1 && 2*pt>=as/3 ) --pt;
    GDrawSetLineWidth(m->w,pt);
    GDrawDrawLine(m->w,x+2*pt,ybase-as/3,x+as/3,ybase-2*pt,fg);
    GDrawDrawLine(m->w,x+2*pt,ybase-as/3-pt,x+as/3,ybase-3*pt,fg);
    GDrawDrawLine(m->w,x+as/3,ybase-2*pt,x+as/3+as/5,ybase-2*pt-as/4,fg);
    GDrawDrawLine(m->w,x+as/3+as/5,ybase-2*pt-as/4,x+as/3+2*as/5,ybase-2*pt-as/3-as/7,fg);
    GDrawDrawLine(m->w,x+as/3+2*as/5,ybase-2*pt-as/3-as/7,x+as/3+3*as/5,ybase-2*pt-as/3-as/7-as/8,fg);
}

static void GMenuDrawUncheckMark(struct gmenu *m, Color fg, int ybase, int r2l) {
}

static void GMenuDrawArrow(struct gmenu *m, int ybase, int r2l) {
    int pt = GDrawPointsToPixels(m->w,1);
    int as = 2*(m->as/2);
    int x = r2l ? m->bp+2*pt : m->rightedge-2*pt;
    GPoint p[3];

    GDrawSetLineWidth(m->w,pt);
    if ( r2l ) {
	p[0].x = x;			p[0].y = ybase-as/2;
	p[1].x = x+3*(as/2);		p[1].y = ybase;
	p[2].x = p[1].x;		p[2].y = ybase-as;

	GDrawDrawLine(m->w,p[0].x,p[0].y,p[2].x,p[2].y,m->box->border_brighter);
	GDrawDrawLine(m->w,p[0].x+pt,p[0].y,p[2].x+pt,p[2].y+pt,m->box->border_brighter);
	GDrawDrawLine(m->w,p[2].x,p[2].y,p[1].x,p[1].y,m->box->border_brightest);
	GDrawDrawLine(m->w,p[2].x-pt,p[2].y+pt,p[1].x+pt,p[1].y-pt,m->box->border_brightest);
	GDrawDrawLine(m->w,p[1].x,p[1].y,p[0].x,p[0].y,m->box->border_darkest);
	GDrawDrawLine(m->w,p[1].x+pt,p[1].y-pt,p[0].x-pt,p[0].y,m->box->border_darkest);
    } else {
	p[0].x = x;			p[0].y = ybase-as/2;
	p[1].x = x-3*(as/2);		p[1].y = ybase;
	p[2].x = p[1].x;		p[2].y = ybase-as;

	GDrawDrawLine(m->w,p[0].x,p[0].y,p[2].x,p[2].y,m->box->border_brighter);
	GDrawDrawLine(m->w,p[0].x-pt,p[0].y,p[2].x+pt,p[2].y+pt,m->box->border_brighter);
	GDrawDrawLine(m->w,p[2].x,p[2].y,p[1].x,p[1].y,m->box->border_brightest);
	GDrawDrawLine(m->w,p[2].x+pt,p[2].y+pt,p[1].x+pt,p[1].y-pt,m->box->border_brightest);
	GDrawDrawLine(m->w,p[1].x,p[1].y,p[0].x,p[0].y,m->box->border_darkest);
	GDrawDrawLine(m->w,p[1].x+pt,p[1].y-pt,p[0].x-pt,p[0].y,m->box->border_darkest);
    }
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
    unichar_t shortbuf[300];
    int as = GTextInfoGetAs(m->w,&mi->ti,m->font);
    int h, width;
    Color fg = m->box->main_foreground;
    GRect old, new;
    int ybase = y+as;
    int r2l = false;
    int x;

    new.x = m->tickoff; new.width = m->rightedge-m->tickoff;
    new.y = y; new.height = GTextInfoGetHeight(m->w,&mi->ti,m->font);
    GDrawPushClip(m->w,&new,&old);

    if ( mi->ti.fg!=COLOR_DEFAULT && mi->ti.fg!=COLOR_UNKNOWN )
	fg = mi->ti.fg;
    if ( mi->ti.disabled || m->disabled )
	fg = m->box->disabled_foreground;
    if ( fg==COLOR_DEFAULT )
	fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(m->w));
    if ( mi->ti.text!=NULL && isrighttoleft(mi->ti.text[0]) )
	r2l = true;

    if ( r2l )
	x = m->width-m->tioff-GTextInfoGetWidth(m->w,&mi->ti,m->font);
    else
	x = m->tioff;
    h = GTextInfoDraw(m->w,x,y,&mi->ti,m->font,
	    (mi->ti.disabled || m->disabled )?m->box->disabled_foreground:fg,
	    m->box->active_border,new.y+new.height);

    if ( mi->ti.checkable ) {
	if ( mi->ti.checked )
	    GMenuDrawCheckMark(m,fg,ybase,r2l);
	else
	    GMenuDrawUncheckMark(m,fg,ybase,r2l);
    }

    if ( mi->sub!=NULL )
	GMenuDrawArrow(m,ybase,r2l);
    else if ( mi->shortcut!=0 && (mi->short_mask&0xffe0)==0 && mac_menu_icons ) {
	_shorttext(mi->shortcut,0,shortbuf);
	width = GDrawGetBiTextWidth(m->w,shortbuf,-1,-1,NULL) + GMenuMacIconsWidth(m,mi->short_mask);
	if ( r2l ) {
	    int x = GDrawDrawBiText(m->w,m->bp,ybase,shortbuf,-1,NULL,fg);
	    GMenuDrawMacIcons(m,fg,ybase, x, mi->short_mask);
	} else {
	    int x = GMenuDrawMacIcons(m,fg,ybase,m->rightedge-width, mi->short_mask);
	    GDrawDrawBiText(m->w,x,ybase,shortbuf,-1,NULL,fg);
	}
    } else if ( mi->shortcut!=0 ) {
	shorttext(mi,shortbuf);

	width = GDrawGetBiTextWidth(m->w,shortbuf,-1,-1,NULL);
	if ( r2l )
	    GDrawDrawBiText(m->w,m->bp,ybase,shortbuf,-1,NULL,fg);
	else
	    GDrawDrawBiText(m->w,m->rightedge-width,ybase,shortbuf,-1,NULL,fg);
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
	else if ( m->lcnt!=m->mcnt && i==m->lcnt+m->offtop-1 ) {
	    GMenuDrawDownArrow(m, m->bp+(i-m->offtop)*m->fh+m->as);
    break;	/* Otherwise we get bits of the line after the last */
	} else
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

static void _GMenuDestroy(struct gmenu *m) {
    if ( m->dying )
return;
    m->dying = true;
    if ( m->line_with_mouse!=-1 )
	m->mi[m->line_with_mouse].ti.selected = false;
    if ( m->child!=NULL )
	_GMenuDestroy(m->child);
    if ( m->parent!=NULL )
	m->parent->child = NULL;
    else if ( m->menubar!=NULL ) {
	m->menubar->child = NULL;
	m->menubar->pressed = false;
	_GWidget_ClearPopupOwner((GGadget *) (m->menubar));
	_GWidget_ClearGrabGadget((GGadget *) (m->menubar));
	GMenuBarChangeSelection(m->menubar,-1,NULL);
    }
    GDrawDestroyWindow(m->w);
    /* data are freed when we get the destroy event !!!! */
}

static void GMenuDestroy(struct gmenu *m) {
    GDrawPointerUngrab(GDrawGetDisplayOfWindow(m->w));
    if ( menu_grabs && m->parent!=NULL )
	GDrawPointerGrab(m->parent->w);
    _GMenuDestroy(m);
}

static void GMenuHideAll(struct gmenu *m) {
    if ( m!=NULL ) {
	struct gmenu *s = m;
	GDrawPointerUngrab(GDrawGetDisplayOfWindow(m->w));
	while ( m->parent ) m = m->parent;
	while ( m ) {
	    m->hidden = true;
	    GDrawSetVisible(m->w,false);
	    m=m->child;
	}
	GDrawSync(GDrawGetDisplayOfWindow(s->w));
	GDrawProcessPendingEvents(GDrawGetDisplayOfWindow(s->w));
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

static void UnsetInitialPress(struct gmenu *m) {
    while ( m!=NULL ) {
	m->initial_press = false;
	if ( m->menubar!=NULL ) m->menubar->initial_press = false;
	m = m->parent;
    }
}

static void GMenuChangeSelection(struct gmenu *m, int newsel,GEvent *event) {
    int old=m->line_with_mouse;

    if ( old==newsel )
return;
    if ( newsel==m->mcnt )
return;

    if ( m->child!=NULL ) {
	GMenuDestroy(m->child);
	m->child = NULL;
    }
    UnsetInitialPress(m);
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
	    m->child = GMenuCreateSubMenu(m,m->mi[newsel].sub,
		    m->disabled || m->mi[newsel].ti.disabled);
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
	    mb->child = GMenuCreatePulldownMenu(mb,mi->sub,mi->ti.disabled);
    }
}

static int MParentInitialPress(struct gmenu *m) {
    if ( m->parent!=NULL )
return( m->parent->initial_press );
    else if ( m->menubar!=NULL )
return( m->menubar->initial_press );

return( false );
}

static int gmenu_mouse(struct gmenu *m, GEvent *event) {
    GPoint p;
    struct gmenu *testm;

    if ( m->hidden || (m->child!=NULL && m->child->hidden))
return( true );

    if ( event->type == et_crossing ) {
	if ( !event->u.crossing.entered )
	    UnsetInitialPress(m);
return( true );
    }

    p.x = event->u.mouse.x; p.y = event->u.mouse.y;

    for ( testm=m; testm->child!=NULL; testm = testm->child );
    if ( testm->scrollit )
	GDrawCancelTimer(testm->scrollit); testm->scrollit = NULL;
    for ( ; testm!=NULL; testm=testm->parent )
	if ( GDrawEventInWindow(testm->w,event) )
    break;

    if ( testm!=m && testm!=NULL ) {
	GDrawPointerGrab(testm->w);
	GDrawTranslateCoordinates(m->w,testm->w,&p);
	m = testm;
    } else if ( testm==NULL /*&& event->u.mouse.y<0*/ ) {/* menubars can be below the menu if no room on screen */
	for ( testm=m; testm->parent!=NULL; testm=testm->parent );
	if ( testm->menubar!=NULL ) {
	    GDrawTranslateCoordinates(m->w,testm->menubar->g.base,&p);
	    if ( p.x>=0 && p.y>=0 &&
		    p.x<testm->menubar->g.inner.x+testm->menubar->g.inner.width &&
		    p.y<testm->menubar->g.inner.y+testm->menubar->g.inner.height ) {
		/*GDrawPointerGrab(testm->menubar->g.base);*/	/* Don't do this */
		event->u.mouse.x = p.x; event->u.mouse.y = p.y;
return( (GDrawGetEH(testm->menubar->g.base))(testm->menubar->g.base,event));
	    }
	}
	testm = NULL;
    }
    if ( testm==NULL ) {
	if ( event->type==et_mousedown )
	    GMenuDismissAll(m);
	else if ( event->type==et_mouseup )
	    GMenuSetPressed(m,false);
	else if ( m->pressed )
	    GMenuChangeSelection(m,-1,event);
return( true );
    }

    event->u.mouse.x = p.x; event->u.mouse.y = p.y; event->w = m->w;
    if (( m->pressed && event->type==et_mousemove ) ||
	    event->type == et_mousedown ) {
	int l = (event->u.mouse.y-m->bp)/m->fh;
	int i = l + m->offtop;
	if ( event->u.mouse.y<m->bp && event->type==et_mousedown )
	    GMenuDismissAll(m);
	else if ( l==0 && m->offtop!=0 ) {
	    GMenuChangeSelection(m,-1,event);
	    m->scrollit = GDrawRequestTimer(m->w,_GScrollBar_RepeatTime/2,_GScrollBar_RepeatTime/2,m);
	    m->scrollup = true;
	} else if ( l>=m->lcnt-1 && m->offtop+m->lcnt<m->mcnt ) {
	    GMenuChangeSelection(m,-1,event);
	    m->scrollit = GDrawRequestTimer(m->w,_GScrollBar_RepeatTime/2,_GScrollBar_RepeatTime/2,m);
	    m->scrollup = false;
	} else if ( event->type == et_mousedown && m->child!=NULL &&
		i == m->line_with_mouse ) {
	    GMenuChangeSelection(m,-1,event);
	} else
	    GMenuChangeSelection(m,i,event);
	if ( event->type == et_mousedown ) {
	    GMenuSetPressed(m,true);
	    if ( m->child!=NULL )
		m->initial_press = true;
	}
    } else if ( event->type == et_mouseup && m->child==NULL ) {
#if 0
 printf("\nActivate menu\n");
#endif
	if ( event->u.mouse.y>=m->bp && event->u.mouse.x>=0 &&
		event->u.mouse.y<m->height-m->bp &&
		event->u.mouse.x < m->width &&
		!MParentInitialPress(m)) {
	    int l = (event->u.mouse.y-m->bp)/m->fh;
	    int i = l + m->offtop;
	    if ( !( l==0 && m->offtop!=0 ) &&
		    !( l==m->lcnt-1 && m->offtop+m->lcnt<m->mcnt ) &&
		    !m->disabled &&
		    !m->mi[i].ti.disabled && !m->mi[i].ti.line ) {
		if ( m->mi[i].ti.checkable )
		    m->mi[i].ti.checked = !m->mi[i].ti.checked;
		GMenuHideAll(m);
		GMenuDismissAll(m);
		if ( m->mi[i].invoke!=NULL )
		    (m->mi[i].invoke)(m->owner,&m->mi[i],event);
	    }
	}
    } else if ( event->type == et_mouseup ) {
	UnsetInitialPress(m);
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

#if 0
static int getkey(int keysym, int option) {
    if ( option && keysym>128 ) {
	/* Under Mac 10.5 (but not under 10.4,3,2,1,0) if the option key is */
	/*  depressed the keysym changes. Opt-A becomes ARing, etc. */
	/* Er... I can't repeat this now. the Option modifier mask (meta) */
	/*  should be stripped off before we call Xutf8LookupString so the */
	/*  conversion shouldn't happen. And doesn't in my tests */
	/* And 0x2000 is not set for option now anyway */
	if ( keysym==0xc5 )
	    keysym = 'a';
	else if ( keysym==0xe5 )
	    keysym = 'A';
	else if ( keysym==0x8bf )
	    keysym = 'b';
	else if ( keysym==0x2b9 )
	    keysym = 'B';
	else if ( keysym==0xe7 )
	    keysym = 'c';
	else if ( keysym==0xc7 )
	    keysym = 'C';
	else if ( keysym==0x8ef )
	    keysym = 'd';
	else if ( keysym==0xce )
	    keysym = 'D';
	else if ( keysym==0xfe51 )
	    keysym = 'e';
	else if ( keysym==0xb4 )
	    keysym = 'E';
	else if ( keysym==0x8f6 )
	    keysym = 'f';
	else if ( keysym==0xcf )
	    keysym = 'F';
	else if ( keysym==0xa9 )
	    keysym = 'g';
	else if ( keysym==0x1bd )
	    keysym = 'G';
	else if ( keysym==0x1ff )
	    keysym = 'h';
	else if ( keysym==0xd3 )
	    keysym = 'H';
	else if ( keysym==0xfe52 )
	    keysym = 'i';
	else if ( keysym==0x2c6 )
	    keysym = 'I';
	else if ( keysym==0x2206 )
	    keysym = 'j';
	else if ( keysym==0xd4 )
	    keysym = 'J';
	else if ( keysym==0x2da )
	    keysym = 'k';
	else if ( keysym==0xf8ff )
	    keysym = 'K';
	else if ( keysym==0xac )
	    keysym = 'l';
	else if ( keysym==0xd2 )
	    keysym = 'L';
	else if ( keysym==0xb5 )
	    keysym = 'm';
	else if ( keysym==0xc2 )
	    keysym = 'M';
	else if ( keysym==0xfe53 )
	    keysym = 'n';
	else if ( keysym==0x2dc )
	    keysym = 'N';
	else if ( keysym==0xf8 )
	    keysym = 'o';
	else if ( keysym==0xd8 )
	    keysym = 'O';
	else if ( keysym==0x7f0 )
	    keysym = 'p';
	else if ( keysym==0x220f )
	    keysym = 'P';
	else if ( keysym==0x13bd )
	    keysym = 'q';
	else if ( keysym==0x13bc )
	    keysym = 'Q';
	else if ( keysym==0xae )
	    keysym = 'r';
	else if ( keysym==0x2030 )
	    keysym = 'R';
	else if ( keysym==0xdf )
	    keysym = 's';
	else if ( keysym==0xcd )
	    keysym = 'S';
	else if ( keysym==0xaf1 )
	    keysym = 't';
	else if ( keysym==0x1b7 )
	    keysym = 'T';
	else if ( keysym==0xfe57 )
	    keysym = 'u';
	else if ( keysym==0xa8 )
	    keysym = 'U';
	else if ( keysym==0x8d6 )
	    keysym = 'v';
	else if ( keysym==0x25ca )
	    keysym = 'V';
	else if ( keysym==0x2211 )
	    keysym = 'w';
	else if ( keysym==0xafe )
	    keysym = 'W';
	else if ( keysym==0x2248 )
	    keysym = 'x';
	else if ( keysym==0x1b2 )
	    keysym = 'X';
	else if ( keysym==0xa5 )
	    keysym = 'y';
	else if ( keysym==0xc1 )
	    keysym = 'Y';
	else if ( keysym==0x7d9 )
	    keysym = 'z';
	else if ( keysym==0xb8 )
	    keysym = 'Z';
	else if ( keysym==0xaaa )
	    keysym = '-';
	else if ( keysym==0xaa9 )
	    keysym = '_';
	else if ( keysym==0x8bd )
	    keysym = '=';
	else if ( keysym==0xb1 )
	    keysym = '+';
	else if ( keysym==0xad2 )
	    keysym = '[';
	else if ( keysym==0xad0 )
	    keysym = ']';
	else if ( keysym==0xad3 )
	    keysym = '{';
	else if ( keysym==0xad1 )
	    keysym = '}';
    }
    if ( islower(keysym)) keysym = toupper(keysym);
return( keysym );
}
#endif

static GMenuItem *GMenuSearchShortcut(GMenuItem *mi, GEvent *event) {
    int i;
    unichar_t keysym = event->u.chr.keysym;

    if ( keysym<GK_Special && islower(keysym))
	keysym = toupper(keysym); /*getkey(keysym,event->u.chr.state&0x2000 );*/
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	if ( mi[i].sub==NULL && mi[i].shortcut == keysym &&
		(menumask&event->u.chr.state)==mi[i].short_mask )
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
	if ( m->line_with_mouse==-1 ) {
	    int ns=0;
	    while ( ns<m->mcnt && (m->mi[ns].ti.disabled || m->mi[ns].ti.line)) ++ns;
	    if ( ns<m->mcnt )
		GMenuChangeSelection(m,ns,event);
	} else if ( m->mi[m->line_with_mouse].sub!=NULL &&
		m->child==NULL ) {
	    m->child = GMenuCreateSubMenu(m,m->mi[m->line_with_mouse].sub,
		    m->disabled || m->mi[m->line_with_mouse].ti.disabled);
	} else {
	    int i = m->line_with_mouse;
	    if ( !m->disabled && !m->mi[i].ti.disabled && !m->mi[i].ti.line ) {
		if ( m->mi[i].ti.checkable )
		    m->mi[i].ti.checked = !m->mi[i].ti.checked;
		GMenuDismissAll(m);
		if ( m->mi[i].invoke!=NULL )
		    (m->mi[i].invoke)(m->owner,&m->mi[i],event);
	    } else
		GMenuDismissAll(m);
	}
return( true );
      case GK_Left: case GK_KP_Left:
	if ( m->parent!=NULL ) {
	    GMenuDestroy(m);
return( true );
	} else if ( m->menubar!=NULL ) {
	    GMenuBar *mb = m->menubar;
	    int en = mb->entry_with_mouse;
	    int lastmi = mb->fake[0].sub!=NULL ? mb->lastmi+1 : mb->lastmi;
	    if ( en>0 ) {
		GMenuBarChangeSelection(mb,en-1,event);
	    } else
		GMenuBarChangeSelection(mb,lastmi-1,event);
return( true );
	}
	/* Else fall into the "Up" case */
      case GK_Up: case GK_KP_Up: case GK_Page_Up: case GK_KP_Page_Up: {
	int ns;
	if ( keysym!=GK_Left && keysym!=GK_KP_Left ) {
	    while ( m->line_with_mouse==-1 && m->parent!=NULL ) {
		GMenu *p = m->parent;
		GMenuDestroy(m);
		m = p;
	    }
	}
	ns = m->line_with_mouse-1;
	while ( ns>=0 && (m->mi[ns].ti.disabled || m->mi[ns].ti.line)) --ns;
	if ( ns<0 ) {
	    ns = m->mcnt-1;
	    while ( ns>=0 && (m->mi[ns].ti.disabled || m->mi[ns].ti.line)) --ns;
	}
	if ( ns<0 && m->line_with_mouse==-1 ) {	/* Nothing selectable? get rid of menu */
	    GMenuDestroy(m);
return( true );
	}
	if ( ns<0 ) ns = -1;
	GMenuChangeSelection(m,ns,NULL);
return( true );
      }
      case GK_Right: case GK_KP_Right:
	if ( m->line_with_mouse!=-1 &&
		m->mi[m->line_with_mouse].sub!=NULL && m->child==NULL ) {
	    m->child = GMenuCreateSubMenu(m,m->mi[m->line_with_mouse].sub,
		    m->disabled || m->mi[m->line_with_mouse].ti.disabled);
return( true );
	} else if ( m->parent==NULL && m->menubar!=NULL ) {
	    GMenuBar *mb = m->menubar;
	    int en = mb->entry_with_mouse;
	    int lastmi = mb->fake[0].sub!=NULL ? mb->lastmi+1 : mb->lastmi;
	    if ( en+1<lastmi ) {
		GMenuBarChangeSelection(mb,en+1,event);
	    } else
		GMenuBarChangeSelection(mb,0,event);
return( true );
	}
      /* Fall through into the "Down" case */
      case GK_Down: case GK_KP_Down: case GK_Page_Down: case GK_KP_Page_Down: {
	int ns;
	if ( keysym!=GK_Right && keysym!=GK_KP_Right ) {
	    while ( m->line_with_mouse==-1 && m->parent!=NULL ) {
		GMenu *p = m->parent;
		GMenuDestroy(m);
		m = p;
	    }
	}
	ns = m->line_with_mouse+1;
	while ( ns<m->mcnt && (m->mi[ns].ti.disabled || m->mi[ns].ti.line)) ++ns;
	if ( ns>=m->mcnt ) {
	    ns = 0;
	    while ( ns<m->mcnt && (m->mi[ns].ti.disabled || m->mi[ns].ti.line)) ++ns;
	}
	if ( ns>=m->mcnt && m->line_with_mouse==-1 ) {	/* Nothing selectable? get rid of menu */
	    GMenuDestroy(m);
return( true );
	}
	GMenuChangeSelection(m,ns,event);
return( true );
      }
      case GK_Home: case GK_KP_Home: {
	int ns=0;
	while ( ns<m->mcnt && (m->mi[ns].ti.disabled || m->mi[ns].ti.line)) ++ns;
	if ( ns!=m->mcnt )
	    GMenuChangeSelection(m,ns,event);
return( true );
      }
      case GK_End: case GK_KP_End: {
	int ns=m->mcnt-1;
	while ( ns>=0 && (m->mi[ns].ti.disabled || m->mi[ns].ti.line)) --ns;
	if ( ns>=0 )
	    GMenuChangeSelection(m,ns,event);
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
    if ( event->u.chr.state&ksm_meta && !(event->u.chr.state&(menumask&~(ksm_meta|ksm_shift)))) {
	/* Only look for mneumonics in the child */
	while ( m->child!=NULL )
	    m = m->child;
	for ( i=0; i<m->mcnt; ++i ) {
	    if ( m->mi[i].ti.mnemonic == keysym &&
			!m->disabled &&
			!m->mi[i].ti.disabled ) {
		GMenuKeyInvoke(m,i);
return( true );
	    }
	}
    }

    /* then look for shortcuts everywhere */
    if ( (event->u.chr.state&(menumask&~ksm_shift)) ||
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
    if ( most_recent_popup_menu==m )
	most_recent_popup_menu = NULL;
    if ( m->donecallback )
	(m->donecallback)(m->owner);
    if ( m->freemi )
	GMenuItemArrayFree(m->mi);
    free(m);
return( true );
}

static int gmenu_eh(GWindow w,GEvent *ge) {
    GMenu *m = (GMenu *) GDrawGetUserData(w);

    switch ( ge->type ) {
      case et_map:
	/* I need to initialize the input context, but I can't do that until */
	/*  the menu pops up */
	if ( ge->u.map.is_visible && m->gic!=NULL )
	    GDrawSetGIC(w,m->gic,0,20);
return( true );
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

static GMenu *_GMenu_Create(GWindow owner,GMenuItem *mi, GPoint *where,
	int awidth, int aheight, GFont *font, int disable) {
    GMenu *m = gcalloc(1,sizeof(GMenu));
    GRect pos;
    GDisplay *disp = GDrawGetDisplayOfWindow(owner);
    GWindowAttrs pattrs;
    int i, width, keywidth;
    unichar_t buffer[300];
    int ds, ld, temp, lh;
    GRect screen;

    m->owner = owner;
    m->mi = mi;
    m->disabled = disable;
    m->font = font;
    m->box = &menu_box;
    m->tickoff = m->tioff = m->bp = GBoxBorderWidth(owner,m->box);
    m->line_with_mouse = -1;

/* Mnemonics in menus don't work under gnome. Turning off nodecor makes them */
/*  work, but that seems a high price to pay */
    pattrs.mask = wam_events|wam_nodecor|wam_positioned|wam_cursor|wam_transient|wam_verytransient;
    pattrs.event_masks = -1;
    pattrs.nodecoration = true;
    pattrs.positioned = true;
    pattrs.cursor = ct_pointer;
    pattrs.transient = GWidgetGetTopWidget(owner);

    pos.x = pos.y = 0; pos.width = pos.height = 100;

    m->w = GDrawCreateTopWindow(disp,&pos,gmenu_eh,m,&pattrs);
    m->gic = GDrawCreateInputContext(m->w,gic_root|gic_orlesser);
    GDrawWindowFontMetrics(m->w,m->font,&m->as, &ds, &ld);
    m->fh = m->as + ds + 1;		/* I need some extra space, else mneumonic underlines look bad */
    lh = m->fh;

    GDrawSetFont(m->w,m->font);
    m->hasticks = false; width = 0; keywidth = 0;
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	if ( mi[i].ti.checkable ) m->hasticks = true;
	temp = GTextInfoGetWidth(m->w,&mi[i].ti,m->font);
	if ( temp>width ) width = temp;
	if ( mi[i].shortcut!=0 && (mi[i].short_mask&0xffe0)==0 && mac_menu_icons ) {
	    _shorttext(mi[i].shortcut,0,buffer);
	    temp = GDrawGetBiTextWidth(m->w,buffer,-1,-1,NULL) + GMenuMacIconsWidth(m,mi[i].short_mask);
	} else {
	    shorttext(&mi[i],buffer);
	    temp = GDrawGetBiTextWidth(m->w,buffer,-1,-1,NULL);
	}
	if ( temp>keywidth ) keywidth=temp;
	if ( mi[i].sub!=NULL && 3*m->as>keywidth )
	    keywidth = 3*m->as;
	temp = GTextInfoGetHeight(m->w,&mi[i].ti,m->font);
	if ( temp>lh ) {
	    if ( temp>3*m->fh/2 )
		temp = 3*m->fh/2;
	    lh = temp;
	}
    }
    m->fh = lh;
    m->mcnt = m->lcnt = i;
    if ( keywidth!=0 ) width += keywidth + GDrawPointsToPixels(m->w,8);
    if ( m->hasticks ) {
	int ticklen = m->as + GDrawPointsToPixels(m->w,5);
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
    if ( pos.y + pos.height > screen.height ) {
	if ( where->y+aheight-pos.height >= 0 )
	    pos.y = where->y+aheight-pos.height;
	else {
	    pos.y = 0;
	    /* Ok, it's going to overlap the press point if we got here */
	    /*  let's see if we can shift it left/right a bit so it won't */
	    if ( awidth<0 )
		/* Oh, well, I guess it won't. It's a submenu and we've already */
		/*  moved off to the left */;
	    else if ( pos.x+awidth+pos.width+3<screen.width )
		pos.x += awidth+3;
	    else if ( pos.x-pos.width-3>=0 )
		pos.x -= pos.width+3;
	    else
		/* There doesn't seem much we can do in this case */;
	}
    }
    if ( pos.x+pos.width > screen.width ) {
	if ( where->x+awidth-pos.width >= 0 )
	    pos.x = where->x+awidth-pos.width;
	else
	    pos.x = 0;
    }
    GDrawResize(m->w,pos.width,pos.height);
    GDrawMove(m->w,pos.x,pos.y);

    GDrawSetVisible(m->w,true);
    if ( menu_grabs )
	GDrawPointerGrab(m->w);
return( m );
}

static GMenu *GMenuCreateSubMenu(GMenu *parent,GMenuItem *mi,int disable) {
    GPoint p;
    GMenu *m;

    p.x = parent->width;
    p.y = (parent->line_with_mouse-parent->offtop)*parent->fh + parent->bp;
    GDrawTranslateCoordinates(parent->w,GDrawGetRoot(GDrawGetDisplayOfWindow(parent->w)),&p);
    m = _GMenu_Create(parent->owner,mi,&p,-parent->width,parent->fh,
	    parent->font, disable);
    m->parent = parent;
    m->pressed = parent->pressed;
return( m );
}

static GMenu *GMenuCreatePulldownMenu(GMenuBar *mb,GMenuItem *mi,int disabled) {
    GPoint p;
    GMenu *m;

    p.x = mb->g.inner.x + mb->xs[mb->entry_with_mouse]-
	    GBoxDrawnWidth(mb->g.base,&menu_box );
    p.y = mb->g.r.y + mb->g.r.height;
    GDrawTranslateCoordinates(mb->g.base,GDrawGetRoot(GDrawGetDisplayOfWindow(mb->g.base)),&p);
    m = _GMenu_Create(mb->g.base,mi,&p,
	    mb->xs[mb->entry_with_mouse+1]-mb->xs[mb->entry_with_mouse],
	    -mb->g.r.height,mb->font, disabled);
    m->menubar = mb;
    m->pressed = mb->pressed;
    _GWidget_SetPopupOwner((GGadget *) mb);
return( m );
}

GWindow _GMenuCreatePopupMenu(GWindow owner,GEvent *event, GMenuItem *mi,
	void (*donecallback)(GWindow) ) {
    GPoint p;
    GMenu *m;
    GEvent e;

    if ( !gmenubar_inited )
	GMenuInit();

    p.x = event->u.mouse.x;
    p.y = event->u.mouse.y;
    GDrawTranslateCoordinates(owner,GDrawGetRoot(GDrawGetDisplayOfWindow(owner)),&p);
    m = _GMenu_Create(owner,GMenuItemArrayCopy(mi,NULL),&p,0,0,menu_font,false);
    GDrawPointerUngrab(GDrawGetDisplayOfWindow(owner));
    GDrawPointerGrab(m->w);
    GDrawGetPointerPosition(m->w,&e);
    if ( e.u.mouse.state & (ksm_button1|ksm_button2|ksm_button3) )
	m->pressed = m->initial_press = true;
    m->donecallback = donecallback;
    m->freemi = true;
    most_recent_popup_menu = m;
return( m->w );
}

GWindow GMenuCreatePopupMenu(GWindow owner,GEvent *event, GMenuItem *mi) {
return( _GMenuCreatePopupMenu(owner,event,mi,NULL));
}

int GMenuPopupCheckKey(GEvent *event) {

    if ( most_recent_popup_menu==NULL )
return( false );

return( gmenu_key(most_recent_popup_menu,event));
}

/* ************************************************************************** */
int GMenuBarCheckKey(GGadget *g, GEvent *event) {
    int i;
    GMenuBar *mb = (GMenuBar *) g;
    GMenuItem *mi;
    unichar_t keysym = event->u.chr.keysym;

    if ( g==NULL )
return( false );

    if ( keysym<GK_Special && islower(keysym))
	keysym = toupper(keysym);
    if ( event->u.chr.state&ksm_meta && !(event->u.chr.state&(menumask&~(ksm_meta|ksm_shift)))) {
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
    if ( event->u.chr.state&(menumask&~ksm_shift) ||
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
    Color fg = g->state==gs_disabled?g->box->disabled_foreground:
		    g->box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
		    g->box->main_foreground;
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

    if ( mb->child!=NULL && mb->child->hidden )
return( true );

    if ( event->type == et_mousedown ) {
	mb->pressed = true;
	if ( mb->child!=NULL )
	    GMenuSetPressed(mb->child,true);
	which = GMenuBarIndex(mb,event->u.mouse.x);
	if ( which==mb->entry_with_mouse && mb->child!=NULL )
	    GMenuDestroy(mb->child);
	else {
	    mb->initial_press = true;
	    GMenuBarChangeSelection(mb,which,event);
	}
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
	mb->initial_press = mb->pressed = false;
	if ( mb->child!=NULL )
	    GMenuSetPressed(mb->child,false);
    }
return( true );
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
	GDrawWindowFontMetrics(mb->g.base,mb->font,&as, &ds, &ld);
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
	mb->xs[i+1] = mb->xs[i]+wid+GTextInfoGetWidth(mb->g.base,&mb->mi[i].ti,NULL);
#else
    for ( i=wid=0; i<mb->mtot; ++i ) {
	temp = GDrawGetBiTextWidth(mb->g.base,mb->mi[i].ti.text,-1,-1,NULL);
	if ( temp>wid ) wid = temp;
    }
    wid += GDrawPointsToPixels(mb->g.base,5);
    mb->xs[0] = 0;
    for ( i=0; i<mb->mtot; ++i )
	mb->xs[i+1] = mb->xs[i]+wid;
#endif
    GMenuBarTestSize(mb);
}

static void MenuMaskInit(GMenuItem *mi) {
    int mask = GMenuItemArrayMask(mi);
    if ( mask_set )
	menumask |= mask;
    else {
	menumask = mask;
	mask_set = true;
    }
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
    mb->font = menubar_font;

    GMenuBarFit(mb,gd);
    GMenuBarFindXs(mb);

    MenuMaskInit(mb->mi);

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&mb->g);
    _GWidget_SetMenuBar(&mb->g);

    mb->g.takes_input = true;
return( &mb->g );
}

GGadget *GMenu2BarCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GMenuBar *mb = gcalloc(1,sizeof(GMenuBar));

    if ( !gmenubar_inited )
	GMenuInit();
    mb->g.funcs = &gmenubar_funcs;
    _GGadget_Create(&mb->g,base,gd,data,&menubar_box);

    mb->mi = GMenuItem2ArrayCopy(gd->u.menu2,&mb->mtot);
    mb->xs = galloc((mb->mtot+1)*sizeof(uint16));
    mb->entry_with_mouse = -1;
    mb->font = menubar_font;

    GMenuBarFit(mb,gd);
    GMenuBarFindXs(mb);

    MenuMaskInit(mb->mi);

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

/* Check to see if event matches the given shortcut, expressed in our standard*/
/*  syntax and subject to gettext translation */
int GMenuIsCommand(GEvent *event,char *shortcut) {
    GMenuItem foo;
    unichar_t keysym = event->u.chr.keysym;

    if ( event->type!=et_char )
return( false );

    if ( keysym<GK_Special && islower(keysym))
	keysym = toupper(keysym);

    memset(&foo,0,sizeof(foo));
    
    GMenuItemParseShortCut(&foo,shortcut);

return( (menumask&event->u.chr.state)==foo.short_mask && foo.shortcut == keysym );
}

int GMenuMask(void) {
return( menumask );
}

GResInfo *_GMenuRIHead(void) {
    if ( !gmenubar_inited )
	GMenuInit();
return( &gmenubar_ri );
}
