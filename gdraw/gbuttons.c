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

#include "gdraw.h"
#include "ggadgetP.h"
#include "gkeysym.h"
#include "gwidget.h"
#include "ustring.h"

static void GListButtonDoPopup(GListButton *);

GBox _GGadget_button_box = GBOX_EMPTY; /* Don't initialize here */
GBox _GGadget_defaultbutton_box = GBOX_EMPTY; /* Don't initialize here */
static GBox _GGadget_cancelbutton_box = GBOX_EMPTY; /* Don't initialize here */
static GBox _GGadget_colorbutton_box = GBOX_EMPTY; /* Don't initialize here */
static GBox _GGadget_droplist_box = GBOX_EMPTY; /* Don't initialize here */
static GBox label_box = GBOX_EMPTY; /* Don't initialize here */
static int shift_on_press = 0;
static GResFont label_font = GRESFONT_INIT("400 10pt " SANS_UI_FAMILIES);
static GResFont button_font = GRESFONT_INIT("400 10pt " SANS_UI_FAMILIES);
#define COLOR_BUTTON_BOX_LEN	10

static GResInfo gcancel_ri, gdefault_ri, gbutton_ri, gdroplist_ri, gcolor_ri;
GResInfo glabel_ri = {
    &gbutton_ri, &ggadget_ri,NULL, NULL,
    &label_box,
    &label_font,
    NULL,
    NULL,
    N_("Label"),
    N_("Text Labels"),
    "GLabel",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_border_width|omf_padding,
    { bt_none, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

static GTextInfo button_lab[] = {
    { (unichar_t *) "Disabled Button", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Enabled Button" , NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' }
};
static GGadgetCreateData button_gcd[] = {
    { GButtonCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &button_lab[0], { NULL }, gg_visible, NULL, NULL }, NULL, NULL },
    { GButtonCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &button_lab[1], { NULL }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *barray[] = { GCD_Glue, &button_gcd[0], GCD_Glue, &button_gcd[1], GCD_Glue, NULL, NULL };
static GGadgetCreateData buttonbox =
	{GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) barray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
static struct resed gbutton_re[] = {
    { N_("Shift On Press"), "ShiftOnPress", rt_bool, &shift_on_press, N_("When true the button will move slightly when pressed"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
static GResInfo gbutton_ri = {
    &gdefault_ri, &ggadget_ri,&gdefault_ri, &gcancel_ri,
    &_GGadget_button_box,
    &button_font,
    &buttonbox,
    gbutton_re,
    N_("Button"),
    N_("Buttons"),
    "GButton",
    "Gdraw",
    true,
    false,
    box_foreground_border_inner|box_foreground_border_outer|
	/*box_active_border_inner|*/box_do_depressed_background|box_draw_default,
    /* Will be initialized later */
    { bt_box, bs_roundrect, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GGadgetCreateData def_gcd[] = {
    { GButtonCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &button_lab[0], { NULL }, gg_visible|gg_but_default, NULL, NULL }, NULL, NULL },
    { GButtonCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &button_lab[1], { NULL }, gg_visible|gg_enabled|gg_but_default, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *darray[] = { GCD_Glue, &def_gcd[0], GCD_Glue, &def_gcd[1], GCD_Glue, NULL, NULL };
static GGadgetCreateData defbox =
	{ GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) darray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
static GResInfo gdefault_ri = {
    &gcancel_ri, &gbutton_ri,&gcancel_ri,NULL,
    &_GGadget_defaultbutton_box,
    NULL,
    &defbox,
    NULL,
    N_("Default Button"),
    N_("Default Buttons"),
    "GDefaultButton",
    "Gdraw",
    true,
    false,
    0,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GGadgetCreateData cancel_gcd[] = {
    { GButtonCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &button_lab[0], { NULL }, gg_visible|gg_but_cancel, NULL, NULL }, NULL, NULL },
    { GButtonCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &button_lab[1], { NULL }, gg_visible|gg_enabled|gg_but_cancel, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *carray[] = { GCD_Glue, &cancel_gcd[0], GCD_Glue, &cancel_gcd[1], GCD_Glue, NULL, NULL };
static GGadgetCreateData cancelbox =
    {GHVGroupCreate, { {2, 2, 0, 0}, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) carray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
static GResInfo gcancel_ri = {
    &gcolor_ri, &gbutton_ri,&gdefault_ri,NULL,
    &_GGadget_cancelbutton_box,
    NULL,
    &cancelbox,
    NULL,
    N_("Cancel Button"),
    N_("Cancel Buttons"),
    "GCancelButton",
    "Gdraw",
    true,
    false,
    0,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GGadgetCreateData color_gcd[] = {
    { GColorButtonCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) (intptr_t) (0x000000) }, gg_visible, NULL, NULL }, NULL, NULL },
    { GColorButtonCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) (intptr_t) (0x000000) }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *colarray[] = { GCD_Glue, &color_gcd[0], GCD_Glue, &color_gcd[1], GCD_Glue, NULL, NULL };
static GGadgetCreateData colorbox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) colarray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
static GResInfo gcolor_ri = {
    &gdroplist_ri, &gbutton_ri,NULL,NULL,
    &_GGadget_colorbutton_box,
    NULL,
    &colorbox,
    NULL,
    N_("Color Button"),
    N_("Color Button"),
    "GColorButton",
    "Gdraw",
    true,
    false,
    0,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GTextInfo list_choices[] = {
    { (unichar_t *) "1", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "2", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "3", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};

static int GDropListRIInit(GResInfo *ri) {
    if ( ri->is_initialized )
	return false;
    return _GResEditInitialize(ri);
}

static GGadgetCreateData droplist_gcd[] = {
    { GListButtonCreate, { { 0, 0, 80, 0 }, NULL, 0, 0, 0, 0, 0, &list_choices[0], { list_choices }, gg_visible, NULL, NULL }, NULL, NULL },
    { GListButtonCreate, { { 0, 0, 80, 0 }, NULL, 0, 0, 0, 0, 0, &list_choices[1], { list_choices }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *dlarray[] = { GCD_Glue, &droplist_gcd[0], GCD_Glue, &droplist_gcd[1], GCD_Glue, NULL, NULL };
static GGadgetCreateData droplistbox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) dlarray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
extern GResInfo gradio_ri;
static GResInfo gdroplist_ri = {
    &gradio_ri, &gbutton_ri,&listmark_ri,NULL,
    &_GGadget_droplist_box,
    NULL,
    &droplistbox,
    NULL,
    N_("Drop List Button"),
    N_("Drop List Button"),
    "GDropList",
    "Gdraw",
    true,
    false,
    0,
    { bt_none, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0 },
    GBOX_EMPTY,
    NULL,
    GDropListRIInit,
    NULL
};

static void GButtonInvoked(GButton *b,GEvent *ev) {
    GEvent e;

    if ( b->labeltype==3 ) {
	struct hslrgba hsl;
	gColor2Hslrgba(&hsl,((GColorButton *) b)->col);
	hsl = GWidgetColorA(_("Pick a color"),&hsl,NULL);
	if ( hsl.rgb ) {
	    ((GColorButton *) b)->col = gHslrgba2Color(&hsl);
	    GGadgetRedraw(&b->g);
	} else
return;
    }
    e.type = et_controlevent;
    e.w = b->g.base;
    e.u.control.subtype = et_buttonactivate;
    e.u.control.g = &b->g;
    if ( ev!=NULL && ev->type==et_mouseup ) {
	e.u.control.u.button.clicks = ev->u.mouse.clicks;
	e.u.control.u.button.button = ev->u.mouse.button;
	e.u.control.u.button.state = ev->u.mouse.state;
    } else {
	e.u.control.u.button.clicks = 0;
	e.u.control.u.button.button = 0;
	e.u.control.u.button.state = 0;
    }
    if ( b->g.handle_controlevent != NULL )
	(b->g.handle_controlevent)(&b->g,&e);
    else
	GDrawPostEvent(&e);
}

static void GButtonPressed(GButton *b) {
    GEvent e;

    if ( b->labeltype==2 && ((GListButton *) b)->ltot>0 )
	GListButtonDoPopup((GListButton *) b);
    else {
	e.type = et_controlevent;
	e.w = b->g.base;
	e.u.control.subtype = et_buttonpress;
	e.u.control.g = &b->g;
	if ( b->g.handle_controlevent != NULL )
	    (b->g.handle_controlevent)(&b->g,&e);
	else
	    GDrawPostEvent(&e);
    }
}

static int gbutton_stringsize( GButton *gb,unichar_t *label,int *lcnt ) {
    int maxtextwidth = 0;
    unichar_t *pt, *start;

    for ( pt = label; ; ) {
        for ( start=pt; *pt!='\0' && *pt!='\n'; ++pt );
        if ( pt!=start ) {
	    int w = GDrawGetTextWidth(gb->g.base,start,pt-start);
	    if ( w>maxtextwidth ) maxtextwidth=w;
        }
        ++(*lcnt);
        if ( *pt=='\0' )
    break;
        ++pt;
    }
return maxtextwidth;
}

static unichar_t *gbutton_textsize( GButton *gb, int *_maxlcnt, int *_maxw ) {
    int i, lcnt, maxlcnt, maxtextwidth;
    unichar_t *ltxt;
    GFont *old;

    old = GDrawSetFont(gb->g.base,gb->font);

    maxtextwidth = maxlcnt = 0;
    if ( gb->label!=NULL ) {
	maxtextwidth = gbutton_stringsize( gb,gb->label,&maxlcnt );
	ltxt = gb->label;
    }
    /* If there is a list of alternate labels, iterate through all */
    /* and get the maximum width and height */
    if ( gb->labeltype == 0 && gb->ti!=NULL ) {
	for ( i=0; i<gb->ltot; i++ ) {
	    lcnt = 0;
	    int w = gbutton_stringsize( gb,gb->ti[i]->text,&lcnt );
	    if ( w>maxtextwidth ) {
		maxtextwidth=w;
		ltxt = gb->ti[i]->text;
	    }
	    if ( lcnt>maxlcnt ) maxlcnt=lcnt;
	}
    }
    (void) GDrawSetFont(gb->g.base,old);
    *_maxlcnt = maxlcnt;
    *_maxw = maxtextwidth;
return( ltxt );
}

static int gbutton_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GImageButton *gb = (GImageButton *) g;
    int off = gb->within && gb->shiftonpress ? gb->pressed : 0;
    int x = g->inner.x + off;
    GImage *img = gb->image;
    GRect old1, old2;
    int width;
    int marklen = GDrawPointsToPixels(pixmap,_GListMarkSize),
	    spacing = GDrawPointsToPixels(pixmap,_GGadget_TextImageSkip);
    int yoff;
    GRect unpadded_inner;
    int pad, lcnt, maxtextwidth;
    unichar_t *pt, *start;
    int cbbl=0;

    if ( g->state == gs_invisible )
return( false );
    else if ( gb->labeltype!=1 /* Not image buttons */ )
	/* Do Nothing */;
    else if ( g->state == gs_disabled ) {
	if ( gb->disabled != NULL ) img = gb->disabled;
    } else if ( gb->pressed && gb->within && gb->active!=NULL )
	img = gb->active;
    else if ( gb->within )
	img = gb->img_within;

    GDrawPushClip(pixmap,&g->r,&old1);

    GBoxDrawBackground(pixmap,&g->r,g->box,
	    gb->within && gb->pressed? gs_pressedactive: g->state,gb->is_default);
    if ( g->box->border_type!=bt_none ||
	    (g->box->flags&(box_foreground_border_inner|box_foreground_border_outer|box_active_border_inner))!=0 ) {
	GBoxDrawBorder(pixmap,&g->r,g->box,g->state,gb->is_default);

	unpadded_inner = g->inner;
	pad = GDrawPointsToPixels(g->base,g->box->padding);
	unpadded_inner.x -= pad; unpadded_inner.y -= pad;
	unpadded_inner.width += 2*pad; unpadded_inner.height += 2*pad;
	GDrawPushClip(pixmap,&unpadded_inner,&old2);
    }
    if ( gb->font!=NULL )
	GDrawSetFont(pixmap,gb->font);

    gbutton_textsize((GButton *) gb,&lcnt,&maxtextwidth);
    yoff = (g->inner.height-lcnt*gb->fh)/2;
    if ( lcnt>1 && yoff<0 )
	yoff = 0;

    if ( gb->g.takes_input ) {
	width = 0;
	if ( img!=NULL ) {
	    width = GImageGetScaledWidth(pixmap,img);
	    if ( gb->label!=NULL )
		width += spacing;
	}
	if ( gb->label!=NULL )
	    width += maxtextwidth;
	if ( gb->labeltype==3 ) {
	    cbbl = GDrawPointsToPixels(NULL,COLOR_BUTTON_BOX_LEN);
	    width += cbbl + spacing;
	}
	if ( width<=g->inner.width )
	    x += ( g->inner.width-width )/2;
	else
	    x += (g->inner.y-g->r.y);
    }
    if ( gb->labeltype==3 ) {
	GRect r;
	Color fg = g->state==gs_disabled?g->box->disabled_foreground:
			g->box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
			g->box->main_foreground;
	r.x = x+1; r.y = g->inner.y+1;
	r.width = cbbl-2;
	if ( img!=NULL )
	    r.height = GImageGetScaledHeight(pixmap,img)-2;
	else
	    r.height = gb->fh-2;
	if ( ((GColorButton *) gb)->col!=COLOR_TRANSPARENT )
	    GDrawFillRect(pixmap,&r,((GColorButton *) gb)->col);
	GDrawDrawRect(pixmap,&r,fg);
	x += cbbl + spacing;
    }
    if ( gb->image_precedes && img!=NULL ) {
	GDrawDrawScaledImage(pixmap,img,x,g->inner.y + off);
	x += GImageGetScaledWidth(pixmap,img) + spacing;
    }
    if ( gb->label!=NULL ) {
	Color fg = g->state==gs_disabled?g->box->disabled_foreground:
			g->box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
			g->box->main_foreground;
	if ( lcnt==1 ) {
	    _ggadget_underlineMnemonic(pixmap,x,g->inner.y + gb->as + yoff + off,gb->label,
		    g->mnemonic,fg,g->inner.y+g->inner.height);
	    x += GDrawDrawText(pixmap,x,g->inner.y + gb->as + yoff + off,gb->label,-1,fg);
	} else {
	    int y = g->inner.y + gb->as + yoff + off;
	    for ( pt = gb->label; ; ) {
		for ( start=pt; *pt!='\0' && *pt!='\n'; ++pt );
		if ( pt!=start )
		    GDrawDrawText(pixmap,x,y,start,pt-start,fg);
		if ( *pt=='\0' )
	    break;
		++pt;
		y+=gb->fh;
	    }
	    x += maxtextwidth;
	}
	x += spacing;
    }
    if ( !gb->image_precedes && img!=NULL )
	GDrawDrawScaledImage(pixmap,img,x,g->inner.y + off);

    if ( g->box->border_type!=bt_none ||
	    (g->box->flags&(box_foreground_border_inner|box_foreground_border_outer|box_active_border_inner))!=0 )
	GDrawPopClip(pixmap,&old2);

    if ( gb->labeltype==2 ) {
	int bp = GBoxBorderWidth(g->base,g->box);
GListMarkDraw(pixmap,
		g->r.x + g->r.width - marklen - spacing/2 - bp,
		g->inner.y,
		g->inner.height,
		g->state);
    }

    GDrawPopClip(pixmap,&old1);
return( true );
}

static int gbutton_mouse(GGadget *g, GEvent *event) {
    GLabel *gb = (GLabel *) g;
    int within = gb->within, pressed = gb->pressed;
    int was_state;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused )) {
	if ( !g->takes_input && event->type == et_mousemove && !gb->pressed &&
		g->popup_msg )
	    GGadgetPreparePopup(g->base,g->popup_msg);
return( false );
    }

    if ( gb->labeltype==2 && event->type==et_mousedown &&
	    ((GListButton *) gb)->popup!=NULL ) {
	GDrawDestroyWindow(((GListButton *) gb)->popup);
	((GListButton *) gb)->popup = NULL;
return( true );
    }
    was_state = g->state;
    if ( event->type == et_crossing ) {
	if ( gb->within && !event->u.crossing.entered )
	    gb->within = false;
	if ( gb->pressed && (event->u.crossing.state&ksm_buttons)==0 )
	    gb->pressed = false;
    } else if ( gb->pressed && event->type==et_mouseup ) {
	if ( GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y)) {
	    if ( event->type == et_mouseup ) {
		gb->pressed = false;
		GButtonInvoked(gb,event);
	    } else
		gb->within = false;
	} else
	    gb->pressed = false;
    } else if ( event->type == et_mousedown &&
	    GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y)) {
	if (event->u.mouse.button == 1) {
	    gb->pressed = true;
	    gb->within = true;
	    GButtonPressed(gb);
	}
    } else if ( event->type == et_mousemove &&
	    GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y)) {
	gb->within = true;
	if ( !gb->pressed && g->popup_msg )
	    GGadgetPreparePopup(g->base,g->popup_msg);
    } else if ( event->type == et_mousemove && gb->within ) {
	gb->within = false;
    } else {
return( false );
    }
    if ( within != gb->within && was_state==g->state )
	g->state = gb->within? gs_active : gs_enabled;
    if ( within != gb->within || pressed != gb->pressed )
	_ggadget_redraw(g);
return( event->type==et_mousedown || event->type==et_mouseup || gb->within );
}

static int gbutton_key(GGadget *g, GEvent *event) {
    GLabel *gb = (GLabel *) g;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active ))
return(false);
    if ( gb->labeltype==2 && ((GListButton *) gb)->popup!=NULL ) {
	GWindow popup = ((GListButton *) gb)->popup;
	(GDrawGetEH(popup))(popup,event);
return( true );
    }

    if ( event->u.chr.chars[0]==' ' ) {
	GButtonInvoked(gb,NULL);
return( true );
    }
return(false);
}

static int gbutton_focus(GGadget *g, GEvent *event) {
    GLabel *gb = (GLabel *) g;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active ))
return(false);

    if ( event->u.focus.mnemonic_focus==mf_shortcut ||
	    event->u.focus.mnemonic_focus==mf_mnemonic ) {
	GButtonInvoked(gb,NULL);
return( true );
    }
return( true );
}

static void gbutton_destroy(GGadget *g) {
    GButton *b = (GButton *) g;

    if ( b==NULL )
return;
    if ( b->labeltype==2 ) {
	GListButton *glb = (GListButton *) g;
	if ( glb->popup ) {
	    /* Must cleanup the popup before we die  */
	    /* We do this instead of GDrawDestroyWindow because this method is synchronous */
	    GEvent die;
	    die.type = et_close;
	    die.w = glb->popup;
	    GDrawPostEvent(&die);
	}
	GTextInfoArrayFree(glb->ti);
    }
    free(b->label);
    _ggadget_destroy(g);
}

static int GButtonGetDesiredWidth(GLabel *gl);
static void GButtonSetInner(GButton *b) {
    int width, mark = 0;
    int bp = GBoxBorderWidth(b->g.base,b->g.box);

    if ( b->labeltype==2 )
	mark = GDrawPointsToPixels(b->g.base,_GListMarkSize) +
		GDrawPointsToPixels(b->g.base,_GGadget_TextImageSkip);
    width = GButtonGetDesiredWidth(b);
    if ( width<=b->g.r.width-2*bp-mark )
	b->g.inner.width = width;
    else
	b->g.inner.width = b->g.r.width-2*bp;
    if ( !b->g.takes_input )
	b->g.inner.x = b->g.r.x + bp;
    else
	b->g.inner.x = b->g.r.x + (b->g.r.width-b->g.inner.width-mark)/2;
}

static void GButtonSetTitle(GGadget *g,const unichar_t *tit) {
    GButton *b = (GButton *) g;

    if ( b->g.free_box )
	free( b->g.box );
    free(b->label);
    b->label = u_copy(tit);
    GButtonSetInner(b);
    _ggadget_redraw(g);
}

static void GButtonSetImageTitle(GGadget *g,GImage *img,const unichar_t *tit, int before) {
    GButton *b = (GButton *) g;

    if ( b->g.free_box )
	free( b->g.box );
    free(b->label);
    b->label = u_copy(tit);
    b->image = img;
    b->image_precedes = before;

    GButtonSetInner(b);

    _ggadget_redraw(g);
}

static const unichar_t *_GButtonGetTitle(GGadget *g) {
    GButton *b = (GButton *) g;
return( b->label );
}

static GImage *GButtonGetImage(GGadget *g) {
    GButton *b = (GButton *) g;
return( b->image );
}

static void GButtonSetFont(GGadget *g,FontInstance *new) {
    GButton *b = (GButton *) g;
    b->font = new;
}

static FontInstance *GButtonGetFont(GGadget *g) {
    GButton *b = (GButton *) g;
return( b->font );
}

static void GListBSelectOne(GGadget *g, int32_t pos) {
    GListButton *gl = (GListButton *) g;
    int i;

    for ( i=0; i<gl->ltot; ++i )
	gl->ti[i]->selected = false;
    if ( pos>=gl->ltot ) pos = gl->ltot-1;
    if ( pos<0 ) pos = 0;
    if ( gl->ltot>0 ) {
	gl->ti[pos]->selected = true;
	GButtonSetImageTitle(g,gl->ti[pos]->image,gl->ti[pos]->text,gl->ti[pos]->image_precedes);
    }
}

static int32_t GListBIsSelected(GGadget *g, int32_t pos) {
    GListButton *gl = (GListButton *) g;

    if ( pos>=gl->ltot )
return( false );
    if ( pos<0 )
return( false );
    if ( gl->ltot>0 )
return( gl->ti[pos]->selected );

return( false );
}

static int32_t GListBGetFirst(GGadget *g) {
    int i;
    GListButton *gl = (GListButton *) g;

    for ( i=0; i<gl->ltot; ++i )
	if ( gl->ti[i]->selected )
return( i );

return( -1 );
}

static GTextInfo **GListBGet(GGadget *g,int32_t *len) {
    GListButton *gl = (GListButton *) g;
    if ( len!=NULL ) *len = gl->ltot;
return( gl->ti );
}

static GTextInfo *GListBGetItem(GGadget *g,int32_t pos) {
    GListButton *gl = (GListButton *) g;
    if ( pos<0 || pos>=gl->ltot )
return( NULL );

return(gl->ti[pos]);
}

static void GListButSet(GGadget *g,GTextInfo **ti,int32_t docopy) {
    GListButton *gl = (GListButton *) g;
    int i;

    GTextInfoArrayFree(gl->ti);
    if ( docopy || ti==NULL )
	ti = GTextInfoArrayCopy(ti);
    gl->ti = ti;
    gl->ltot = GTextInfoArrayCount(ti);
    for ( i=0; ti[i]->text!=NULL || ti[i]->line; ++i ) {
	if ( ti[i]->selected && ti[i]->text!=NULL ) {
	    GGadgetSetTitle(g,ti[i]->text);
    break;
	}
    }
}

static void GListButClear(GGadget *g) {
    GListButSet(g,NULL,true);
}

static int GButtonIsDefault(GGadget *g) {
    GLabel *gl = (GLabel *) g;
return( gl->is_default );
}

static int GButtonGetDesiredWidth(GLabel *gl) {
    int iwidth=0, width=0;
    if ( gl->image!=NULL ) {
	iwidth = GImageGetScaledWidth(gl->g.base,gl->image);
    }
    if ( gl->label!=NULL ) {
	int lcnt;
	gbutton_textsize(gl,&lcnt,&width);
    }

    if ( width!=0 && iwidth!=0 )
	width += GDrawPointsToPixels(gl->g.base,_GGadget_TextImageSkip);
    width += iwidth;

    if ( gl->labeltype==3 )
	width += GDrawPointsToPixels(gl->g.base,_GGadget_TextImageSkip) +
		GDrawPointsToPixels(gl->g.base,COLOR_BUTTON_BOX_LEN);
return( width );
}

static void GButtonGetDesiredSize(GGadget *g, GRect *outer, GRect *inner) {
    GLabel *gl = (GLabel *) g;
    int iwidth=0, iheight=0;
    GTextBounds bounds;
    int as=0, ds, ld, fh=0, width=0;
    GRect needed;
    int i;
    int bp = GBoxBorderWidth(g->base,g->box);

    if ( gl->image!=NULL ) {
	iwidth = GImageGetScaledWidth(gl->g.base,gl->image);
	iheight = GImageGetScaledHeight(gl->g.base,gl->image);
    }
    GDrawWindowFontMetrics(g->base,gl->font,&as, &ds, &ld);
    if ( gl->label!=NULL ) {
	int lcnt;
	unichar_t *ltxt = NULL;
	ltxt = gbutton_textsize(gl,&lcnt,&width);
	if ( lcnt==1 ) {
	    FontInstance *old = GDrawSetFont(gl->g.base,gl->font);
	    width = GDrawGetTextBounds(gl->g.base,ltxt, -1, &bounds);
	    GDrawSetFont(gl->g.base,old);
	    if ( as<bounds.as ) as = bounds.as;
	    if ( ds<bounds.ds ) ds = bounds.ds;
	    fh = as+ds;
	} else
	    fh = gl->fh*lcnt;
    } else
	fh = as+ds;

    if ( gl->labeltype==3 ) {
	if ( width!=0 )
	    width += GDrawPointsToPixels(NULL,COLOR_BUTTON_BOX_LEN) +
		    GDrawPointsToPixels(gl->g.base,_GGadget_TextImageSkip);
	else
	    width = GDrawPointsToPixels(NULL,COLOR_BUTTON_BOX_LEN);
    }

    if ( width!=0 && iwidth!=0 )
	width += GDrawPointsToPixels(gl->g.base,_GGadget_TextImageSkip);
    width += iwidth;
    if ( iheight<fh )
	iheight = fh;

    if ( gl->labeltype==2 ) {
	GListButton *glb = (GListButton *) gl;
	int temp;
	int extra = GDrawPointsToPixels(gl->g.base,_GListMarkSize) +
		    2*GDrawPointsToPixels(gl->g.base,_GGadget_TextImageSkip) +
		    GBoxBorderWidth(gl->g.base,&_GListMark_Box);
	for ( i=0; i<glb->ltot; ++i ) {
	    temp = GTextInfoGetWidth(gl->g.base,glb->ti[i],gl->font) + extra;
	    if ( temp>width ) width = temp;
	    temp = GTextInfoGetHeight(gl->g.base,glb->ti[i],gl->font);
	    if ( temp>iheight )
		iheight = temp;
	}
	width += GDrawPointsToPixels(gl->g.base,_GGadget_TextImageSkip) +
		GDrawPointsToPixels(gl->g.base,_GListMarkSize);
    }

    if ( gl->shiftonpress ) {
	++width; ++iheight;		/* one pixel for movement when button is pushed */
    }

    width += gl->g.takes_input?2*GDrawPointsToPixels(gl->g.base,2):0;
    needed.x = needed.y = 0;
    needed.width = width;
    needed.height = iheight;
    if ( g->desired_width>2*bp )
	needed.width = g->desired_width-2*bp;
    if ( g->desired_height>2*bp )
	needed.height = g->desired_height-2*bp;
    if ( inner!=NULL ) {
	inner->x = inner->y = 0;
	inner->width = width;
	inner->height = iheight;
    }

    _ggadgetFigureSize(gl->g.base,gl->g.box,&needed,gl->is_default);

    if ( outer!=NULL ) {
	outer->x = outer->y = 0;
	outer->width = needed.width;
	outer->height = needed.height;
    }
}

static void _gbutton_resize(GGadget *g, int32_t width, int32_t height ) {
    GRect inner;
    int bp = GBoxBorderWidth(g->base,g->box);

    GButtonGetDesiredSize(g,NULL,&inner);
    if ( inner.height<height-2*bp ) inner.height = height-2*bp;
    
    g->inner.height = inner.height;
    g->inner.y = g->r.y + (height-inner.height)/2;
    g->r.width = width;
    g->r.height = height;
    GButtonSetInner((GButton *) g);
}

struct gfuncs gbutton_funcs = {
    0,
    sizeof(struct gfuncs),

    gbutton_expose,
    gbutton_mouse,
    gbutton_key,
    NULL,
    gbutton_focus,
    NULL,
    NULL,

    _ggadget_redraw,
    _ggadget_move,
    _gbutton_resize,
    _ggadget_setvisible,
    _ggadget_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    gbutton_destroy,

    GButtonSetTitle,
    _GButtonGetTitle,
    NULL,
    GButtonSetImageTitle,
    GButtonGetImage,

    GButtonSetFont,
    GButtonGetFont,

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

    GButtonGetDesiredSize,
    _ggadget_setDesiredSize,
    NULL,
    GButtonIsDefault
};

struct gfuncs glistbutton_funcs = {
    0,
    sizeof(struct gfuncs),

    gbutton_expose,
    gbutton_mouse,
    gbutton_key,
    NULL,
    gbutton_focus,
    NULL,
    NULL,

    _ggadget_redraw,
    _ggadget_move,
    _gbutton_resize,
    _ggadget_setvisible,
    _ggadget_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    gbutton_destroy,

    GButtonSetTitle,
    _GButtonGetTitle,
    NULL,
    GButtonSetImageTitle,
    GButtonGetImage,

    GButtonSetFont,
    GButtonGetFont,

    GListButClear,
    GListButSet,
    GListBGet,
    GListBGetItem,
    NULL,
    GListBSelectOne,
    GListBIsSelected,
    GListBGetFirst,
    NULL,
    NULL,
    NULL,

    GButtonGetDesiredSize,
    _ggadget_setDesiredSize,		/* GTextField does this right. but this is good enough for now */
    NULL,
    NULL
};

void _GButton_SetDefault(GGadget *g,int32_t is_default) {
    GButton *gb = (GButton *) g;
    GRect maxr;
    int scale = GDrawPointsToPixels(g->base,1);
    int def_size = (g->box->flags & box_draw_default)?scale+GDrawPointsToPixels(g->base,2):0;

    if ( gb->is_default == is_default )
return;
    gb->is_default = is_default;
    if ( def_size==0 )
return;
    if ( is_default ) {
	g->r.x -= def_size;
	g->r.y -= def_size;
	g->r.width += 2*def_size;
	g->r.height += 2*def_size;
	maxr = g->r;
    } else {
	maxr = g->r;
	g->r.x += def_size;
	g->r.y += def_size;
	g->r.width -= 2*def_size;
	g->r.height -= 2*def_size;
    }
    ++maxr.width; ++maxr.height;
    GDrawRequestExpose(g->base, &maxr, false);
}

void _GButtonInit(void) {
   GResEditDoInit(&glabel_ri);
   GResEditDoInit(&gbutton_ri);
   GResEditDoInit(&gdroplist_ri);
   GResEditDoInit(&gdefault_ri);
   GResEditDoInit(&gcancel_ri);
   GResEditDoInit(&gcolor_ri);
}

static void GLabelFit(GLabel *gl) {
    int as=0, ds, ld;
    GRect outer,inner;

    if ( gl->g.r.width == -1 ) {
	gl->g.r.width = GDrawPointsToPixels(gl->g.base,GIntGetResource(_NUM_Buttonsize));
	if ( gl->is_default )
	    gl->g.r.width += 6;
    }

    GDrawWindowFontMetrics(gl->g.base,gl->font,&as, &ds, &ld);
    gl->as = as;
    gl->fh = as+ds;

    GButtonGetDesiredSize(&gl->g,&outer, &inner);
    _ggadgetSetRects(&gl->g,&outer, &inner, 0, 0);
    if ( gl->g.takes_input )
	GButtonSetInner((GButton *) gl);
}

static GLabel *_GLabelCreate(GLabel *gl, struct gwindow *base, GGadgetData *gd,void *data, GBox *def) {
    _GButtonInit();
    gl->g.funcs = &gbutton_funcs;
    _GGadget_Create(&gl->g,base,gd,data,def);

    if (( gl->is_default = gd->flags&gg_but_default?1:0 ) )
	_GWidget_SetDefaultButton(&gl->g);
    if (( gl->is_cancel = gd->flags&gg_but_cancel?1:0 ))
	_GWidget_SetCancelButton(&gl->g);
    gl->font = def==&label_box ? label_font.fi : button_font.fi;
    if ( gd->label!=NULL ) {
	gl->image_precedes = gd->label->image_precedes;
	if ( gd->label->font!=NULL )
	    gl->font = gd->label->font;
	if ( gd->label->text_is_1byte )
	    gl->label = utf82u_mncopy((char *) gd->label->text,&gl->g.mnemonic);
	else
	    gl->label = u_copy(gd->label->text);
	gl->image = gd->label->image;
	if ( gd->mnemonic )
	    gl->g.mnemonic = gd->mnemonic;
    }
    gl->shiftonpress = shift_on_press;
    GLabelFit(gl);
    _GGadget_FinalPosition(&gl->g,base,gd);

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&gl->g);
return( gl );
}

GGadget *GLabelCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GLabel *gl = calloc(1,sizeof(GListButton));
    int i;

    if ( gd->u.list!=NULL ) {
	gl->ti = GTextInfoArrayFromList(gd->u.list,&gl->ltot);
    }
    if ( gd->label==NULL && gd->u.list!=NULL ) {
	for ( i=0; gd->u.list[i].text == NULL; ++i );
	if ( gd->u.list[i].text!=NULL )
	    gd->label = &gd->u.list[i];
    }
    gl = _GLabelCreate(gl,base,gd,data,&label_box);
return( &gl->g );
}

GGadget *GButtonCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GLabel *gl = _GLabelCreate(calloc(1,sizeof(GLabel)),base,gd,data,
	    gd->flags & gg_but_default ? &_GGadget_defaultbutton_box :
	    gd->flags & gg_but_cancel ? &_GGadget_cancelbutton_box :
	    &_GGadget_button_box);

    gl->g.takes_input = true; gl->g.takes_keyboard = true; gl->g.focusable = true;
return( &gl->g );
}

GGadget *GImageButtonCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GImageButton *gl =
	(GImageButton *) _GLabelCreate(calloc(1,sizeof(GImageButton)),base,gd,data,&_GGadget_button_box);

    gl->g.takes_input = true;
    gl->labeltype = 1;
    if ( gd->label!=NULL ) {
	gl->img_within = gd->label[1].image;
	gl->active = gd->label[2].image;
	gl->disabled = gd->label[3].image;
    }
return( &gl->g );
}

GGadget *GColorButtonCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GColorButton *gl;
    static GTextInfo ti;
    Color col = gd->u.col;

    if ( ti.image==NULL && ti.text==NULL ) {
	ti.image = GGadgetImageCache("colorwheel.png");
	if ( ti.image==NULL ) {
	    ti.text = (unichar_t *) _("Color");
	    ti.text_is_1byte = true;
	}
    }
    gd->label = &ti;
    gl = calloc(1,sizeof(GColorButton));
    gl->labeltype = 3;
    gl = (GColorButton *) _GLabelCreate((GLabel *) gl,base,gd,data,&_GGadget_colorbutton_box);
    gl->g.takes_input = true;
    gl->col = col;
return( &gl->g );
}

void GColorButtonSetColor(GGadget *g, Color col) {
    GColorButton *gb = (GColorButton *) g;
    gb->col = col;
    GGadgetRedraw(g);
}

Color GColorButtonGetColor(GGadget *g) {
return( ((GColorButton *) g)->col );
}

static void GListButtonSelected(GGadget *g, int i) {
    GListButton *gl = (GListButton *) g;
    GEvent e;

    gl->popup = NULL;
    _GWidget_ClearGrabGadget(&gl->g);
    if ( i<0 || i>=gl->ltot )
return;
    free(gl->label); gl->label = u_copy(gl->ti[i]->text);
    gl->image = gl->ti[i]->image;
    gl->image_precedes = gl->ti[i]->image_precedes;
    GButtonSetInner((GButton *) gl);
    _ggadget_redraw(g);

    e.type = et_controlevent;
    e.w = g->base;
    e.u.control.subtype = et_listselected;
    e.u.control.g = g;
    e.u.control.u.list.from_mouse = true;
    if ( gl->g.handle_controlevent != NULL )
	(gl->g.handle_controlevent)(&gl->g,&e);
    else
	GDrawPostEvent(&e);
}

static void GListButtonDoPopup(GListButton *gl) {
    gl->within = false; gl->pressed = false;
    gl->popup = GListPopupCreate(&gl->g,GListButtonSelected,gl->ti);
}

static int _GListAlphaCompare(const void *v1, const void *v2) {
    GTextInfo * const *pt1 = v1, * const *pt2 = v2;
return( GTextInfoCompare(*pt1,*pt2));
}

GGadget *GListButtonCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GListButton *gl = calloc(1,sizeof(GListButton));
    int i;

    gl->labeltype = 2;
    gl->g.takes_input = true;
    if ( gd->u.list!=NULL ) {
	gl->ti = GTextInfoArrayFromList(gd->u.list,&gl->ltot);
	if ( gd->flags & gg_list_alphabetic )
	    qsort(gl->ti,gl->ltot,sizeof(GTextInfo *),_GListAlphaCompare);
    }
    if ( gd->label==NULL && gd->u.list!=NULL ) {
	/* find first selected item if there is one */
	for ( i=0; gd->u.list[i].text!=NULL || gd->u.list[i].line; ++i )
	    if ( gd->u.list[i].selected )
	break;
	/* else first item with text */
	if ( gd->u.list[i].text==NULL && !gd->u.list[i].line ) {
	    for ( i=0; gd->u.list[i].line; ++i );
	    if ( gd->u.list[i].text==NULL && !gd->u.list[i].line )
		i = 0;
	}
	gd->label = &gd->u.list[i];
    }
    _GLabelCreate((GLabel *) gl,base,gd,data,&_GGadget_droplist_box);
    gl->g.funcs = &glistbutton_funcs;
return( &gl->g );
}
