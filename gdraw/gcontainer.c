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

#include <fontforge-config.h>

#include "gdrawP.h"
#include "ggadget.h"
#include "ggadgetP.h"
#include "gkeysym.h"
#include "gresource.h"
#include "gwidgetP.h"
#include "utype.h"

static GWindow current_focus_window, previous_focus_window, last_input_window;
    /* in focus follows pointer mode, the current focus doesn't really count */
    /*  until the window gets input (otherwise moving the mouse across a wind */
    /*  would set the previous_focus_window to something inappropriate) */
    /* So... we set current focus window when a window gains the focus, and */
    /*  at the same time we set the previous focus window to the last window */
    /*  that got input. NOT to the old current_focus_window (which might be junk) */
static GWindow last_paletted_focus_window = NULL;

static int broken_palettes = true;
static int widgets_initted = false;

static void gwidget_init(void) {
    broken_palettes = GResourceFindBool("GWidget.BrokenPalettes",broken_palettes);
    widgets_initted = true;
}

GWindow GWindowGetCurrentFocusTopWindow(void) {
return( current_focus_window );
}

GWindow GWidgetGetPreviousFocusTopWindow(void) {
return( previous_focus_window );
}

GGadget *GWindowGetCurrentFocusGadget(void) {
    GTopLevelD *td;
    if ( current_focus_window==NULL )
return( NULL );
    td = (GTopLevelD *) (current_focus_window->widget_data);
return( td->gfocus );
}

void GWindowClearFocusGadgetOfWindow(GWindow gw) {
    GTopLevelD *td;
    if ( gw==NULL )
return;
    while ( gw->parent!=NULL && !gw->is_toplevel ) gw=gw->parent;
    td = (GTopLevelD *) (gw->widget_data);
    if ( gw == current_focus_window && td->gfocus!=NULL &&
	    td->gfocus->funcs->handle_focus!=NULL ) {
	GEvent e;
	e.type = et_focus;
	e.w = gw;
	e.u.focus.gained_focus = false;
	e.u.focus.mnemonic_focus = mf_normal;
	(td->gfocus->funcs->handle_focus)(td->gfocus,&e);
    }
    td->gfocus = NULL;
}

GGadget *GWindowGetFocusGadgetOfWindow(GWindow gw) {
    GTopLevelD *td;
    if ( gw==NULL )
return( NULL );
    while ( gw->parent!=NULL && !gw->is_toplevel ) gw=gw->parent;
    td = (GTopLevelD *) (gw->widget_data);
return( td->gfocus );
}

int GGadgetActiveGadgetEditCmd(GWindow gw,enum editor_commands cmd) {
    GGadget *g = GWindowGetFocusGadgetOfWindow(gw);

    if ( g==NULL )
return( false );
return( GGadgetEditCmd(g,cmd));
}

GWindow GWidgetGetCurrentFocusWindow(void) {
    GTopLevelD *td;
    if ( current_focus_window==NULL )
return( NULL );
    td = (GTopLevelD *) (current_focus_window->widget_data);
    if ( td->gfocus!=NULL )
return( td->gfocus->base );
    return NULL;
}

struct gfuncs *last_indicatedfocus_funcs;		/* !!!! Debug code */
GGadget *last_indicatedfocus_gadget;
struct gwindow *last_indicatedfocus_widget;

static void _GWidget_IndicateFocusGadget(GGadget *g, enum mnemonic_focus mf) {
    GWindow top;
    GTopLevelD *td;
    GEvent e;

  last_indicatedfocus_funcs = g->funcs;
  last_indicatedfocus_gadget = g;
  last_indicatedfocus_widget = g->base;

    if ( g->funcs==NULL ) {
	fprintf( stderr, "Bad focus attempt\n" );
return;
    }

    // We recurse and find the top-level gadget.
    for ( top=g->base; top->parent!=NULL && !top->is_toplevel ; top=top->parent );
    td = (GTopLevelD *) (top->widget_data);

    // We check whether the gadget in question has focus.
    if ( td->gfocus!=g ) {
      // If not, we try to deal with the lack of focus.
/* Hmm. KDE doesn't give us a focus out event when we make a window invisible */
/*  So to be on the save side lets send local focus out events even when not */
/*  strictly needed */
	if ( /*top == current_focus_window &&*/ td->gfocus!=NULL &&
		td->gfocus->funcs->handle_focus!=NULL ) {
            // We use the focus handler provided by the presently focussed gadget and process a loss-of-focus event for the currently focused object.
            memset(&e, 0, sizeof(GEvent));
	    e.type = et_focus;
	    e.w = top;
	    e.u.focus.gained_focus = false;
	    e.u.focus.mnemonic_focus = mf_normal;
	    (td->gfocus->funcs->handle_focus)(td->gfocus,&e);
	}
    }
    // We give focus to the desired gadget.
    td->gfocus = g;
    if ( top == current_focus_window && g->funcs->handle_focus!=NULL ) {
        // If the desired gadget has a focus handler, we construct an event and run it.
        memset(&e, 0, sizeof(GEvent));
	e.u.focus.gained_focus = true;
	e.u.focus.mnemonic_focus = mf;
	(g->funcs->handle_focus)(g,&e);
    }
}

void GWidgetIndicateFocusGadget(GGadget *g) {
    _GWidget_IndicateFocusGadget(g,mf_normal);
}

static GGadget *_GWidget_FindPost(GContainerD *cd,GGadget *oldfocus,GGadget **last) {
    GGadget *g;
    GWidgetD *w;

    if ( cd==NULL || !cd->iscontainer )
return( false );
    for ( g=cd->gadgets; g!=NULL; g=g->prev ) {
	if ( g==oldfocus )
return( *last );
	if ( g->focusable && g->state!=gs_invisible && g->state!=gs_disabled )
	    *last = g;
    }
    for ( w=cd->widgets; w!=NULL; w=w->next ) {
	if (( g = _GWidget_FindPost((GContainerD *) w,oldfocus,last))!=NULL )
return( g );
    }
return( NULL );
}

void GWidgetNextFocus(GWindow top) {
    GTopLevelD *topd;
    GGadget *focus, *last=NULL;

    while ( top->parent!=NULL && !top->is_toplevel ) top = top->parent;
    topd = (GTopLevelD *) (top->widget_data);
    if ( topd==NULL || topd->gfocus == NULL )
return;
    if ( (focus = _GWidget_FindPost((GContainerD *) topd,topd->gfocus,&last))== NULL ) {
	/* if we didn't find a Next Gadget, it's either because: */
	/*  1) the focus gadget is first in the chain, in which case we want */
	/*	the last thing in the chain */
	/*  2) our data structures are screwed up */
	_GWidget_FindPost((GContainerD *) topd,NULL,&last);
	focus = last;
    }
    _GWidget_IndicateFocusGadget(focus,mf_tab);
}

static GGadget *_GWidget_FindPrev(GContainerD *cd,GGadget *oldfocus,GGadget **first, int *found) {
    GGadget *g;
    GWidgetD *w;

    if ( cd==NULL || !cd->iscontainer )
return( false );
    for ( g=cd->gadgets; g!=NULL; g=g->prev ) {
	if ( g->focusable && g->state!=gs_invisible && g->state!=gs_disabled ) {
	    if ( *first==NULL )
		*first = g;
	    if ( *found )
return( g );
	}
	if ( g==oldfocus )
	    *found = true;
    }
    for ( w=cd->widgets; w!=NULL; w=w->next ) {
	if (( g = _GWidget_FindPrev((GContainerD *) w,oldfocus,first,found))!=NULL )
return( g );
    }
return( NULL );
}
    
void GWidgetPrevFocus(GWindow top) {
    GTopLevelD *topd;
    GGadget *focus;

    while ( top->parent!=NULL && !top->is_toplevel ) top = top->parent;
    topd = (GTopLevelD *) (top->widget_data);
    if ( topd==NULL || topd->gfocus == NULL )
return;
    for ( focus = topd->gfocus->prev;
	    focus!=NULL && (!focus->focusable || focus->state==gs_invisible || focus->state==gs_disabled);
	    focus = focus->prev );
    if ( focus==NULL ) {
	GGadget *first = NULL; int found = false;
	if ( (focus = _GWidget_FindPrev((GContainerD *) topd,topd->gfocus,&first,&found))== NULL )
	    focus = first;
    }
    _GWidget_IndicateFocusGadget(focus,mf_tab);
}

static int _GWidget_Container_eh(GWindow gw, GEvent *event) {
/* Gadgets can get mouse, char and expose events */
/* Widgets might need char events redirected to them */
/* If a gadget doesn't want an event it returns false from its eh */
/* If a subwidget doesn't want an event it may send it up to us */
    int handled = false;
    GGadget *gadget;
    GContainerD *gd = (GContainerD *) (gw->widget_data);
    GWindow pixmap;
    GWindow parent;
    GTopLevelD *topd;

    if ( gd==NULL )			/* dying */
return(true);

    for ( parent = gw; parent->parent!=NULL && parent->parent->widget_data!=NULL &&
	    !parent->is_toplevel; parent = parent->parent );
    topd = (GTopLevelD *) (parent->widget_data);
    if ( topd==NULL )
	fprintf( stderr, "No top level window found\n" );

    GGadgetPopupExternalEvent(event);
    if ( event->type == et_expose ) {
	GRect old;

	pixmap = _GWidget_GetPixmap(gw,&event->u.expose.rect);
	pixmap->ggc->bg = gw->ggc->bg;
	GDrawPushClip(pixmap,&event->u.expose.rect,&old);
	pixmap->user_data = gw->user_data;

	if ( gd->e_h!=NULL ) {
	    (gd->e_h)(pixmap,event);
	}
	for ( gadget = gd->gadgets; gadget!=NULL ; gadget=gadget->prev )
	    if ( ! ( gadget->r.x>event->u.expose.rect.x+event->u.expose.rect.width ||
		    gadget->r.y>event->u.expose.rect.y+event->u.expose.rect.height ||
		    gadget->r.x+gadget->r.width<event->u.expose.rect.x ||
		    gadget->r.y+gadget->r.height<event->u.expose.rect.y ) &&
		    gadget->state!=gs_invisible )
		(gadget->funcs->handle_expose)(pixmap,gadget,event);

	GDrawPopClip(pixmap,&old);
	_GWidget_RestorePixmap(gw,pixmap,&event->u.expose.rect);

return( true );
    } else if ( event->type >= et_mousemove && event->type <= et_crossing ) {
	if ( topd!=NULL && topd->popupowner!=NULL ) {
	    handled = (topd->popupowner->funcs->handle_mouse)(topd->popupowner,event);
return( handled );
	}
	if ( gd->grabgadget && event->type==et_mousedown &&
		(event->u.mouse.state&ksm_buttons)==0 )
	    gd->grabgadget = NULL;		/* Happens if a gadget invokes a popup menu. Gadget remains grabbed because menu gets the mouse up */
	if ( gd->grabgadget!=NULL ) {
	    handled = (gd->grabgadget->funcs->handle_mouse)(gd->grabgadget,event);
	    if ( !GDrawNativeWindowExists(NULL,event->native_window) ||
		    gw->is_dying || gw->widget_data==NULL )
return( true );
	    if ( event->type==et_mouseup )
		gd->grabgadget = NULL;
	} else if ( event->type==et_mousedown ) {
	    if ( !parent->is_popup )
		last_input_window = parent;
	    for ( gadget = gd->gadgets; gadget!=NULL && !handled ; gadget=gadget->prev ) {
		if ( gadget->state!=gs_disabled && gadget->state!=gs_invisible &&
			gadget->takes_input &&
			GGadgetWithin(gadget,event->u.mouse.x,event->u.mouse.y)) {
		    handled = (gadget->funcs->handle_mouse)(gadget,event);
		    if ( !GDrawNativeWindowExists(NULL,event->native_window) ||
			    gw->is_dying || gw->widget_data==NULL )
return( true );
		    gd->grabgadget = gadget;
		    if ( gadget->focusable && handled )
			GWidgetIndicateFocusGadget(gadget);
		}
	    }
	} else {
	    for ( gadget = gd->gadgets; gadget!=NULL && !handled ; gadget=gadget->prev ) {
		if ( !gadget->state!=gs_disabled && gadget->state!=gs_invisible &&
			/*gadget->takes_input &&*/	/* everybody needs mouse moves for popups, even labels */
			GGadgetWithin(gadget,event->u.mouse.x,event->u.mouse.y)) {
		    if ( gd->lastwiggle!=NULL && gd->lastwiggle!=gadget )
			(gd->lastwiggle->funcs->handle_mouse)(gd->lastwiggle,event);
		    handled = (gadget->funcs->handle_mouse)(gadget,event);
		    if ( !GDrawNativeWindowExists(NULL,event->native_window) ||
			    gw->is_dying || gw->widget_data==NULL )
return( true );
		    gd->lastwiggle = gadget;
		}
	    }
	    if ( !handled && gd->lastwiggle!=NULL ) {
		(gd->lastwiggle->funcs->handle_mouse)(gd->lastwiggle,event);
		if ( !GDrawNativeWindowExists(NULL,event->native_window) ||
			gw->is_dying )
return( true );
	    }
	}
    } else if ( event->type == et_char || event->type == et_charup ) {
	if ( topd!=NULL ) {
	    handled = (topd->handle_key)(parent,gw,event);
	}
    } else if ( event->type == et_drag || event->type == et_dragout || event->type==et_drop ) {
	GGadget *lastdd = NULL;
	GEvent e;
	for ( gadget = gd->gadgets; gadget!=NULL && !handled ; gadget=gadget->prev ) {
	    if ( gadget->funcs->handle_sel &&
		    GGadgetWithin(gadget,event->u.drag_drop.x,event->u.drag_drop.y))
		if (( handled = (gadget->funcs->handle_sel)(gadget,event) )) {
		    lastdd = gadget;
		    if ( !GDrawNativeWindowExists(NULL,event->native_window) ||
			    gw->is_dying )
return( true );
		    }
	}
	if ( !handled && gd->e_h!=NULL ) {
	    handled = (gd->e_h)(gw,event);
	    if ( !GDrawNativeWindowExists(NULL,event->native_window) ||
		    gw->is_dying )
return( true );
	    lastdd = (GGadget *) -1;
	}
	e.type = et_dragout;
	e.w = gw;
	if ( lastdd!=gd->lastddgadget ) {
	    if ( gd->lastddgadget==(GGadget *) -1 )
		(gd->e_h)(gw,&e);
	    else if ( gd->lastddgadget!=NULL )
		(gd->lastddgadget->funcs->handle_sel)(gd->lastddgadget,&e);
	    if ( !GDrawNativeWindowExists(NULL,event->native_window) ||
		    gw->is_dying )
return( true );
	}
	if ( event->type==et_drag )
	    gd->lastddgadget = lastdd;
	else
	    gd->lastddgadget = NULL;
return( handled );
    } else if ( event->type == et_selclear ) {
	for ( gadget = gd->gadgets; gadget!=NULL && !handled ; gadget=gadget->prev ) {
	    if ( gadget->funcs->handle_sel ) {
		if ( (handled = (gadget->funcs->handle_sel)(gadget,event)) )
	break;
	    }
	}
    } else if ( event->type == et_timer ) {
	for ( gadget = gd->gadgets; gadget!=NULL && !handled ; gadget=gadget->prev ) {
	    if ( gadget->funcs->handle_timer ) {
		if ( (handled = (gadget->funcs->handle_timer)(gadget,event)) )
	break;
	    }
	}
    } else if ( event->type == et_resize ) {
	GWidgetFlowGadgets(gw);
    }
    if ( gd->e_h!=NULL && (!handled || event->type==et_mousemove ))
	handled = (gd->e_h)(gw,event);
    if ( event->type == et_destroy ) {
	GGadget *prev;
	for ( gadget = gd->gadgets; gadget!=NULL ; gadget=prev ) {
	    prev = gadget->prev;
	    if ( !gadget->contained )
		(gadget->funcs->destroy)(gadget);
	    /* contained ggadgets will be destroyed when their owner is destroyed */
	}
	/* Widgets are windows and should get their own destroy events and free themselves */
	/* remove us from our parent */
	parent = gw->parent;
	if ( parent!=NULL && parent->widget_data!=NULL ) {
	    GContainerD *pgd = (GContainerD *) (parent->widget_data);
	    struct gwidgetdata *wd, *pwd;
	    for ( pwd=NULL, wd=pgd->widgets; wd!=NULL && wd!=(struct gwidgetdata *) gd; pwd = wd, wd = wd->next );
	    if ( pwd==NULL )
		pgd->widgets = gd->next;
	    else
		pwd->next = gd->next;
	}
	free(gd);
	gw->widget_data = NULL;
    }
return( handled );
}

static int GWidgetCheckMn(GContainerD *gd,GEvent *event) {
    int handled = false;
    GGadget *gadget, *last;
    struct gwidgetdata *widget;
    unichar_t keysym = event->u.chr.keysym;
    int mask = GMenuMask() & (ksm_control|ksm_cmdmacosx);

    if ( (mask&ksm_cmdmacosx) && keysym>0x7f &&
	    (event->u.chr.state&ksm_meta) && !(event->u.chr.state&mask) )
	keysym = GGadgetUndoMacEnglishOptionCombinations(event);

    if ( islower(keysym)) keysym = toupper(keysym);
    for ( gadget = gd->gadgets; gadget!=NULL && !handled ; gadget=gadget->prev ) {
	if ( (event->u.chr.state&ksm_meta) && !(event->u.chr.state&mask) &&
		gadget->mnemonic==keysym &&
		gadget->state != gs_invisible && gadget->state != gs_disabled ) {
	    if ( gadget->focusable ) {	/* labels may have a mnemonic */
		    /* (ie. because textfields can't display mnemonics) */
		    /* but they don't act on it */
		_GWidget_IndicateFocusGadget(gadget,mf_mnemonic);
		handled = true;
	    } else if ( last!=NULL && last->mnemonic=='\0' ) {
		/* So if we get a label with a mnemonic, and the next gadget */
		/*  is focusable and doesn't have a mnemoic itself, give it */
		/*  the label's focus */
		_GWidget_IndicateFocusGadget(last,mf_mnemonic);
		handled = true;
	    }
	} else if ( gadget->shortcut == keysym &&
		(gadget->short_mask&event->u.chr.state)==gadget->short_mask ) {
	    _GWidget_IndicateFocusGadget(gadget,mf_shortcut);
	    handled = true;
	} else if ( gadget->state != gs_invisible &&
		gadget->state != gs_disabled &&
		gadget->focusable ) {
	    last = gadget;
	}
    }
    for ( widget = gd->widgets; widget!=NULL && !handled ; widget=widget->next ) {
	if ( widget->iscontainer )
	    handled = GWidgetCheckMn((GContainerD *) widget,event);
    }
return( handled );
}

static int _GWidget_TopLevel_Key(GWindow top, GWindow ew, GEvent *event) {
    GTopLevelD *topd = (GTopLevelD *) (top->widget_data);
    int handled=0;
    GEvent sub;

    if ( !top->is_popup )
	last_input_window = top;

    /* If the palette has the focus, and it usually will under kde, then */
    /*  give the event to the main window if the cursor is outside of the palette */
    if ( topd->ispalette ) {
	if ( event->u.chr.x<-2 || event->u.chr.x>top->pos.width+2 ||
		event->u.chr.y<-2 || event->u.chr.y>top->pos.height+2 ) {
	    GPoint p;
	    topd = topd->owner;
	    p.x = event->u.chr.x; p.y = event->u.chr.y;
	    GDrawTranslateCoordinates(ew,topd->w,&p);
	    event->u.chr.x = p.x; event->u.chr.y = p.y;
	    ew = top = topd->w;
	    event->w = top;
	}
    }
    /* Check for mnemonics and shortcuts */
    if ( event->type == et_char && !GKeysymIsModifier(event->u.chr.keysym) ) {
	handled = GMenuPopupCheckKey(event);
	if ( topd->ispalette ) {
	    if ( !(handled = GMenuBarCheckKey(top,topd->owner->gmenubar,event)) )
		handled = GWidgetCheckMn((GContainerD *) topd->owner,event);
	}
	if ( !handled )
	    if ( !(handled = GMenuBarCheckKey(top,topd->gmenubar,event)) )
		handled = GWidgetCheckMn((GContainerD *) topd,event);
    }
    if ( handled )
	/* No op */;
    else if ( topd->popupowner!=NULL ) {
	/* When we've got an active popup (menu, list, etc.) all key events */
	/*  go to the popups owner (which, presumably sends them to the popup)*/
	if ( topd->popupowner->funcs->handle_key !=NULL )
	    handled = (topd->popupowner->funcs->handle_key)(topd->popupowner,event);
    } else if ( topd->gfocus!=NULL && topd->gfocus->funcs->handle_key )
	handled = (topd->gfocus->funcs->handle_key)(topd->gfocus,event);
    if ( !handled ) {
	if ( ew->widget_data==NULL ) {
	    if ( ew->eh!=NULL )
		handled = (ew->eh)(ew,event);
	} else if ( ew->widget_data->e_h!=NULL )
	    handled = (ew->widget_data->e_h)(ew,event);
    }

    if ( event->type==et_charup )
return( handled );
    /* If no one wanted it then try keyboard navigation */
    /* Tab or back tab cycles the focus through our widgets/gadgets */
    if ( !handled && (event->u.chr.keysym==GK_Tab || event->u.chr.keysym==GK_BackTab )) {
	if ( event->u.chr.keysym==GK_BackTab || (event->u.chr.state&ksm_shift) )
	    GWidgetPrevFocus(ew);
	else
	    GWidgetNextFocus(ew);
	handled = true;
    }
    /* Return activates the default button (if there is one) */
    else if ( !handled && (event->u.chr.keysym==GK_Return || event->u.chr.keysym==GK_KP_Enter) &&
	    topd->gdef!=NULL ) {
	sub.type = et_controlevent;
	sub.w = topd->gdef->base;
	sub.u.control.subtype = et_buttonactivate;
	sub.u.control.g = topd->gdef;
	sub.u.control.u.button.clicks = 0;
	if ( topd->gdef->handle_controlevent != NULL )
	    (topd->gdef->handle_controlevent)(topd->gdef,&sub);
	else
	    GDrawPostEvent(&sub);
    }
    /* Escape activates the cancel button (if there is one) */
    /*  (On the mac, Command-. has that meaning) */
    else if ( !handled && topd->gcancel!=NULL &&
	    (event->u.chr.keysym==GK_Escape ||
	     ((GMenuMask()&ksm_cmdmacosx) &&
	      (event->u.chr.state&GMenuMask())==ksm_cmdmacosx &&
	      event->u.chr.keysym=='.'))) {
	sub.type = et_controlevent;
	sub.w = topd->gcancel->base;
	sub.u.control.subtype = et_buttonactivate;
	sub.u.control.g = topd->gcancel;
	sub.u.control.u.button.clicks = 0;
	if ( topd->gcancel->handle_controlevent != NULL )
	    (topd->gcancel->handle_controlevent)(topd->gcancel,&sub);
	else
	    GDrawPostEvent(&sub);
    }
return( handled );
}

static int GiveToAll(GContainerD *wd, GEvent *event) {
    GGadget *g;
    GContainerD *sub;

    if ( wd!=NULL && wd->iscontainer ) {
	for ( g=wd->gadgets; g!=NULL; g=g->prev )
	    if ( g->funcs->handle_mouse!=NULL )
		(g->funcs->handle_mouse)(g,event);
	for ( sub = (GContainerD *) (wd->widgets); sub!=NULL;
		sub=(GContainerD *) (sub->next) )
	    GiveToAll(sub,event);
    }
    if ( wd!=NULL ) {
	if ( wd->e_h!=NULL )
	    (wd->e_h)(wd->w,event);
    } else {
	if ( wd->w->eh!=NULL )
	    (wd->w->eh)(wd->w,event);
    }
return( true );
}

static void ManagePalettesVis(GTopLevelD *td, int is_visible ) {
    GTopLevelD *palette;

    if ( td->w!=last_paletted_focus_window )
return;
    for ( palette=td->palettes; palette!=NULL; palette = palette->nextp ) {
	if ( is_visible && palette->w->visible_request )
	    GDrawSetVisible(palette->w,true);
	else if ( !is_visible && palette->w->visible_request ) {
	    GDrawSetVisible(palette->w,false);
	    palette->w->visible_request = true;
	}
    }
}

static GTopLevelD *oldtd = NULL;
static GGadget *oldgfocus = NULL;

static int _GWidget_TopLevel_eh(GWindow gw, GEvent *event) {
    GTopLevelD *td;
    int ret;

    if ( !GDrawNativeWindowExists(NULL,event->native_window) )
return( true );

    td = (GTopLevelD *) (gw->widget_data);
    if ( td==NULL )		/* Dying */
return( true );

    GGadgetPopupExternalEvent(event);
    if ( event->type==et_focus ) {
	
	if ( event->u.focus.gained_focus ) {
	    if ( gw->is_toplevel && !gw->is_popup && !gw->is_dying ) {
		if ( last_input_window!=gw )
		    previous_focus_window = last_input_window;
		current_focus_window = gw;
	    }
	} else if ( current_focus_window==gw ) {
	    current_focus_window = NULL;
	}
	if ( !td->ispalette && gw->is_visible && event->u.focus.gained_focus && !gw->is_dying ) {
	    GWindow dlg = GDrawGetRedirectWindow(NULL);
	    if ( dlg==NULL || dlg==gw ) {
		/* If top level window loses the focus all its palettes go invisible */
		/* if it gains focus then all palettes that are supposed to be vis */
		/*  become visible */
		/* But not if we've got an active dialog */
		GTopLevelD *palette;
		if ( last_paletted_focus_window!=NULL && !last_paletted_focus_window->is_dying ) {
		    GTopLevelD *lpfw_td = (GTopLevelD *) (last_paletted_focus_window->widget_data);
		    for ( palette=lpfw_td->palettes; palette!=NULL; palette = palette->nextp ) {
			if ( !palette->w->is_visible && palette->w->visible_request ) {
			    GDrawSetVisible(palette->w,false);
			    palette->w->visible_request = true;
			}
		    }
		}
		for ( palette=td->palettes; palette!=NULL; palette = palette->nextp ) {
		    if ( !palette->w->is_visible && palette->w->visible_request )
			GDrawSetVisible(palette->w,true);
		}
		last_paletted_focus_window = gw;
	    }
	}
	if ( !gw->is_dying && td->gfocus!=NULL && td->gfocus->funcs->handle_focus!=NULL ) {
 { oldtd = td; oldgfocus = td->gfocus; }	/* Debug!!!! */
	    (td->gfocus->funcs->handle_focus)(td->gfocus,event);
	}
	if ( !gw->is_dying && td->e_h!=NULL )
	    (td->e_h)(gw,event);
return( true );
    } else if ( !gw->is_dying && event->type == et_crossing ) {
	GiveToAll((GContainerD *) td,event);
return( true );
    } else if ( event->type == et_char || event->type == et_charup ) {
        // If this is not a nested event, redirect it at the window that
        // contains the currently focused gadget.
        if (!td->isnestedkey && td->gfocus && td->gfocus->base) {
            td->isnestedkey = true;
            ret = _GWidget_TopLevel_Key(gw, td->gfocus->base, event);
            td->isnestedkey = false;
        } else {
            ret = _GWidget_TopLevel_Key(gw, gw, event);
        }
        return ret;
    } else if ( !gw->is_dying && event->type == et_resize ) {
	GRect r;
	if ( td->gmenubar!=NULL ) {
	    GGadgetGetSize(td->gmenubar,&r);
	    GGadgetResize(td->gmenubar,event->u.resize.size.width,r.height);
	    GGadgetRedraw(td->gmenubar);
	} /* status line, toolbar, etc. */
	if ( td->palettes!=NULL && event->u.resize.moved ) {
	    GTopLevelD *palette;
	    for ( palette=td->palettes; palette!=NULL; palette = palette->nextp ) {
		if ( !broken_palettes || !palette->positioned_yet ) {
		    int x = gw->pos.x + palette->owner_off_x,
			y = gw->pos.y + palette->owner_off_y;
		    if ( x<0 ) x=0;
		    if ( y<0 ) y=0;
		    if ( x+palette->w->pos.width>GDrawGetRoot(NULL)->pos.width )
			x = GDrawGetRoot(NULL)->pos.width-palette->w->pos.width;
		    if ( y+palette->w->pos.height>GDrawGetRoot(NULL)->pos.height )
			y = GDrawGetRoot(NULL)->pos.height-palette->w->pos.height;
		    ++palette->programmove;
		    if ( gw->is_visible )
			GDrawTrueMove(palette->w, x, y);
		    else
			GDrawMove(palette->w, x, y);
		    palette->positioned_yet = true;
		}
	    }
	}
	if ( td->ispalette ) {
	    if ( td->programmove>0 )
		--td->programmove;
	    else {
		td->owner_off_x = gw->pos.x - td->owner->w->pos.x;
		td->owner_off_y = gw->pos.y - td->owner->w->pos.y;
	    }
	}
    } else if ( event->type == et_close && td->ispalette ) {
	GDrawSetVisible(gw,false);
return( true );
    } else if ( !gw->is_dying && event->type == et_visibility ) {
	if ( broken_palettes )
	    /* Do Nothing */;
	else if ( td->ispalette && event->u.visibility.state!=vs_unobscured ) {
	    if ( !GDrawIsAbove(gw,td->owner->w))
		GDrawRaiseAbove(gw,td->owner->w);
	}
    } else if ( !gw->is_dying && event->type == et_map && !td->ispalette ) {
	/* If top level window goes invisible all its palettes follow */
	/* if it goes visible then all palettes that are supposed to be vis */
	/*  follow */
	ManagePalettesVis(td, event->u.map.is_visible );
    }
    if ( event->type == et_destroy ) {
	if ( td->palettes!=NULL ) {
	    struct gtopleveldata *palettes, *next;
	    for ( palettes=td->palettes; palettes!=NULL; palettes = next ) {
		next = palettes->nextp;
		GDrawDestroyWindow(palettes->w);
	    }
	    /* Palettes must die before our widget data are freed */
	    GDrawSync(GDrawGetDisplayOfWindow(gw));
	    GDrawProcessPendingEvents(GDrawGetDisplayOfWindow(gw));
	}
    }
    ret = _GWidget_Container_eh(gw,event);
    if ( event->type == et_destroy ) {
	if ( gw==current_focus_window )
	    current_focus_window = NULL;
	if ( gw==previous_focus_window )
	    previous_focus_window = NULL;
	if ( gw==last_input_window )
	    last_input_window = NULL;
	if ( gw==last_paletted_focus_window )
	    last_paletted_focus_window = NULL;
	ret = true;
    }
return( ret );
}

static struct wfuncs _gwidget_container_funcs;
static struct wfuncs _gwidget_toplevel_funcs;

static void MakeContainerWidget(GWindow gw) {
    struct gwidgetcontainerdata *gd;

    if ( gw->widget_data!=NULL )
	GDrawIError( "Attempt to make a window into a widget twice");
    if ( !widgets_initted )
	gwidget_init();
    if ( gw->parent==NULL || gw->is_toplevel )
	gd = calloc(1,sizeof(struct gtopleveldata));
    else
	gd = calloc(1,sizeof(struct gwidgetcontainerdata));
    gw->widget_data = (struct gwidgetdata *) gd;
    gd->w = gw;
    gd->e_h = gw->eh;
    gw->eh = _GWidget_Container_eh;
    gd->enabled = true;
    gd->iscontainer = true;
    gd->funcs = &_gwidget_container_funcs;
    if ( gw->parent!=NULL && !gw->is_toplevel ) {
	if ( gw->parent->widget_data==NULL )
	    MakeContainerWidget(gw->parent);
	if ( !gw->parent->widget_data->iscontainer )
	    GDrawIError( "Attempt to add a widget to something which is not a container");
	gd->next = ((struct gwidgetcontainerdata *) (gw->parent->widget_data))->widgets;
	((struct gwidgetcontainerdata *) (gw->parent->widget_data))->widgets =
		(struct gwidgetdata *) gd;
    } else {
	struct gtopleveldata *topd;
	topd = (struct gtopleveldata *) gd;
	gd->funcs = &_gwidget_toplevel_funcs;
	gw->eh = _GWidget_TopLevel_eh;
	topd->handle_key = _GWidget_TopLevel_Key;
	topd->istoplevel = true;
    }
}

void _GWidget_AddGGadget(GWindow gw,GGadget *g) {
    struct gwidgetcontainerdata *gd;

    if ( gw->widget_data==NULL )
	MakeContainerWidget(gw);
    gd = (struct gwidgetcontainerdata *) (gw->widget_data);
    if ( !gd->iscontainer )
	GDrawIError( "Attempt to add a gadget to something which is not a container");
    g->prev = gd->gadgets;
    gd->gadgets = g;
    if ( g->base!=NULL )
	GDrawIError( "Attempt to add a gadget to two widgets" );
    g->base = gw;
}

void _GWidget_RemoveGadget(GGadget *g) {
    struct gwidgetcontainerdata *gd;
    GTopLevelD *td;
    GWindow gw = g->base;
    GGadget *next;

    if ( gw==NULL )
return;

    gd = (struct gwidgetcontainerdata *) (gw->widget_data);
    if ( gd==NULL || !gd->iscontainer )
	GDrawIError( "Attempt to remove a gadget to something which is not a container");
    if ( gd->gadgets==g )
	gd->gadgets = g->prev;
    else {
	for ( next = gd->gadgets; next!=NULL && next->prev!=g; next = next->prev );
	if ( next==NULL )
	    GDrawIError( "Attempt to remove a gadget which is not in the gadget list" );
	else
	    next->prev = g->prev;
    }
    if ( gd->grabgadget == g ) gd->grabgadget = NULL;
    g->prev = NULL;
    g->base = NULL;

    while ( gw->parent!=NULL && !gw->is_toplevel ) gw = gw->parent;
    td = (GTopLevelD *) (gw->widget_data);
    if ( td->gdef == g ) td->gdef = NULL;
    if ( td->gcancel == g ) td->gcancel = NULL;
    if ( td->gfocus == g ) td->gfocus = NULL;
}

void _GWidget_SetDefaultButton(GGadget *g) {
    struct gtopleveldata *gd=NULL;
    GWindow gw = g->base;

    if ( gw!=NULL ) while ( gw->parent != NULL && !gw->is_toplevel ) gw = gw->parent;
    if ( gw!=NULL )
	gd = (struct gtopleveldata *) (gw->widget_data);
    if ( gd==NULL || !gd->istoplevel )
	GDrawIError( "This gadget isn't in a top level widget, can't be a default button" );
    else
	gd->gdef = g;
}

/* The previous function assumes everything is set up properly. This one doesn't */
/*  it's for when you've got two buttons which might be default and you toggle */
/*  between them. It lets each button know if it's default, and then redraws */
/*  both */
void _GWidget_MakeDefaultButton(GGadget *g) {
    struct gtopleveldata *gd=NULL;
    GWindow gw = g->base;

    if ( gw!=NULL ) while ( gw->parent != NULL && !gw->is_toplevel ) gw = gw->parent;
    if ( gw!=NULL )
	gd = (struct gtopleveldata *) (gw->widget_data);
    if ( gd==NULL || !gd->istoplevel )
	GDrawIError( "This gadget isn't in a top level widget, can't be a default button" );
    else if ( gd->gdef!=g ) {
	_GButton_SetDefault(gd->gdef,false);
	gd->gdef = g;
	_GButton_SetDefault(g,true);
    }
}

void _GWidget_SetCancelButton(GGadget *g) {
    struct gtopleveldata *gd=NULL;
    GWindow gw = g->base;

    if ( gw!=NULL ) while ( gw->parent != NULL && !gw->is_toplevel ) gw = gw->parent;
    if ( gw!=NULL )
	gd = (struct gtopleveldata *) (gw->widget_data);
    if ( gd==NULL || !gd->istoplevel )
	GDrawIError( "This gadget isn't in a top level widget, can't be a cancel button" );
    else
	gd->gcancel = g;
}

void _GWidget_SetMenuBar(GGadget *g) {
    struct gtopleveldata *gd=NULL;
    GWindow gw = g->base;

    if ( gw!=NULL ) while ( gw->parent != NULL && !gw->is_toplevel ) gw = gw->parent;
    if ( gw!=NULL )
	gd = (struct gtopleveldata *) (gw->widget_data);
    if ( gd==NULL || !gd->istoplevel )
	GDrawIError( "This gadget isn't in a top level widget, can't be a menubar" );
    else
	gd->gmenubar = g;
}

void _GWidget_SetPopupOwner(GGadget *g) {
    struct gtopleveldata *gd=NULL;
    GWindow gw = g->base;

    if ( gw!=NULL ) while ( gw->parent != NULL && !gw->is_toplevel ) gw = gw->parent;
    if ( gw!=NULL )
	gd = (struct gtopleveldata *) (gw->widget_data);
    if ( gd==NULL || !gd->istoplevel )
	GDrawIError( "This gadget isn't in a top level widget, can't have a popup" );
    else
	gd->popupowner = g;
}

void _GWidget_ClearPopupOwner(GGadget *g) {
    struct gtopleveldata *gd=NULL;
    GWindow gw = g->base;

    if ( gw!=NULL ) while ( gw->parent != NULL && !gw->is_toplevel ) gw = gw->parent;
    if ( gw!=NULL )
	gd = (struct gtopleveldata *) (gw->widget_data);
    if ( gd==NULL || !gd->istoplevel )
	GDrawIError( "This gadget isn't in a top level widget, can't have a popup" );
    else
	gd->popupowner = NULL;
}

void _GWidget_SetGrabGadget(GGadget *g) {
    struct gwidgetcontainerdata *gd=NULL;
    GWindow gw = g->base;

    if ( gw!=NULL )
        gd = (struct gwidgetcontainerdata *) (gw->widget_data);
    if ( gd==NULL || !gd->iscontainer )
        GDrawIError( "This gadget isn't in a container, can't be a grab gadget" );
    else
        gd->grabgadget = g;
}

void _GWidget_ClearGrabGadget(GGadget *g) {
    struct gwidgetcontainerdata *gd=NULL;
    GWindow gw = g->base;

    if ( gw!=NULL )
        gd = (struct gwidgetcontainerdata *) (gw->widget_data);
    if ( gd==NULL || !gd->iscontainer )
        GDrawIError( "This gadget isn't in a container, can't be a grab gadget" );
    else
        gd->grabgadget = NULL;
}

GWindow GWidgetCreateTopWindow(GDisplay *gdisp, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs) {
    GWindow gw = GDrawCreateTopWindow(gdisp,pos,eh,user_data,wattrs);
    MakeContainerWidget(gw);
return( gw );
}

GWindow GWidgetCreateSubWindow(GWindow w, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs) {
    GWindow gw = GDrawCreateSubWindow(w,pos,eh,user_data,wattrs);
    MakeContainerWidget(gw);
return( gw );
}

/* Palettes follow their owners when the owner moves around the screen */
/*  Palettes go invisible when the owner does, and become visible again when it does */
/*  Palettes are always on top of their owner */
/*  Palettes go invisible when the owner loses focus, and become visible when it gains focus */
GWindow GWidgetCreatePalette(GWindow w, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs) {
    GWindow gw;
    GPoint pt, base;
    GRect newpos, ownerpos, screensize;
    struct gtopleveldata *gd, *od;
    GWindow root;

    if ( !w->is_toplevel )
return( false );

    pt.x = pos->x; pt.y = pos->y;
    root = GDrawGetRoot(w->display);
    GDrawGetSize(w,&ownerpos);
    GDrawGetSize(root,&screensize);
    GDrawTranslateCoordinates(w,root,&pt);
    base.x = base.y = 0;
    GDrawTranslateCoordinates(w,root,&base);
    if ( pt.x<0 ) {
	if ( base.x+ownerpos.width+20+pos->width+20 > screensize.width )
	    pt.x=0;
	else
	    pt.x = base.x+ownerpos.width+20;
    }
    if ( pt.y<0 ) pt.y=0;
    if ( pt.x+pos->width>root->pos.width )
	pt.x = root->pos.width-pos->width;
    if ( pt.y+pos->height>root->pos.height )
	pt.y = root->pos.height-pos->height;

    newpos.x = pt.x; newpos.y = pt.y; newpos.width = pos->width; newpos.height = pos->height;
    wattrs->event_masks |= (1<<et_visibility);
    if ( !(wattrs->mask&wam_transient)) {
	wattrs->mask |= wam_transient;
	wattrs->transient = GWidgetGetTopWidget(w);
    }
    if ( broken_palettes ) {
	wattrs->mask |= wam_positioned;
	wattrs->positioned = true;
    }
    gw = GDrawCreateTopWindow(w->display,&newpos,eh,user_data,wattrs);
    MakeContainerWidget(gw);
    if ( w->widget_data==NULL )
	MakeContainerWidget(w);
    od = (struct gtopleveldata *) (w->widget_data);
    gd = (struct gtopleveldata *) (gw->widget_data);
    gd->nextp = od->palettes;
    gd->owner = od;
    od->palettes = gd;
    gd->ispalette = true;
    gd->owner_off_x = pos->x; gd->owner_off_y = pos->y;
return( gw );
}

void GWidgetRequestVisiblePalette(GWindow palette,int visible) {
    GTopLevelD *td = (GTopLevelD *) (palette->widget_data);

    if ( td->owner!=NULL ) {
	palette->visible_request = visible;
	if ( td->owner->w == last_paletted_focus_window )
	    GDrawSetVisible(palette,visible);
    }
}

GGadget *GWidgetGetControl(GWindow gw, int cid) {
    GGadget *gadget;
    GContainerD *gd = (GContainerD *) (gw->widget_data);
    GWidgetD *widg;

    if ( gd==NULL || !gd->iscontainer )
return( NULL );
    for ( gadget = gd->gadgets; gadget!=NULL ; gadget=gadget->prev ) {
	if ( gadget->cid == cid )
return( gadget );
    }
    for ( widg = gd->widgets; widg!=NULL; widg = widg->next ) {
	if ( widg->iscontainer ) {
	    gadget = GWidgetGetControl(widg->w,cid);
	    if ( gadget!=NULL )
return( gadget );
	}
    }
return( NULL );
}

GGadget *_GWidgetGetGadgets(GWindow gw) {
    GContainerD *gd;

    if ( gw==NULL )
return( NULL );

    gd = (GContainerD *) (gw->widget_data);

    if ( gd==NULL || !gd->iscontainer )
return( NULL );

return( gd->gadgets );
}

GWindow GWidgetGetParent(GWindow gw) {

return( gw->parent );
}

GWindow GWidgetGetTopWidget(GWindow gw) {

    for ( ; gw->parent!=NULL && !gw->is_toplevel; gw = gw->parent );
return( gw );
}

GDrawEH GWidgetGetEH(GWindow gw) {
    if ( gw->widget_data==NULL )
return( gw->eh );
    else
return( gw->widget_data->e_h );
}

void GWidgetSetEH(GWindow gw, GDrawEH e_h ) {
    if ( gw->widget_data==NULL )
	gw->eh = e_h;
    else
	gw->widget_data->e_h = e_h;
}

void GWidgetHidePalettes(void) {
    GTopLevelD *td, *palette;

    if ( last_paletted_focus_window==NULL )
return;
    td = (GTopLevelD *) (last_paletted_focus_window->widget_data);
    for ( palette=td->palettes; palette!=NULL; palette = palette->nextp ) {
	if ( palette->w->visible_request ) {
	    GDrawSetVisible(palette->w,false);
	    palette->w->visible_request = true;
	}
    }
}

GIC *GWidgetCreateInputContext(GWindow w,enum gic_style def_style) {
    GWidgetD *wd = (GWidgetD *) (w->widget_data);

    if ( wd->gic==NULL )
	wd->gic = GDrawCreateInputContext(w,def_style);
return( wd->gic );
}
    
GIC *GWidgetGetInputContext(GWindow w) {
    GWidgetD *wd = (GWidgetD *) (w->widget_data);
return( wd->gic );
}

void GWidgetFlowGadgets(GWindow gw) {
    GGadget *gadget;
    GContainerD *gd = (GContainerD *) (gw->widget_data);

    if ( gd==NULL )
return;

    gadget = gd->gadgets;
    if ( gadget!=NULL ) {
	while ( gadget->prev!=NULL )
	    gadget=gadget->prev;
    }
    if ( gadget != NULL && GGadgetFillsWindow(gadget)) {
	GRect wsize;
	GDrawGetSize(gw, &wsize);

	/* Make any offset simmetrical */
	if (wsize.width >= 2*gadget->r.x) wsize.width -= 2*gadget->r.x;
	else wsize.width = 0;
	
	if (wsize.height >= 2*gadget->r.y) wsize.height -= 2*gadget->r.y;
	else wsize.height = 0;
	
	GGadgetResize(gadget,wsize.width,wsize.height);
	GDrawRequestExpose(gw,NULL,false);
    }
}

void GWidgetToDesiredSize(GWindow gw) {
    GGadget *gadget;
    GContainerD *gd = (GContainerD *) (gw->widget_data);

    if ( gd==NULL )
return;

    gadget = gd->gadgets;
    if ( gadget!=NULL ) {
	while ( gadget->prev!=NULL )
	    gadget=gadget->prev;
    }
    if ( gadget != NULL && GGadgetFillsWindow(gadget))
	GHVBoxFitWindow(gadget);
}
