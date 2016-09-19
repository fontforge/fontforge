/* Copyright (C) 2000-2012 by George Williams */
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
#include "hotkeys.h"
#include "gutils/prefs.h"

static GBox menubar_box = GBOX_EMPTY; /* Don't initialize here */
static GBox menu_box = GBOX_EMPTY; /* Don't initialize here */
static FontInstance *menu_font = NULL, *menubar_font = NULL;
static int gmenubar_inited = false;
#ifdef __Mac
static int mac_menu_icons = true;
#else
static int mac_menu_icons = false;
#endif
static int menu_3d_look = 1; // The 3D look is the default/legacy setting.
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
    omf_border_shape|omf_border_width|box_foreground_border_outer,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static struct resed menu_re[] = {
    {N_("MacIcons"), "MacIcons", rt_bool, &mac_menu_icons, N_("Whether to use mac-like icons to indicate modifiers (for instance ^ for Control)\nor to use an abbreviation (for instance \"Cnt-\")"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
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
    omf_border_shape|omf_padding|box_foreground_border_outer,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

char* HKTextInfoToUntranslatedText( char* text_untranslated );
char* HKTextInfoToUntranslatedTextFromTextInfo( GTextInfo* ti );

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
    menu_font = menubar_font = GDrawInstanciateFont(NULL,&rq);
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
    menu_3d_look = GResourceFindBool("GMenu.3DLook", menu_3d_look);
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
    unsigned int any_unmasked_shortcuts: 1;		/* Only set for popup menus. Else info in menubar */
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
    GTimer *scrollit;		/* No longer in use, see comment below */
    FontInstance *font;
    void (*donecallback)(GWindow owner);
    GIC *gic;
    char subMenuName[100];
    /* The code below still contains the old code for using up/down arrows */
    /*  in the menu instead of a scrollbar. If vsb==NULL and the menu doesn't */
    /*  fit on the screen, then the old code will be implemented (normally */
    /*  that won't happen, but if I ever want to re-enable it, this is how */
    /* The up arrow was placed at the top of the menu IFF there were lines off */
    /*  the top, similarly for the bottom. Clicking on either one would scroll */
    /*  in the indicated directions */
    GGadget *vsb;
} GMenu;

static char*
translate_shortcut (int i, char *modifier)
{
  char buffer[32];
  char *temp;
  TRACE("translate_shortcut(top) i:%d modifier:%s\n", i, modifier );

  sprintf (buffer, "Flag0x%02x", 1 << i);
  temp = dgettext (GMenuGetShortcutDomain (), buffer);

  if (strcmp (temp, buffer) != 0)
      modifier = temp;
  else
      modifier = dgettext (GMenuGetShortcutDomain (), modifier);

  TRACE("translate_shortcut(end) i:%d modifier:%s\n", i, modifier );

  return modifier;
}



static void _shorttext(int shortcut, int short_mask, unichar_t *buf) {
    unichar_t *pt = buf;
    static int initted = false;
    struct { int mask; char *modifier; } mods[8] = {
	{ ksm_shift, H_("Shift+") },
	{ ksm_capslock, H_("CapsLk+") },
	{ ksm_control, H_("Ctrl+") },
	{ ksm_meta, H_("Alt+") },
	{ 0x10, H_("Flag0x10+") },
	{ 0x20, H_("Flag0x20+") },
	{ 0x40, H_("Flag0x40+") },
	{ 0x80, H_("Flag0x80+") }
	};
    int i;
    char buffer[32];

    uc_strcpy(pt,"xx⎇");
    pt += u_strlen(pt);
    *pt = '\0';
    return;

    if ( !initted )
    {
	/* char *temp; */
	for ( i=0; i<8; ++i )
	{
	    /* sprintf( buffer,"Flag0x%02x", 1<<i ); */
	    /* temp = dgettext(GMenuGetShortcutDomain(),buffer); */
	    /* if ( strcmp(temp,buffer)!=0 ) */
	    /* 	mods[i].modifier = temp; */
	    /* else */
	    /* 	mods[i].modifier = dgettext(GMenuGetShortcutDomain(),mods[i].modifier); */

          if (mac_menu_icons)
	  {
	      TRACE("mods[i].mask: %s\n", mods[i].modifier );

              if (mods[i].mask == ksm_cmdmacosx)
		  mods[i].modifier = "⌘";
              else if (mods[i].mask == ksm_control)
		  mods[i].modifier = "⌃";
              else if (mods[i].mask == ksm_meta)
		  mods[i].modifier = "⎇";
              else if (mods[i].mask == ksm_shift)
		  mods[i].modifier = "⇧";
              else
		  mods[i].modifier = translate_shortcut (i, mods[i].modifier);
	  }
	  else
	  {
              translate_shortcut (i, mods[i].modifier);
	  }




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


/*
 * Unused
static void shorttext(GMenuItem *gi,unichar_t *buf) {
    _shorttext(gi->shortcut,gi->short_mask,buf);
}
*/

static int GMenuGetMenuPathRecurse( GMenuItem** stack,
				    GMenuItem *basemi,
				    GMenuItem *targetmi ) {
    GMenuItem *mi = basemi;
    int i;
    for ( i=0; mi[i].ti.text || mi[i].ti.text_untranslated || mi[i].ti.image || mi[i].ti.line; ++i ) {
//	TRACE("text_untranslated: %s\n", mi[i].ti.text_untranslated );
	if ( mi[i].sub ) {
//	    TRACE("GMenuGetMenuPathRecurse() going down on %s\n", u_to_c(mi[i].ti.text));
	    stack[0] = &mi[i];
	    int rc = GMenuGetMenuPathRecurse( &stack[1], mi[i].sub, targetmi );
	    if( rc == 1 )
		return rc;
	    stack[0] = 0;
	}
	if( mi[i].ti.text ) {
//	    TRACE("GMenuGetMenuPathRecurse() inspecting %s %p %p\n", u_to_c(mi[i].ti.text),mi[i],targetmi);
	    GMenuItem* cur = &mi[i];
	    if( cur == targetmi ) {
		stack[0] = cur;
		return 1;
	    }
	}
    }
    return 0;
}

/**
 * Get a string describing the path from basemi to the targetmi.
 * For example, if the basemi is the first mi in the whole menu structure
 * and targetmi is to the edit / select all menu item then the return
 * value is:
 * edit.select all
 *
 * If you can't get to targetmi from basemi then return 0.
 * If something doesn't make sense then return 0.
 *
 * Do not free the return value, it is a pointer into a buffer owned
 * by this function.
 */
static char* GMenuGetMenuPath( GMenuItem *basemi, GMenuItem *targetmi ) {
    GMenuItem* stack[1024];
    memset(stack, 0, sizeof(stack));
    if( !targetmi->ti.text )
	return 0;

//    TRACE("GMenuGetMenuPath() base   %s\n", u_to_c(basemi->ti.text));
//    TRACE("GMenuGetMenuPath() target %s\n", u_to_c(targetmi->ti.text));

    /* { */
    /* 	int i=0; */
    /* 	GMenuItem *mi = basemi; */
    /* 	for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) { */
    /* 	    if( mi[i].ti.text ) { */
    /* 		TRACE("GMenuGetMenuPath() xbase   %s\n", u_to_c(mi[i].ti.text)); */
    /* 	    } */
    /* 	} */
    /* } */
    /* TRACE("GMenuGetMenuPath() starting...\n"); */

    GMenuItem *mi = basemi;
    int i;
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	if( mi[i].ti.text ) {
	    memset(stack, 0, sizeof(stack));
//	    TRACE("GMenuGetMenuPath() xbase   %s\n", u_to_c(mi[i].ti.text));
//	    TRACE("GMenuGetMenuPath() untrans %s\n", mi[i].ti.text_untranslated);
	    GMenuGetMenuPathRecurse( stack, &mi[i], targetmi );
	    if( stack[0] != 0 ) {
//		TRACE("GMenuGetMenuPath() have stack[0]...\n");
		break;
	    }
	}
    }
    if( stack[0] == 0 ) {
	return 0;
    }

    static char buffer[PATH_MAX];
    buffer[0] = '\0';
    for( i=0; stack[i]; i++ ) {
	if( i )
	    strcat(buffer,".");
	if( stack[i]->ti.text_untranslated )
	{
//	    TRACE("adding %s\n", HKTextInfoToUntranslatedTextFromTextInfo( &stack[i]->ti ));
	    strcat( buffer, HKTextInfoToUntranslatedTextFromTextInfo( &stack[i]->ti ));
	}
	else if( stack[i]->ti.text )
	{
	    cu_strcat(buffer,stack[i]->ti.text);
	}
//	TRACE("GMenuGetMenuPath() stack at i:%d  mi %s\n", i, u_to_c(stack[i]->ti.text));
    }
    return buffer;
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
	p[1].x = x+1*(as/2);		p[1].y = ybase;
	p[2].x = p[1].x;		p[2].y = ybase-as;

	// If rendering menus in standard (3-dimensional) look, use the shadow colors for fake relief.
	// Otherwise, use foreground colors.
	if (menu_3d_look) {
		GDrawDrawLine(m->w,p[0].x,p[0].y,p[2].x,p[2].y,m->box->border_brighter);
		GDrawDrawLine(m->w,p[0].x+pt,p[0].y,p[2].x+pt,p[2].y+pt,m->box->border_brighter);
		GDrawDrawLine(m->w,p[1].x,p[1].y,p[0].x,p[0].y,m->box->border_darkest);
		GDrawDrawLine(m->w,p[1].x+pt,p[1].y-pt,p[0].x-pt,p[0].y,m->box->border_darkest);
	} else {
		GDrawDrawLine(m->w,p[0].x,p[0].y,p[2].x,p[2].y,m->box->main_foreground);
		GDrawDrawLine(m->w,p[0].x+pt,p[0].y,p[2].x+pt,p[2].y+pt,m->box->main_foreground);
		GDrawDrawLine(m->w,p[1].x,p[1].y,p[0].x,p[0].y,m->box->main_foreground);
		GDrawDrawLine(m->w,p[1].x+pt,p[1].y-pt,p[0].x-pt,p[0].y,m->box->main_foreground);
	}
    } else {
	p[0].x = x;			p[0].y = ybase-as/2;
	p[1].x = x-1*(as/2);		p[1].y = ybase;
	p[2].x = p[1].x;		p[2].y = ybase-as;

	if (menu_3d_look) {
		GDrawDrawLine(m->w,p[0].x,p[0].y,p[2].x,p[2].y,m->box->border_brighter);
		GDrawDrawLine(m->w,p[0].x-pt,p[0].y,p[2].x+pt,p[2].y+pt,m->box->border_brighter);
		GDrawDrawLine(m->w,p[1].x,p[1].y,p[0].x,p[0].y,m->box->border_darkest);
		GDrawDrawLine(m->w,p[1].x+pt,p[1].y-pt,p[0].x-pt,p[0].y,m->box->border_darkest);
	} else {
		GDrawDrawLine(m->w,p[0].x,p[0].y,p[2].x,p[2].y,m->box->main_foreground);
		GDrawDrawLine(m->w,p[0].x-pt,p[0].y,p[2].x+pt,p[2].y+pt,m->box->main_foreground);
		GDrawDrawLine(m->w,p[1].x,p[1].y,p[0].x,p[0].y,m->box->main_foreground);
		GDrawDrawLine(m->w,p[1].x+pt,p[1].y-pt,p[0].x-pt,p[0].y,m->box->main_foreground);
	}
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

    // If rendering menus in standard (3-dimensional) look, use the shadow colors for fake relief.
    // Otherwise, use foreground colors.
    if (menu_3d_look) {
        GDrawDrawLine(m->w,p[0].x,p[0].y,p[1].x,p[1].y,m->box->border_brightest);
        GDrawDrawLine(m->w,p[0].x,p[0].y+pt,p[1].x+pt,p[1].y,m->box->border_brightest);
        GDrawDrawLine(m->w,p[1].x,p[1].y,p[2].x,p[2].y,m->box->border_darker);
        GDrawDrawLine(m->w,p[1].x+pt,p[1].y,p[2].x-pt,p[2].y,m->box->border_darker);
        GDrawDrawLine(m->w,p[2].x,p[2].y,p[0].x,p[0].y,m->box->border_darkest);
        GDrawDrawLine(m->w,p[2].x-pt,p[2].y,p[0].x,p[0].y+pt,m->box->border_darkest);
    } else {
        GDrawDrawLine(m->w,p[0].x,p[0].y,p[1].x,p[1].y,m->box->main_foreground);
        GDrawDrawLine(m->w,p[0].x,p[0].y+pt,p[1].x+pt,p[1].y,m->box->main_foreground);
        GDrawDrawLine(m->w,p[1].x,p[1].y,p[2].x,p[2].y,m->box->main_foreground);
        GDrawDrawLine(m->w,p[1].x+pt,p[1].y,p[2].x-pt,p[2].y,m->box->main_foreground);
        GDrawDrawLine(m->w,p[2].x,p[2].y,p[0].x,p[0].y,m->box->main_foreground);
        GDrawDrawLine(m->w,p[2].x-pt,p[2].y,p[0].x,p[0].y+pt,m->box->main_foreground);
    }
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

    // If rendering menus in standard (3-dimensional) look, use the shadow colors for fake relief.
    // Otherwise, use foreground colors.
    if (menu_3d_look) {
        GDrawDrawLine(m->w,p[0].x,p[0].y,p[1].x,p[1].y,m->box->border_darker);
        GDrawDrawLine(m->w,p[0].x,p[0].y+pt,p[1].x+pt,p[1].y,m->box->border_darker);
        GDrawDrawLine(m->w,p[1].x,p[1].y,p[2].x,p[2].y,m->box->border_brightest);
        GDrawDrawLine(m->w,p[1].x+pt,p[1].y,p[2].x-pt,p[2].y,m->box->border_brightest);
        GDrawDrawLine(m->w,p[2].x,p[2].y,p[0].x,p[0].y,m->box->border_darkest);
        GDrawDrawLine(m->w,p[2].x-pt,p[2].y,p[0].x,p[0].y+pt,m->box->border_darkest);
    } else {
        GDrawDrawLine(m->w,p[0].x,p[0].y,p[1].x,p[1].y,m->box->main_foreground);
        GDrawDrawLine(m->w,p[0].x,p[0].y+pt,p[1].x+pt,p[1].y,m->box->main_foreground);
        GDrawDrawLine(m->w,p[1].x,p[1].y,p[2].x,p[2].y,m->box->main_foreground);
        GDrawDrawLine(m->w,p[1].x+pt,p[1].y,p[2].x-pt,p[2].y,m->box->main_foreground);
        GDrawDrawLine(m->w,p[2].x,p[2].y,p[0].x,p[0].y,m->box->main_foreground);
        GDrawDrawLine(m->w,p[2].x-pt,p[2].y,p[0].x,p[0].y+pt,m->box->main_foreground);
    }
}

/**
 * Return the menu bar at the top of this menu list
 * */
static GMenuBar * getTopLevelMenubar( struct gmenu *m ) {
    while( m->parent ) {
	m = m->parent;
    }
    return m->menubar;
}


static int GMenuDrawMenuLine(struct gmenu *m, GMenuItem *mi, int y,GWindow pixmap) {
    unichar_t shortbuf[300];
    int as = GTextInfoGetAs(m->w,&mi->ti,m->font);
    int h, width;
    Color fg = m->box->main_foreground;
    GRect old, new;
    int ybase = y+as;
    int r2l = false;
    int x;

    //printf("GMenuDrawMenuLine(top)\n");
    /* if(mi->ti.text) */
    /* 	TRACE("GMenuDrawMenuLine() mi:%s\n",u_to_c(mi->ti.text)); */

    new.x = m->tickoff; new.width = m->rightedge-m->tickoff;
    new.y = y; new.height = GTextInfoGetHeight(pixmap,&mi->ti,m->font);
    GDrawPushClip(pixmap,&new,&old);

    if ( mi->ti.fg!=COLOR_DEFAULT && mi->ti.fg!=COLOR_UNKNOWN )
	fg = mi->ti.fg;
    if ( mi->ti.disabled || m->disabled )
	fg = m->box->disabled_foreground;
    if ( fg==COLOR_DEFAULT )
	fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap));
    if ( mi->ti.text!=NULL && isrighttoleft(mi->ti.text[0]) )
	r2l = true;

    if ( r2l )
	x = m->width-m->tioff-GTextInfoGetWidth(pixmap,&mi->ti,m->font);
    else
	x = m->tioff;
    h = GTextInfoDraw(pixmap,x,y,&mi->ti,m->font,
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
    else
    {
	_shorttext(mi->shortcut,0,shortbuf);
	uint16 short_mask = mi->short_mask;

	/* TRACE("m->menubar: %p\n", m->menubar ); */
	/* TRACE("m->parent: %p\n", m->parent ); */
	/* TRACE("m->toplevel: %p\n", getTopLevelMenubar(m)); */

	/**
	 * Grab the hotkey if there is one. First we work out the
	 * menubar for this menu item, and then get the path from the
	 * menubar to the menuitem and pass that to the hotkey system
	 * to get the hotkey if there is one for that menu item.
	 */
	GMenuBar* toplevel = getTopLevelMenubar(m);
	Hotkey* hk = 0;
	if( toplevel )
	{
	    hk = hotkeyFindByMenuPath( toplevel->g.base,
				       GMenuGetMenuPath( toplevel->mi, mi ));
	}
	else if( m->owner && strlen(m->subMenuName) )
	{
	    hk = hotkeyFindByMenuPathInSubMenu( m->owner, m->subMenuName,
						GMenuGetMenuPath( m->mi, mi ));
	}

	short_mask = 0;
	uc_strcpy(shortbuf,"");

	if( hk )
	{
	    /* TRACE("m->menubar->mi: %p\n", toplevel->mi ); */
	    /* TRACE("m->menubar->window: %p\n", toplevel->g.base ); */
	    /* TRACE("drawline... hk: %p\n", hk ); */
	    short_mask = hk->state;
	    char* keydesc = hk->text;
	    if( mac_menu_icons )
	    {
		keydesc = hotkeyTextToMacModifiers( keydesc );
	    }
	    utf82u_strcpy( shortbuf, keydesc );
	    if( keydesc != hk->text )
		free( keydesc );
	}

	width = GDrawGetTextWidth(pixmap,shortbuf,-1);
	if ( r2l )
	{
	    GDrawDrawText(pixmap,m->bp,ybase,shortbuf,-1,fg);
	}
	else
	{
	    int x = m->rightedge-width;
	    GDrawDrawText(pixmap,x,ybase,shortbuf,-1,fg);
	}
    }

    GDrawPopClip(pixmap,&old);
return( y + h );
}

static int gmenu_expose(struct gmenu *m, GEvent *event,GWindow pixmap) {
    GRect old1, old2;
    GRect r;
    int i;

    /* TRACE("gmenu_expose() m:%p ev:%p\n",m,event); */
    GDrawPushClip(pixmap,&event->u.expose.rect,&old1);
    r.x = 0; r.width = m->width; r.y = 0; r.height = m->height;
    GBoxDrawBackground(pixmap,&r,m->box,gs_active,false);
    GBoxDrawBorder(pixmap,&r,m->box,gs_active,false);
    r.x = m->tickoff; r.width = m->rightedge-m->tickoff;
    r.y = m->bp; r.height = m->height - 2*m->bp;
    GDrawPushClip(pixmap,&r,&old2);
    for ( i = event->u.expose.rect.y/m->fh+m->offtop; i<m->mcnt &&
	    i<=(event->u.expose.rect.y+event->u.expose.rect.height)/m->fh+m->offtop;
	    ++i ) {
	if ( i==m->offtop && m->offtop!=0 && m->vsb==NULL )
	    GMenuDrawUpArrow(m, m->bp+m->as);
	else if ( m->lcnt!=m->mcnt && i==m->lcnt+m->offtop-1 && i!=m->mcnt-1 ) {
	    if ( m->vsb==NULL )
		GMenuDrawDownArrow(m, m->bp+(i-m->offtop)*m->fh+m->as);
	    else
		GMenuDrawMenuLine(m, &m->mi[i], m->bp+(i-m->offtop)*m->fh, pixmap);
    break;	/* Otherwise we get bits of the line after the last */
	} else
	    GMenuDrawMenuLine(m, &m->mi[i], m->bp+(i-m->offtop)*m->fh, pixmap);
    }
    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
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
	GMenuDrawMenuLine(m, &m->mi[ln], m->bp+(ln-m->offtop)*m->fh,m->w);
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
    } else if ((event->type==et_mouseup || event->type==et_mousedown) &&
               (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
        //From scrollbar.c: X treats a scroll as a mousedown/mouseup event
        return GGadgetDispatchEvent(m->vsb,event);
    }

    p.x = event->u.mouse.x; p.y = event->u.mouse.y;

    for ( testm=m; testm->child!=NULL; testm = testm->child );
    if ( testm->scrollit && testm!=m ) {
	GDrawCancelTimer(testm->scrollit);
	testm->scrollit = NULL;
    }
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
	if ( m->scrollit!=NULL )
	    ;
	else if ( event->u.mouse.y<m->bp && event->type==et_mousedown )
	    GMenuDismissAll(m);
	else if ( l==0 && m->offtop!=0 && m->vsb==NULL ) {
	    GMenuChangeSelection(m,-1,event);
	    if ( m->scrollit==NULL )
		m->scrollit = GDrawRequestTimer(m->w,1,_GScrollBar_RepeatTime,m);
	    m->scrollup = true;
	} else if ( l>=m->lcnt-1 && m->offtop+m->lcnt<m->mcnt && m->vsb==NULL ) {
	    GMenuChangeSelection(m,-1,event);
	    if ( m->scrollit==NULL )
		m->scrollit = GDrawRequestTimer(m->w,1,_GScrollBar_RepeatTime,m);
	    m->scrollup = false;
	} else if ( event->type == et_mousedown && m->child!=NULL &&
		i == m->line_with_mouse ) {
	    GMenuChangeSelection(m,-1,event);
	} else if ( i >= m->mcnt ){
	    GMenuChangeSelection(m,-1,event);
	} else
	    GMenuChangeSelection(m,i,event);
	if ( event->type == et_mousedown ) {
	    GMenuSetPressed(m,true);
	    if ( m->child!=NULL )
		m->initial_press = true;
	}
    } else if ( event->type == et_mouseup && m->child==NULL ) {
	if ( m->scrollit!=NULL ) {
	    GDrawCancelTimer(m->scrollit);
	    m->scrollit = NULL;
	} else if ( event->u.mouse.y>=m->bp && event->u.mouse.x>=0 &&
		event->u.mouse.y<m->height-m->bp &&
		event->u.mouse.x < m->width &&
		!MParentInitialPress(m)) {
	    int l = (event->u.mouse.y-m->bp)/m->fh;
	    int i = l + m->offtop;
	    if ( !( l==0 && m->offtop!=0 && m->vsb==NULL ) &&
		    !( l==m->lcnt-1 && m->offtop+m->lcnt<m->mcnt && m->vsb==NULL ) &&
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
	if ( m->offtop==0 )
return(true);
	if ( --m->offtop<0 ) m->offtop = 0;
    } else {
	if ( m->offtop == m->mcnt-m->lcnt )
return( true );
	++m->offtop;
	if ( m->offtop + m->lcnt > m->mcnt )
	    m->offtop = m->mcnt-m->lcnt;
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

/**
 * Remove any instances of 'ch' from 'ret' and return 'ret'
 *
 * When ch is encountered it is removed from the string, with all the
 * characters following it moved left to cover over the space ch used
 * to occupy.
 */
static char* str_remove_all_single_char( char * ret, char ch )
{
    char* src = ret;
    char* dst = ret;
    for( ; *src; src++ )
    {
	if( *src == ch )
	    continue;

	*dst = *src;
	dst++;
    }
    *dst = '\0';
    return ret;
}

/**
 * Given a text_untranslated which may be either the hotkey of format:
 *
 * Foo|ShortCutKey
 * Win*Foo|ShortCutKey
 * _Foo
 *
 * return just the english text for the entry, ie, Foo
 *
 * The return value is owned by this function, do not free it.
 */
char* HKTextInfoToUntranslatedText(char *text_untranslated) {
    char ret[PATH_MAX+1];
    char* pt;
    int i;

    strncpy(ret,text_untranslated,PATH_MAX);

    if( (pt=strchr(ret,'*')) )
        for (i=0; pt[i]!='\0'; i++)
            ret[i]=pt[i+1];
    if( (pt=strchr(ret,'|')) )
	*pt = '\0';
    str_remove_all_single_char( ret, '_' );
    return copy(ret);
}

/**
 * Call HKTextInfoToUntranslatedText on ti->text_untranslated
 * guarding against the chance of null for ti, and ti->text_untranslated
 */
char* HKTextInfoToUntranslatedTextFromTextInfo( GTextInfo* ti )
{
    if( !ti )
	return 0;
    if( !ti->text_untranslated )
	return 0;
    return HKTextInfoToUntranslatedText( ti->text_untranslated );
}


/**
 * return true if the prefix matches the first segment of the given action.
 * For example,
 * return will be 0 when action = foo.bar.baz prefix = bar
 * return will be 0 when action = foo.bar.baz prefix = fooo
 * return will be 1 when action = foo.bar.baz prefix = foo
 * return will be 1 when action = baz prefix = baz
 */
static int HKActionMatchesFirstPartOf( char* action, char* prefix_const, int munge )
{
    char prefix[PATH_MAX+1];
    char* pt = 0;
    strncpy( prefix, prefix_const, PATH_MAX );
    if( munge )
	strncpy( prefix, HKTextInfoToUntranslatedText( prefix_const ),PATH_MAX );
//    TRACE("munge:%d prefix2:%s\n", munge, prefix );

    pt = strchr(action,'.');
    if( !pt )
	return( 0==strcmp(action,prefix) );

    int l = pt - action;
    if( strlen(prefix) < l )
	return 0;
    int rc = strncmp( action, prefix, l );
    return rc == 0;
}

/**
 * Call HKActionMatchesFirstPartOf on the given ti.
 */
static int HKActionMatchesFirstPartOfTextInfo( char* action, GTextInfo* ti )
{
    if( ti->text_untranslated )
	return HKActionMatchesFirstPartOf( action, ti->text_untranslated, 1 );
    if( ti->text )
	return HKActionMatchesFirstPartOf( action, u_to_c(ti->text), 0 );
    return 0;
}

static char* HKActionPointerPastLeftmostKey( char* action ) {
    char* pt = strchr(action,'.');
    if( !pt )
	return 0;
    return pt + 1;
}



static GMenuItem *GMenuSearchActionRecursive( GWindow gw,
					      GMenuItem *mi,
					      char* action,
					      GEvent *event,
					      int call_moveto) {

//    TRACE("GMenuSearchAction() action:%s\n", action );
    int i;
    for ( i=0; mi[i].ti.text || mi[i].ti.text_untranslated || mi[i].ti.image || mi[i].ti.line; ++i ) {
//	if( mi[i].ti.text )
//	    TRACE("GMenuSearchActionRecursive() text             : %s\n", u_to_c(mi[i].ti.text) );
//	TRACE("GMenuSearchActionRecursive() text_untranslated: %s\n", mi[i].ti.text_untranslated );
	if ( call_moveto && mi[i].moveto != NULL)
	    (mi[i].moveto)(gw,&(mi[i]),event);

	if ( mi[i].sub ) {
	    if( HKActionMatchesFirstPartOfTextInfo( action, &mi[i].ti )) {
		char* subaction = HKActionPointerPastLeftmostKey(action);

//		TRACE("GMenuSearchAction() action:%s decending menu:%s\n", action, u_to_c(mi[i].ti.text) );
		GMenuItem *ret = GMenuSearchActionRecursive(gw,mi[i].sub,subaction,event,call_moveto);
		if ( ret!=NULL )
		    return( ret );
	    }
	} else {
//	    TRACE("GMenuSearchAction() action:%s testing menu:%s\n", action, u_to_c(mi[i].ti.text) );
	    if( HKActionMatchesFirstPartOfTextInfo( action, &mi[i].ti )) {
//		TRACE("GMenuSearchAction() matching final menu part! action:%s\n", action );
		return &mi[i];
	    }
	}

    }
return( NULL );
}
static GMenuItem *GMenuSearchAction( GWindow gw,
				     GMenuItem *mi,
				     char* action,
				     GEvent *event,
				     int call_moveto) {
    char* windowType = GDrawGetWindowTypeName( gw );
    if( !windowType )
	return 0;
//    TRACE("GMenuSearchAction() windowtype:%s\n", windowType );
    int actionlen = strlen(action);
    int prefixlen = strlen(windowType) + 1 + strlen("Menu.");
    if( actionlen < prefixlen ) {
	return 0;
    }
    action += prefixlen;
    GMenuItem * ret = GMenuSearchActionRecursive( gw, mi, action,
						  event, call_moveto );
    return ret;
}



static GMenuItem *GMenuSearchShortcut(GWindow gw, GMenuItem *mi, GEvent *event,
	int call_moveto) {
    int i;
    unichar_t keysym = event->u.chr.keysym;

    if ( keysym<GK_Special && islower(keysym))
	keysym = toupper(keysym); /*getkey(keysym,event->u.chr.state&0x2000 );*/
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	if ( call_moveto && mi[i].moveto != NULL)
	    (mi[i].moveto)(gw,&(mi[i]),event);
	if ( mi[i].sub==NULL && mi[i].shortcut == keysym &&
		(menumask&event->u.chr.state)==mi[i].short_mask )
return( &mi[i]);
	else if ( mi[i].sub!=NULL ) {
	    GMenuItem *ret = GMenuSearchShortcut(gw,mi[i].sub,event,call_moveto);
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
	    mi = GMenuSearchShortcut(top->owner,top->menubar->mi,event,false);
	else
	    mi = GMenuSearchShortcut(top->owner,top->mi,event,false);
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
return( gmenu_expose(m,ge,w));
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

static int gmenu_scroll(GGadget *g, GEvent *event) {
    enum sb sbt = event->u.control.u.sb.type;
    GMenu *m = (GMenu *) (g->data);
    int newpos = m->offtop;

    if ( sbt==et_sb_top )
	newpos = 0;
    else if ( sbt==et_sb_bottom )
	newpos = m->mcnt-m->lcnt;
    else if ( sbt==et_sb_up ) {
	--newpos;
    } else if ( sbt==et_sb_down ) {
	++newpos;
    } else if ( sbt==et_sb_uppage ) {
	if ( m->lcnt!=1 )		/* Normally we leave one line in window from before, except if only one line fits */
	    newpos -= m->lcnt-1;
	else
	    newpos -= 1;
    } else if ( sbt==et_sb_downpage ) {
	if ( m->lcnt!=1 )		/* Normally we leave one line in window from before, except if only one line fits */
	    newpos += m->lcnt-1;
	else
	    newpos += 1;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	newpos = event->u.control.u.sb.pos;
    }
    if ( newpos+m->lcnt > m->mcnt )
	newpos = m->mcnt-m->lcnt;
    if ( newpos<0 )
	newpos = 0;
    if ( newpos!= m->offtop ) {
	m->offtop = newpos;
	GScrollBarSetPos(m->vsb,newpos);
	GDrawRequestExpose(m->w,NULL,false);
    }
return( true );
}

static GMenu *_GMenu_Create( GMenuBar* toplevel,
			     GWindow owner,
			     GMenuItem *mi,
			     GPoint *where,
			     int awidth, int aheight,
			     GFont *font, int disable,
			     char* subMenuName )
{
    GMenu *m = calloc(1,sizeof(GMenu));
    GRect pos;
    GDisplay *disp = GDrawGetDisplayOfWindow(owner);
    GWindowAttrs pattrs;
    int i, width, max_iwidth = 0, max_hkwidth = 0;
    unichar_t buffer[300];
    extern int _GScrollBar_Width;
    int ds, ld, temp, lh;
    int sbwidth = 0;
    GRect screen;

    m->owner = owner;
    m->mi = mi;
    m->disabled = disable;
    m->font = font;
    m->box = &menu_box;
    m->tickoff = m->tioff = m->bp = GBoxBorderWidth(owner,m->box);
    m->line_with_mouse = -1;
    if( subMenuName )
	strncpy(m->subMenuName,subMenuName,sizeof(m->subMenuName)-1);
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
    m->hasticks = false;
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	if ( mi[i].ti.checkable )
	    m->hasticks = true;
	temp = GTextInfoGetWidth(m->w,&mi[i].ti,m->font);
	if (temp > max_iwidth)
	    max_iwidth = temp;

	uc_strcpy(buffer,"");
	uint16 short_mask = 0;
	/**
	 * Grab the hotkey if there is one. First we work out the
	 * menubar for this menu item, and then get the path from the
	 * menubar to the menuitem and pass that to the hotkey system
	 * to get the hotkey if there is one for that menu item.
	 *
	 * If we have a hotkey then set the width to the text porition
	 * first and then add the modifier icons if there are to be
	 * any for the key combination.
	 */
//	TRACE("_gmenu_create() toplevel:%p owner:%p\n", toplevel, owner );

	Hotkey* hk = 0;
	if( toplevel ) {
	    hk = hotkeyFindByMenuPath( toplevel->g.base,
				       GMenuGetMenuPath( toplevel->mi, &mi[i] ));
	}
	else if( owner && strlen(m->subMenuName) )
	{
	    hk = hotkeyFindByMenuPathInSubMenu( owner, m->subMenuName,
						GMenuGetMenuPath( mi, &mi[i] ));
	}

//	TRACE("hk:%p\n", hk);
	if( hk )
	{
	    short_mask = hk->state;
	    char* keydesc = hk->text;
	    if( mac_menu_icons )
	    {
//		keydesc = hotkeyTextWithoutModifiers( keydesc );
	    }
	    uc_strcpy( buffer, keydesc );

	    temp = GDrawGetTextWidth(m->w,buffer,-1);
	    if (temp > max_hkwidth)
	        max_hkwidth = temp;
	    /* if( short_mask && mac_menu_icons ) { */
	    /* 	temp += GMenuMacIconsWidth( m, short_mask ); */
	    /* } */
	}

	temp = GTextInfoGetHeight(m->w,&mi[i].ti,m->font);
	if ( temp>lh ) {
	    if ( temp>3*m->fh/2 )
		temp = 3*m->fh/2;
	    lh = temp;
	}
    }
    m->fh = lh;
    m->mcnt = m->lcnt = i;

    //Width: Max item length + max length of hotkey text + padding
    width = max_iwidth + max_hkwidth + GDrawPointsToPixels(m->w, 10);
    if ( m->hasticks ) {
	int ticklen = m->as + GDrawPointsToPixels(m->w,5);
	width += ticklen;
	m->tioff += ticklen;
    }
    m->width = pos.width = width + 2*m->bp;
    m->rightedge = m->width - m->bp;
    m->height = pos.height = i*m->fh + 2*m->bp;
    GDrawGetSize(GDrawGetRoot(disp),&screen);

    sbwidth = 0;
/* On the mac, the menu bar takes up the top twenty pixels or so of screen */
/*  so never put a menu that high */
#define MAC_MENUBAR	20
    if ( pos.height > screen.height-MAC_MENUBAR-m->fh ) {
	GGadgetData gd;

	m->lcnt = (screen.height-MAC_MENUBAR-m->fh-2*m->bp)/m->fh;
	m->height = pos.height = m->lcnt*m->fh + 2*m->bp;

	/* It's too long, so add a scrollbar */
	sbwidth = GDrawPointsToPixels(owner,_GScrollBar_Width);
	pos.width += sbwidth;
	memset(&gd,'\0',sizeof(gd));
	gd.pos.y = 0; gd.pos.height = pos.height;
	gd.pos.width = sbwidth;
	gd.pos.x = m->width;
	gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert|gg_pos_use0;
	gd.handle_controlevent = gmenu_scroll;
	m->vsb = GScrollBarCreate(m->w,&gd,m);
	GScrollBarSetBounds(m->vsb,0,m->mcnt,m->lcnt);
    }

    pos.x = where->x; pos.y = where->y;
    if ( pos.y + pos.height > screen.height-MAC_MENUBAR ) {
	if ( where->y+aheight-pos.height >= MAC_MENUBAR )
	    pos.y = where->y+aheight-pos.height;
	else {
	    pos.y = MAC_MENUBAR;
	    /* Ok, it's going to overlap the press point if we got here */
	    /*  let's see if we can shift it left/right a bit so it won't */
	    if ( awidth<0 )
		/* Oh, well, I guess it won't. It's a submenu and we've already */
		/*  moved off to the left */;
	    else if ( pos.x+awidth+pos.width+3<screen.width )
		pos.x += awidth+3;
	    else if ( pos.x-pos.width-3>=0 )
		pos.x -= pos.width+3;
	    else {
		/* There doesn't seem much we can do in this case */
		;
            }
	}
    }
    if ( pos.x+pos.width > screen.width ) {
	if ( where->x+awidth-pos.width >= 0 )
	    pos.x = where->x+awidth-pos.width-3;
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
    char *subMenuName = 0;

    p.x = parent->width;
    p.y = (parent->line_with_mouse-parent->offtop)*parent->fh + parent->bp;
    GDrawTranslateCoordinates(parent->w,GDrawGetRoot(GDrawGetDisplayOfWindow(parent->w)),&p);
    m = _GMenu_Create(getTopLevelMenubar(parent),parent->owner,mi,&p,-parent->width,parent->fh,
		      parent->font, disable, subMenuName );
    m->parent = parent;
    m->pressed = parent->pressed;
return( m );
}

static GMenu *GMenuCreatePulldownMenu(GMenuBar *mb,GMenuItem *mi,int disabled) {
    GPoint p;
    GMenu *m;
    char *subMenuName = 0;

    p.x = mb->g.inner.x + mb->xs[mb->entry_with_mouse]-
	    GBoxDrawnWidth(mb->g.base,&menu_box );
    p.y = mb->g.r.y + mb->g.r.height;
    GDrawTranslateCoordinates(mb->g.base,GDrawGetRoot(GDrawGetDisplayOfWindow(mb->g.base)),&p);
    m = _GMenu_Create( mb, mb->g.base, mi, &p,
		       mb->xs[mb->entry_with_mouse+1]-mb->xs[mb->entry_with_mouse],
		       -mb->g.r.height,
		       mb->font, disabled, subMenuName );
    m->menubar = mb;
    m->pressed = mb->pressed;
    _GWidget_SetPopupOwner((GGadget *) mb);
return( m );
}

GWindow _GMenuCreatePopupMenu( GWindow owner,GEvent *event, GMenuItem *mi,
			       void (*donecallback)(GWindow) )
{
    char *subMenuName = 0;
    return _GMenuCreatePopupMenuWithName( owner, event, mi, subMenuName, donecallback );
}


GWindow _GMenuCreatePopupMenuWithName( GWindow owner,GEvent *event, GMenuItem *mi,
				       char *subMenuName,
				       void (*donecallback)(GWindow) )
{
    GPoint p;
    GMenu *m;
    GEvent e;

    if ( !gmenubar_inited )
	GMenuInit();

    p.x = event->u.mouse.x;
    p.y = event->u.mouse.y;
    GDrawTranslateCoordinates(owner,GDrawGetRoot(GDrawGetDisplayOfWindow(owner)),&p);
    m = _GMenu_Create( 0, owner, GMenuItemArrayCopy(mi,NULL), &p,
		       0, 0, menu_font,false, subMenuName );
    m->any_unmasked_shortcuts = GMenuItemArrayAnyUnmasked(m->mi);
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

GWindow GMenuCreatePopupMenu(GWindow owner,GEvent *event, GMenuItem *mi)
{
    char* subMenuName = 0;
    return( _GMenuCreatePopupMenuWithName(owner,event,mi,subMenuName,NULL));
}

GWindow GMenuCreatePopupMenuWithName(GWindow owner,GEvent *event,
				     char* subMenuName, GMenuItem *mi)
{
    return( _GMenuCreatePopupMenuWithName(owner,event,mi,subMenuName,NULL));
}


int GMenuPopupCheckKey(GEvent *event) {

    if ( most_recent_popup_menu==NULL ) return( false );

    return( gmenu_key(most_recent_popup_menu,event) );
}

/* ************************************************************************** */

int GGadgetUndoMacEnglishOptionCombinations(GEvent *event) {
    int keysym = event->u.chr.keysym;

    switch ( keysym ) {
    /* translate special mac keys to ordinary keys */
      case 0xba:
	keysym = '0'; break;
      case 0xa1:
	keysym = '1'; break;
      case 0x2122:
	keysym = '2'; break;
      case 0xa3:
	keysym = '3'; break;
      case 0xa2:
	keysym = '4'; break;
      case 0x221e:
	keysym = '5'; break;
      case 0xa7:
	keysym = '6'; break;
      case 0xb6:
	keysym = '7'; break;
      case 0x2022:
	keysym = '8'; break;
      case 0xaa:
	keysym = '9'; break;
      case 0xe5:
	keysym = 'a'; break;
      case 0x222b:
	keysym = 'b'; break;
      case 0xe7:
	keysym = 'c'; break;
      case 0x2202:
	keysym = 'd'; break;
      /* e is a modifier */
      case 0x192:
	keysym = 'f'; break;
      case 0xa9:
	keysym = 'g'; break;
      case 0x2d9:
	keysym = 'h'; break;
      /* i is a modifier */
      case 0x2206:
	keysym = 'j'; break;
      case 0x2da:
	keysym = 'k'; break;
      case 0xac:
	keysym = 'l'; break;
      case 0xb5:
	keysym = 'm'; break;
      /* n is a modifier */
      case 0xf8:
	keysym = 'o'; break;
      case 0x3c0:
	keysym = 'p'; break;
      case 0x153:
	keysym = 'q'; break;
      case 0xae:
	keysym = 'r'; break;
      case 0x2020:
	keysym = 's'; break;
      case 0xee:
	keysym = 't'; break;
      /* u is a modifier */
      case 0x221a:
	keysym = 'v'; break;
      case 0x2211:
	keysym = 'w'; break;
      case 0x2248:
	keysym = 'x'; break;
      case 0xa5:
	keysym = 'y'; break;
      case 0x3a9:
	keysym = 'z'; break;
    }
    return( keysym );
}

/**
 * On OSX the XEvents have some extra translation performed to try to be handier.
 * For example, in xev you might notice that alt+- gives a keysym of endash.
 *
 * Under Linux this translation doesn't happen and you get the alt
 * modifier and the minus keysym. The hotkey code is expecting
 * modifier(s) + base keysym not what osx gives (modifier(s) +
 * alternate-keysym). So this little function is designed to convert
 * the osx "enhanced" keysym back to their basic keysym.
 */
#ifdef __Mac
static int osx_handle_keysyms( int st, int k )
{
//    TRACE("osx_handle_keysyms() st:%d k:%d\n", st, k );

    if( (st & ksm_control) && (st & ksm_meta) )
	switch( k )
	{
	case 8211:  return 45; // Command + Alt + -
	case 8804:  return 44; // Command + Alt + ,
	}

    if( (st & ksm_control) && (st & ksm_meta) && (st & ksm_shift) )
	switch( k )
	{
	case 177:   return 43; // Command + Alt + Shift + =
	case 197:   return 65; // Command + Alt + Shift + A
	case 305:   return 66; // Command + Alt + Shift + B
	case 199:   return 67; // Command + Alt + Shift + C
	case 206:   return 68; // Command + Alt + Shift + D
	case 180:   return 69; // Command + Alt + Shift + E
	case 207:   return 70; // Command + Alt + Shift + F
	case 733:   return 71; // Command + Alt + Shift + G
	case 211:   return 72; // Command + Alt + Shift + H
	case 710:   return 73; // Command + Alt + Shift + I
	case 212:   return 74; // Command + Alt + Shift + J
	case 63743: return 75; // Command + Alt + Shift + K
	case 210:   return 76; // Command + Alt + Shift + L
	case 194:   return 77; // Command + Alt + Shift + M
	case 732:   return 78; // Command + Alt + Shift + N
	case 216:   return 79; // Command + Alt + Shift + O
	case 8719:  return 80; // Command + Alt + Shift + P
	case 65505: return 81; // Command + Alt + Shift + Q
	case 8240:  return 82; // Command + Alt + Shift + R
	case 205:   return 83; // Command + Alt + Shift + S
	case 711:   return 84; // Command + Alt + Shift + T
	case 168:   return 85; // Command + Alt + Shift + U
	case 9674:  return 86; // Command + Alt + Shift + V
	case 8222:  return 87; // Command + Alt + Shift + W
	case 731:   return 88; // Command + Alt + Shift + X
	case 193:   return 89; // Command + Alt + Shift + Y
	case 184:   return 90; // Command + Alt + Shift + Z

	case 8260:  return 33; // Command + Alt + Shift + 1
	case 8360:  return 64; // Command + Alt + Shift + 2
	case 8249:  return 35; // Command + Alt + Shift + 3
	case 8250:  return 36; // Command + Alt + Shift + 4
	case 64257: return 37; // Command + Alt + Shift + 5
	case 64258: return 94; // Command + Alt + Shift + 6
	case 8225:  return 38; // Command + Alt + Shift + 7
	case 176:   return 42; // Command + Alt + Shift + 8
	case 183:   return 40; // Command + Alt + Shift + 9
	case 8218:  return 41; // Command + Alt + Shift + 0
	}

    if( st & ksm_meta )
	switch( k )
	{
	case 2730:  return 45; // Alt + -
	case 2237:  return 61; // Alt + = (can avoid shift on this one for simpler up/down)
	case 8800:  return 61; // Alt + = (can avoid shift on this one for simpler up/down)
	}

    return k;
}
#endif

int osx_fontview_copy_cut_counter = 0;


static int GMenuBarCheckHotkey(GWindow top, GGadget *g, GEvent *event) {
//    TRACE("GMenuBarCheckKey(top) keysym:%d upper:%d lower:%d\n",keysym,toupper(keysym),tolower(keysym));
    // see if we should skip processing (e.g. no modifier key pressed)
    GMenuBar *mb = (GMenuBar *) g;
	GWindow w = GGadgetGetWindow(g);
	GGadget* focus = GWindowGetFocusGadgetOfWindow(w);
	if (GGadgetGetSkipHotkeyProcessing(focus))
	    return 0;

//    TRACE("GMenuBarCheckKey(2) keysym:%d upper:%d lower:%d\n",keysym,toupper(keysym),tolower(keysym));

    /* then look for hotkeys everywhere */

    if( hotkeySystemGetCanUseMacCommand() )
    {
	// If Mac command key was pressed, swap with the control key.
	if ((event->u.chr.state & (ksm_cmdmacosx|ksm_control)) == ksm_cmdmacosx) {
	    event->u.chr.state ^= (ksm_cmdmacosx|ksm_control);
	}
    }
#ifdef __Mac

    //
    // Command + Alt + Shift + F on OSX doesn't give the keysym one
    // might expect if they have been testing on a Linux Machine.
    //
    TRACE("looking2 for hotkey in new system...state:%d keysym:%d\n", event->u.chr.state, event->u.chr.keysym );
    TRACE("     has ksm_control:%d\n", (event->u.chr.state & ksm_control ));
    TRACE("     has ksm_cmdmacosx:%d\n", (event->u.chr.state & ksm_cmdmacosx ));
    TRACE("     has ksm_cmdmacosx|control:%d\n", ((event->u.chr.state & (ksm_cmdmacosx|ksm_control)) == ksm_cmdmacosx) );
    TRACE("     has ksm_meta:%d\n",    (event->u.chr.state & ksm_meta ));
    TRACE("     has ksm_shift:%d\n",   (event->u.chr.state & ksm_shift ));
    TRACE("     has ksm_option:%d\n",   (event->u.chr.state & ksm_option ));

    if( event->u.chr.state & ksm_option )
	event->u.chr.state ^= ksm_option;

    event->u.chr.keysym = osx_handle_keysyms( event->u.chr.state, event->u.chr.keysym );
    TRACE(" 3   has ksm_option:%d\n",   (event->u.chr.state & ksm_option ));

    // Command-c or Command-x
    if( event->u.chr.state == ksm_control
	&& (event->u.chr.keysym == 99 || event->u.chr.keysym == 120 ))
    {
	osx_fontview_copy_cut_counter++;
    }
#endif

//    TRACE("about to look for hotkey in new system...state:%d keysym:%d\n", event->u.chr.state, event->u.chr.keysym );
//    TRACE("     has ksm_control:%d\n", (event->u.chr.state & ksm_control ));
//    TRACE("     has ksm_meta:%d\n",    (event->u.chr.state & ksm_meta ));
//    TRACE("     has ksm_shift:%d\n",   (event->u.chr.state & ksm_shift ));

    event->u.chr.state |= ksm_numlock;
//    TRACE("about2 to look for hotkey in new system...state:%d keysym:%d\n", event->u.chr.state, event->u.chr.keysym );

    /**
     * Mask off the parts we don't explicitly care about
     */
    event->u.chr.state &= ( ksm_control | ksm_meta | ksm_shift | ksm_option );

//    TRACE("about3 to look for hotkey in new system...state:%d keysym:%d\n", event->u.chr.state, event->u.chr.keysym );
//    TRACE("     has ksm_control:%d\n", (event->u.chr.state & ksm_control ));
//    TRACE("     has ksm_meta:%d\n",    (event->u.chr.state & ksm_meta ));
//    TRACE("     has ksm_shift:%d\n",   (event->u.chr.state & ksm_shift ));

    if( GGadgetGetSkipUnQualifiedHotkeyProcessing(focus) && !event->u.chr.state )
    {
	TRACE("skipping unqualified hotkey for widget g:%p\n", g);
	return 0;
    }


    struct dlistnodeExternal* node= hotkeyFindAllByEvent( top, event );
    struct dlistnode* hklist = (struct dlistnode*)node;
    for( ; node; node=(struct dlistnodeExternal*)(node->next) ) {
	Hotkey* hk = (Hotkey*)node->ptr;
//	TRACE("hotkey found by event! hk:%p\n", hk );
//	TRACE("hotkey found by event! action:%s\n", hk->action );

	int skipkey = false;

	if( cv_auto_goto )
	{
	    if( !hk->state )
		skipkey = true;
//		TRACE("hotkey state:%d skip:%d\n", hk->state, skipkey );
	}

	if( !skipkey )
	{
	    GMenuItem *mi = GMenuSearchAction(mb->g.base,mb->mi,hk->action,event,mb->child==NULL);
	    if ( mi )
	    {
//		TRACE("GMenuBarCheckKey(x) have mi... :%p\n", mi );
//		TRACE("GMenuBarCheckKey(x) have mitext:%s\n", u_to_c(mi->ti.text) );
		if ( mi->ti.checkable && !mi->ti.disabled )
		    mi->ti.checked = !mi->ti.checked;
		if ( mi->invoke!=NULL && !mi->ti.disabled )
		    (mi->invoke)(mb->g.base,mi,NULL);
		if ( mb->child != NULL )
		    GMenuDestroy(mb->child);
		return( true );
	    }
	    else
	    {
//		    TRACE("hotkey found for event must be a non menu action... action:%s\n", hk->action );

	    }
	}

//	    TRACE("END hotkey found by event! hk:%p\n", hk );
    }
    dlist_free_external(&hklist);

//    TRACE("menubarcheckkey(e1)\n");
    return false;
}


int GMenuBarCheckKey(GWindow top, GGadget *g, GEvent *event) {
    int i;
    GMenuBar *mb = (GMenuBar *) g;
    unichar_t keysym = event->u.chr.keysym;

    if ( g==NULL || keysym==0 ) return( false ); /* exit if no gadget or key */

    if ( (menumask&ksm_cmdmacosx) && keysym>0x7f &&
	    (event->u.chr.state&ksm_meta) &&
	    !(event->u.chr.state&menumask&(ksm_control|ksm_cmdmacosx)) )
	keysym = GGadgetUndoMacEnglishOptionCombinations(event);

    if ( keysym<GK_Special && islower(keysym))
	keysym = toupper(keysym);
    if ( event->u.chr.state&ksm_meta && !(event->u.chr.state&(menumask&~(ksm_meta|ksm_shift)))) {
	/* Only look for mneumonics in the leaf of the displayed menu structure */
	if ( mb->child!=NULL )
	    return( gmenu_key(mb->child,event)); /* this routine will do shortcuts too */

	for ( i=0; i<mb->mtot; ++i ) {
	    if ( mb->mi[i].ti.mnemonic == keysym && !mb->mi[i].ti.disabled ) {
		GMenuBarKeyInvoke(mb,i);
		return( true );
	    }
	}
    }

    // See if it matches a hotkey
    if (GMenuBarCheckHotkey(top, g, event)) {
        return true;
    }

    if ( mb->child )
    {
	GMenu *m;
	for ( m=mb->child; m->child!=NULL; m = m->child )
	{
	}
	return( GMenuSpecialKeys(m,event->u.chr.keysym,event));
    }

//    TRACE("menubarcheckkey(e2)\n");
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

    // If rendering menus in standard (3-dimensional) look, use the shadow colors for fake relief.
    // Otherwise, use foreground colors.
    if (menu_3d_look) {
        GDrawDrawLine(pixmap,p[0].x,p[0].y,p[1].x,p[1].y,mb->g.box->border_darker);
        GDrawDrawLine(pixmap,p[0].x,p[0].y+pt,p[1].x+pt,p[1].y,mb->g.box->border_darker);
        GDrawDrawLine(pixmap,p[1].x,p[1].y,p[2].x,p[2].y,mb->g.box->border_brightest);
        GDrawDrawLine(pixmap,p[1].x+pt,p[1].y,p[2].x-pt,p[2].y,mb->g.box->border_brightest);
        GDrawDrawLine(pixmap,p[2].x,p[2].y,p[0].x,p[0].y,mb->g.box->border_darkest);
        GDrawDrawLine(pixmap,p[2].x-pt,p[2].y,p[0].x,p[0].y+pt,mb->g.box->border_darkest);
    } else {
        GDrawDrawLine(pixmap,p[0].x,p[0].y,p[1].x,p[1].y,mb->g.box->main_foreground);
        GDrawDrawLine(pixmap,p[0].x,p[0].y+pt,p[1].x+pt,p[1].y,mb->g.box->main_foreground);
        GDrawDrawLine(pixmap,p[1].x,p[1].y,p[2].x,p[2].y,mb->g.box->main_foreground);
        GDrawDrawLine(pixmap,p[1].x+pt,p[1].y,p[2].x-pt,p[2].y,mb->g.box->main_foreground);
        GDrawDrawLine(pixmap,p[2].x,p[2].y,p[0].x,p[0].y,mb->g.box->main_foreground);
        GDrawDrawLine(pixmap,p[2].x-pt,p[2].y,p[0].x,p[0].y+pt,mb->g.box->main_foreground);
    }
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
    GMenuBarGetFont,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL
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
    wid = GDrawPointsToPixels(mb->g.base,8);
    mb->xs[0] = GDrawPointsToPixels(mb->g.base,2);
    for ( i=0; i<mb->mtot; ++i )
	mb->xs[i+1] = mb->xs[i]+wid+GTextInfoGetWidth(mb->g.base,&mb->mi[i].ti,NULL);
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
    GMenuBar *mb = calloc(1,sizeof(GMenuBar));

    if ( !gmenubar_inited )
	GMenuInit();
    mb->g.funcs = &gmenubar_funcs;
    _GGadget_Create(&mb->g,base,gd,data,&menubar_box);

    mb->mi = GMenuItemArrayCopy(gd->u.menu,&mb->mtot);
    mb->xs = malloc((mb->mtot+1)*sizeof(uint16));
    mb->entry_with_mouse = -1;
    mb->font = menubar_font;

    GMenuBarFit(mb,gd);
    GMenuBarFindXs(mb);

    MenuMaskInit(mb->mi);
    mb->any_unmasked_shortcuts = GMenuItemArrayAnyUnmasked(mb->mi);

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&mb->g);
    _GWidget_SetMenuBar(&mb->g);

    mb->g.takes_input = true;
return( &mb->g );
}

GGadget *GMenu2BarCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GMenuBar *mb = calloc(1,sizeof(GMenuBar));

    if ( !gmenubar_inited )
	GMenuInit();
    mb->g.funcs = &gmenubar_funcs;
    _GGadget_Create(&mb->g,base,gd,data,&menubar_box);

    mb->mi = GMenuItem2ArrayCopy(gd->u.menu2,&mb->mtot);
    mb->xs = malloc((mb->mtot+1)*sizeof(uint16));
    mb->entry_with_mouse = -1;
    mb->font = menubar_font;

    GMenuBarFit(mb,gd);
    GMenuBarFindXs(mb);

    MenuMaskInit(mb->mi);
    mb->any_unmasked_shortcuts = GMenuItemArrayAnyUnmasked(mb->mi);

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

int GMenuAnyUnmaskedShortcuts(GGadget *mb1, GGadget *mb2) {

    if ( most_recent_popup_menu!=NULL && most_recent_popup_menu->any_unmasked_shortcuts )
return( true );

    if ( mb1!=NULL && ((GMenuBar *) mb1)->any_unmasked_shortcuts )
return( true );

    if ( mb2!=NULL && ((GMenuBar *) mb2)->any_unmasked_shortcuts )
return( true );

return( false );
}
