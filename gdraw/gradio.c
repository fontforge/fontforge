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
#include "ustring.h"

#include <math.h>

static GBox radio_box = GBOX_EMPTY; /* Don't initialize here */
static GBox radio_on_box = GBOX_EMPTY; /* Don't initialize here */
static GBox radio_off_box = GBOX_EMPTY; /* Don't initialize here */
static GBox checkbox_box = GBOX_EMPTY; /* Don't initialize here */
static GBox checkbox_on_box = GBOX_EMPTY; /* Don't initialize here */
static GBox checkbox_off_box = GBOX_EMPTY; /* Don't initialize here */
static GBox visibility_box = GBOX_EMPTY; /* Don't initialize here */
static GBox visibility_on_box = GBOX_EMPTY; /* Don't initialize here */
static GBox visibility_off_box = GBOX_EMPTY; /* Don't initialize here */
static GResImage radon = GRESIMAGE_INIT("radio_on.png");
static GResImage radoff = GRESIMAGE_INIT("radio_off.png");
static GResImage raddison = GRESIMAGE_INIT("radio_on_disabled.png");
static GResImage raddisoff = GRESIMAGE_INIT("radio_off_disabled.png");
static GResImage checkon = GRESIMAGE_INIT("check_on.png");
static GResImage checkoff = GRESIMAGE_INIT("check_off.png");
static GResImage checkdison = GRESIMAGE_INIT("check_on_disabled.png");
static GResImage checkdisoff = GRESIMAGE_INIT("check_off_disabled.png");
static GResImage visibilityon = GRESIMAGE_INIT("check_on.png");
static GResImage visibilityoff = GRESIMAGE_INIT("check_off.png");
static GResImage visibilitydison = GRESIMAGE_INIT("check_on_disabled.png");
static GResImage visibilitydisoff = GRESIMAGE_INIT("check_off_disabled.png");
static GResFont checkbox_font = GRESFONT_INIT("400 10pt " SANS_UI_FAMILIES);
static int gradio_inited = false;

GResInfo gradio_ri;
static GResInfo gradioon_ri, gradiooff_ri;
static GResInfo gcheckbox_ri, gcheckboxon_ri, gcheckboxoff_ri;
static GResInfo gvisibility_ri, gvisibilityon_ri, gvisibilityoff_ri;
static GTextInfo radio_lab[] = {
    { (unichar_t *) "Disabled On", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Disabled Off", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Enabled" , NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Enabled 2" , NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' }
};
static GGadgetCreateData radio_gcd[] = {
    { GRadioCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &radio_lab[0], { NULL }, gg_visible, NULL, NULL }, NULL, NULL },
    { GRadioCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &radio_lab[1], { NULL }, gg_visible|gg_cb_on, NULL, NULL }, NULL, NULL },
    { GRadioCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &radio_lab[2], { NULL }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL },
    { GRadioCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &radio_lab[3], { NULL }, gg_visible|gg_enabled|gg_cb_on, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *rarray[] = { GCD_Glue, &radio_gcd[0], GCD_Glue, &radio_gcd[1], GCD_Glue, &radio_gcd[2], GCD_Glue, &radio_gcd[3], GCD_Glue, NULL, NULL };
static GGadgetCreateData radiobox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) rarray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
GResInfo gradio_ri = {
    &gradioon_ri, &ggadget_ri, &gradioon_ri, &gradiooff_ri,
    &radio_box,
    NULL,
    &radiobox,
    NULL,
    N_("Radio Button"),
    N_("Radio Button"),
    "GRadio",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_padding,
    { bt_none, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static struct resed gradioon_re[] = {
    { N_("Image"), "Image", rt_image, &radon, N_("Image used instead of the Radio On Mark"), NULL, { 0 }, 0, 0 },
    { N_("Disabled Image"), "DisabledImage", rt_image, &raddison, N_("Image used instead of the Radio On Mark (when the radio is disabled)"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
static GResInfo gradioon_ri = {
    &gradiooff_ri, &ggadget_ri,&gradiooff_ri, &gradio_ri,
    &radio_on_box,
    NULL,
    &radiobox,
    gradioon_re,
    N_("Radio On Mark"),
    N_("The mark showing a radio button is on (depressed, selected)"),
    "GRadioOn",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_border_shape|box_do_depressed_background,
    { bt_raised, bs_diamond, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static struct resed gradiooff_re[] = {
    { N_("Image"), "Image", rt_image, &radoff, N_("Image used instead of the Radio Off Mark"), NULL, { 0 }, 0, 0 },
    { N_("Disabled Image"), "DisabledImage", rt_image, &raddisoff, N_("Image used instead of the Radio Off Mark (when the radio is disabled)"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
static GResInfo gradiooff_ri = {
    &gcheckbox_ri, &ggadget_ri,&gradioon_ri, &gradio_ri,
    &radio_off_box,
    NULL,
    &radiobox,
    gradiooff_re,
    N_("Radio Off Mark"),
    N_("The mark showing a radio button is off (up, not selected)"),
    "GRadioOff",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_border_shape|box_do_depressed_background,
    { bt_lowered, bs_diamond, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GTextInfo checkbox_lab[] = {
    { (unichar_t *) "Disabled On", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Disabled Off", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Enabled" , NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' }
};
static GGadgetCreateData checkbox_gcd[] = {
    { GCheckBoxCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &checkbox_lab[0], { NULL }, gg_visible, NULL, NULL }, NULL, NULL },
    { GCheckBoxCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &checkbox_lab[1], { NULL }, gg_visible|gg_cb_on, NULL, NULL }, NULL, NULL },
    { GCheckBoxCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &checkbox_lab[1], { NULL }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *carray[] = { GCD_Glue, &checkbox_gcd[0], GCD_Glue, &checkbox_gcd[1], GCD_Glue, &checkbox_gcd[2], GCD_Glue, NULL, NULL };
static GGadgetCreateData checkboxbox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) carray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
static GResInfo gcheckbox_ri = {
    &gcheckboxon_ri, &ggadget_ri,&gcheckboxon_ri, &gcheckboxoff_ri,
    &checkbox_box,
    &checkbox_font,
    &checkboxbox,
    NULL,
    N_("Check Box"),
    N_("Check Box"),
    "GCheckBox",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_padding,
    { bt_none, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static struct resed gcheckboxon_re[] = {
    {N_("Image"), "Image", rt_image, &checkon, N_("Image used instead of the Radio On Mark"), NULL, { 0 }, 0, 0 },
    {N_("Disabled Image"), "DisabledImage", rt_image, &checkdison, N_("Image used instead of the Radio On Mark (when the radio is disabled)"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
static GResInfo gcheckboxon_ri = {
    &gcheckboxoff_ri, &ggadget_ri,&gcheckboxoff_ri, &gcheckbox_ri,
    &checkbox_on_box,
    NULL,
    &checkboxbox,
    gcheckboxon_re,
    N_("Check Box On Mark"),
    N_("The mark showing a checkbox is on (depressed, selected)"),
    "GCheckBoxOn",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_border_shape|box_do_depressed_background,
    { bt_raised, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL

};
static struct resed gcheckboxoff_re[] = {
    {N_("Image"), "Image", rt_image, &checkoff, N_("Image used instead of the Check Box Off Mark"), NULL, { 0 }, 0, 0 },
    {N_("Disabled Image"), "DisabledImage", rt_image, &checkdisoff, N_("Image used instead of the Check Box Off Mark (when the checkbox is disabled)"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
static GResInfo gcheckboxoff_ri = {
    &gvisibility_ri, &ggadget_ri,&gcheckboxon_ri, &gcheckbox_ri,
    &checkbox_off_box,
    NULL,
    &checkboxbox,
    gcheckboxoff_re,
    N_("Check Box Off Mark"),
    N_("The mark showing a checkbox is off (up, not selected)"),
    "GCheckBoxOff",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_border_shape|box_do_depressed_background,
    { bt_lowered, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GTextInfo visibility_lab[] = {
    { (unichar_t *) "Disabled On", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Disabled Off", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "Enabled" , NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' }
};
static GGadgetCreateData visibility_gcd[] = {
    { GVisibilityBoxCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &visibility_lab[0], { NULL }, gg_visible, NULL, NULL }, NULL, NULL },
    { GVisibilityBoxCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &visibility_lab[1], { NULL }, gg_visible|gg_cb_on, NULL, NULL }, NULL, NULL },
    { GVisibilityBoxCreate, { GRECT_EMPTY, NULL, 0, 0, 0, 0, 0, &visibility_lab[2], { NULL }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *varray[] = { GCD_Glue, &visibility_gcd[0], GCD_Glue, &visibility_gcd[1], GCD_Glue, &visibility_gcd[2], GCD_Glue, NULL, NULL };
static GGadgetCreateData visibilitybox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) varray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
static GResInfo gvisibility_ri = {
    &gvisibilityon_ri, &ggadget_ri,&gvisibilityon_ri, &gvisibilityoff_ri,
    &visibility_box,
    NULL,
    &visibilitybox,
    NULL,
    N_("Visibility Box"),
    N_("Visibility Box"),
    "GVisibilityBox",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_padding,
    { bt_none, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static struct resed gvisibilityon_re[] = {
    {N_("Image"), "Image", rt_image, &visibilityon, N_("Image used instead of the Visibility Box On Mark"), NULL, { 0 }, 0, 0 },
    {N_("Disabled Image"), "DisabledImage", rt_image, &visibilitydison, N_("Image used instead of the Visibility Box On Mark (when the mark is disabled)"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
static GResInfo gvisibilityon_ri = {
    &gvisibilityoff_ri, &ggadget_ri,&gvisibilityoff_ri,&gvisibility_ri,
    &visibility_on_box,
    NULL,
    &visibilitybox,
    gvisibilityon_re,
    N_("Visibility Box On Mark"),
    N_("The mark showing visibliity is On (up, not selected)"),
    "GVisibilityBoxOn",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_padding,
    { bt_none, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static struct resed gvisibilityoff_re[] = {
    {N_("Image"), "Image", rt_image, &visibilityoff, N_("Image used instead of the Visibility Box Off Mark"), NULL, { 0 }, 0, 0 },
    {N_("Disabled Image"), "DisabledImage", rt_image, &visibilitydisoff, N_("Image used instead of the Visibility Box Off Mark (when the mark is disabled)"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
extern GResInfo gtextfield_ri;
static GResInfo gvisibilityoff_ri = {
    &gtextfield_ri, &ggadget_ri,&gvisibilityon_ri,&gvisibility_ri,
    &visibility_off_box,
    NULL,
    &visibilitybox,
    gvisibilityoff_re,
    N_("Visibility Box Off Mark"),
    N_("The mark showing visibility is off"),
    "GVisibilityBoxOff",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_padding,
    { bt_none, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

static void GRadioChanged(GRadio *gr) {
    GEvent e;

    if ( gr->isradio && gr->ison )
return;		/* Do Nothing, it's already on */
    else if ( gr->isradio ) {
	GRadio *other;
	for ( other=gr->post; other!=gr; other = other->post ) {
	    if ( other->ison ) {
		other->ison = false;
		_ggadget_redraw((GGadget *) other);
	    }
	}
    } else {
	/* Checkboxes just default down */
	;
    }
    gr->ison = !gr->ison;
    e.type = et_controlevent;
    e.w = gr->g.base;
    e.u.control.subtype = et_radiochanged;
    e.u.control.g = &gr->g;
    if ( gr->g.handle_controlevent != NULL )
	(gr->g.handle_controlevent)(&gr->g,&e);
    else
	GDrawPostEvent(&e);
}

/* Return the number of lines in the label of a radio button. */
static int gradio_linecount(GRadio *gr) {
    int lcnt;
    unichar_t *pt;

    lcnt = 0;
    if ( gr->label!=NULL ) {
	for ( pt = gr->label; ; ) {
	    for ( ; *pt!='\0' && *pt!='\n'; ++pt );
	    ++lcnt;
	    if ( *pt=='\0' )
	break;
	    ++pt;
	}
    }
return( lcnt );
}

/* Called on expose events, this renders the button. */
static int gradio_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GRadio *gr = (GRadio *) g;
    int x;
    GImage *img = gr->image; /* the optional image tied to the label */
    GResImage *mark;
    GRect old1, old2, old3;
    int yoff = (g->inner.height-(gr->fh))/2;

    if ( g->state == gs_invisible )
return( false );

     /* First blank out the button area. */
    GDrawPushClip(pixmap,&g->r,&old1);

    GBoxDrawBackground(pixmap,&g->r,g->box,
	    g->state==gs_enabled? gs_pressedactive: g->state,false);
    GBoxDrawBorder(pixmap,&g->r,g->box,g->state,false);

    GDrawPushClip(pixmap,&gr->onoffrect,&old2);
    GBoxDrawBackground(pixmap,&gr->onoffrect,gr->ison?gr->onbox:gr->offbox,
	    gs_pressedactive,false);
    if (gr->ison && gr->onbox->border_type!=bt_none)
        GBoxDrawBorder(pixmap,&gr->onoffrect,gr->onbox, gs_pressedactive,false);
    else if (!gr->ison && gr->offbox->border_type!=bt_none)
        GBoxDrawBorder(pixmap,&gr->onoffrect,gr->offbox,gs_pressedactive,false);

     /* Next draw either the right image or draw in an on or off indicator. */
    mark = NULL;
    if ( g->state == gs_disabled )
	mark = gr->ison ? gr->ondis : gr->offdis; /* note: ondis or offdis may be NULL! */
    if ( mark==NULL || GResImageGetImage(mark)==NULL )
	mark = gr->ison ? gr->on : gr->off; /* note: on or off may be NULL! */
    if ( mark!=NULL && GResImageGetImage(mark)==NULL ) /* when there's a reference to a special image, but no actual image */
	mark = NULL;
    if ( mark!=NULL ) {
	GDrawPushClip(pixmap,&gr->onoffinner,&old3);
	GDrawDrawScaledImage(pixmap,GResImageGetImage(mark),
		gr->onoffinner.x,gr->onoffinner.y);
	GDrawPopClip(pixmap,&old3);
    } else if ( gr->ison && gr->onbox == &checkbox_on_box ) {
	 /* for radio buttons where the on is a checkbox style, draw an X */
	Color fg = g->state==gs_disabled?g->box->disabled_foreground:
			g->box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
			g->box->main_foreground;
	int bp = GDrawPointsToPixels(pixmap,gr->onbox->border_width);
	GDrawDrawLine(pixmap, gr->onoffrect.x+bp,gr->onoffrect.y+bp,
				gr->onoffrect.x+gr->onoffrect.width-1-bp,gr->onoffrect.y+gr->onoffrect.height-1-bp,
			        fg);
	GDrawDrawLine(pixmap, gr->onoffrect.x+gr->onoffrect.width-1-bp,gr->onoffrect.y+bp,
				gr->onoffrect.x+bp,gr->onoffrect.y+gr->onoffrect.height-1-bp,
			        fg);
    } else if ( gr->ison && gr->onbox == &visibility_on_box ) {
         /* draw open white of eye */
        GPoint pts[15];
        Color fg;
        double angle;
        int c,i;
	int bp = gr->onbox->border_type==bt_none ? 0 : GDrawPointsToPixels(pixmap,gr->onbox->border_width);
        int x=gr->onoffrect.x+bp;
        int y=gr->onoffrect.y+bp;
        int w=gr->onoffrect.width -1-2*bp;
        int h=gr->onoffrect.height-1-2*bp;
        GRect rect;
        for (c=0, i=0; c<7; c++) {
            angle=(30+c/6.*120)*FF_PI/180;
            pts[i].x=.5*w*cos(angle)+x+w/2;
            pts[i].y=.5*h*sin(angle)+y+h/4;
            ++i;
        }
        for (c=1; c<6; c++) {
            angle=(180+30+c/6.*120)*FF_PI/180;
            pts[i].x=.5*w*cos(angle)+x+w/2;
            pts[i].y=.5*h*sin(angle)+y+h*3/4;
            ++i;
        }
        pts[i].x=pts[0].x;
        pts[i].y=pts[0].y;
        ++i;
        fg=0x00ffffff; /* white */
        GDrawFillPoly(pixmap, pts, i, fg);
	fg = g->state==gs_disabled?g->box->disabled_foreground:
			g->box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
			g->box->main_foreground;
        GDrawDrawPoly(pixmap, pts, i, fg);

         /* draw pupil */
        rect.x=gr->onoffrect.x+bp+w*.3;
        rect.y=gr->onoffrect.y+bp+h*.3;
        rect.width =.4*w;
        rect.height=.4*h;
        fg=0; /* black */
        GDrawFillElipse(pixmap, &rect, fg);

    } else if ( (!gr->ison) && gr->onbox == &visibility_on_box ) {
         /* draw closed eye */
        GPoint pts[7];
        double angle;
	int bp = gr->onbox->border_type==bt_none ? 0 : GDrawPointsToPixels(pixmap,gr->onbox->border_width);
        int x=gr->onoffrect.x+bp;
        int y=gr->onoffrect.y+bp;
        int w=gr->onoffrect.width -1-2*bp;
        int h=gr->onoffrect.height-1-2*bp;
        Color fg = g->state==gs_disabled?g->box->disabled_foreground:
			g->box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
			g->box->main_foreground;
        for (int i = 0; i <= 6; i++) {
            angle=(30+i/6.*120)*FF_PI/180;
            pts[i].x=.5*w*cos(angle)+x+w/2;
            pts[i].y=.5*h*sin(angle)+y+h/4;

             /* draw lashes */
            if (i>0 && i<6) GDrawDrawLine(pixmap, pts[i].x,pts[i].y, .75*w*cos(angle)+x+w/2, .75*h*sin(angle)+y+h/4, fg);
        }
        GDrawDrawPoly(pixmap, pts, sizeof(pts)/sizeof(pts[0]), fg);
    }

    GDrawPopClip(pixmap,&old2);
    x = gr->onoffrect.x + gr->onoffrect.width + GDrawPointsToPixels(pixmap,4);

     /* Finally write out the label if any. */
    GDrawPushClip(pixmap,&g->inner,&old2);
    if ( gr->font!=NULL )
	GDrawSetFont(pixmap,gr->font);
    if ( gr->image_precedes && img!=NULL ) {
	GDrawDrawScaledImage(pixmap,img,x,g->inner.y);
	x += GImageGetScaledWidth(pixmap,img) + GDrawPointsToPixels(pixmap,_GGadget_TextImageSkip);
    }
    if ( gr->label!=NULL ) {
	Color fg = g->state==gs_disabled?g->box->disabled_foreground:
			g->box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
			g->box->main_foreground;
	int lcnt = gradio_linecount(gr);
	if ( lcnt>1 )
	    yoff = (g->inner.height-lcnt*gr->fh)/2;
	_ggadget_underlineMnemonic(pixmap,x,g->inner.y + gr->as + yoff,gr->label,
		g->mnemonic,fg,g->inner.y+g->inner.height);
	x += GDrawDrawText(pixmap,x,g->inner.y + gr->as + yoff,gr->label,-1,fg);
	x += GDrawPointsToPixels(pixmap,_GGadget_TextImageSkip);
    }
    if ( !gr->image_precedes && img!=NULL )
	GDrawDrawScaledImage(pixmap,img,x,g->inner.y);

    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
return( true );
}

static int gradio_mouse(GGadget *g, GEvent *event) {
    GRadio *gr = (GRadio *) g;
    int within = gr->within, pressed = gr->pressed;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return( false );

    if ( event->type == et_crossing ) {
	if ( gr->within && !event->u.crossing.entered )
	    gr->within = false;
    } else if ( gr->pressed && event->type!=et_mousemove ) {
	if ( event->type == et_mousedown ) /* They pressed 2 mouse buttons? */
	    gr->pressed = false;
	else if ( GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y)) {
	    gr->pressed = false;
	    if ( !gr->isradio || !gr->ison )
		GRadioChanged(gr);
	} else if ( event->type == et_mouseup )
	    gr->pressed = false;
	else
	    gr->within = true;
    } else if ( event->type == et_mousedown &&
	    GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y)) {
	gr->pressed = true;
	gr->within = true;
    } else if ( event->type == et_mousemove &&
	    GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y)) {
	gr->within = true;
	if ( !gr->pressed && g->popup_msg )
	    GGadgetPreparePopup(g->base,g->popup_msg);
    } else if ( event->type == et_mousemove && gr->within ) {
	gr->within = false;
    } else {
return( false );
    }
    if ( within != gr->within )
	g->state = gr->within? gs_active : gs_enabled;
    if ( within != gr->within || pressed != gr->pressed )
	_ggadget_redraw(g);
return( gr->within );
}

static int gradio_key(GGadget *g, GEvent *event) {
    GRadio *gr = (GRadio *) g;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return(false);

    if ( event->u.chr.keysym == GK_Return || event->u.chr.keysym == GK_Tab ||
	    event->u.chr.keysym == GK_BackTab || event->u.chr.keysym == GK_Escape )
return( false );

    if (event->type == et_char && event->u.chr.chars[0]==' ' ) {
	GRadioChanged(gr);
	_ggadget_redraw(g);
return( true );
    }
return( false );
}

static int gradio_focus(GGadget *g, GEvent *event) {
    GRadio *gr = (GRadio *) g;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active ))
return(false);

    if ( event->u.focus.mnemonic_focus==mf_shortcut ) {
	GRadioChanged(gr);
    }
return( true );
}

static void gradio_destroy(GGadget *g) {
    GRadio *gr = (GRadio *) g;

    if ( gr==NULL )
return;
    if ( gr->isradio && gr->post!=gr ) {
	GRadio *prev;
	for ( prev=gr->post; prev->post!=gr; prev = prev->post );
	prev->post = gr->post;
    }
    free(gr->label);
    _ggadget_destroy(g);
}

static void GRadioSetTitle(GGadget *g,const unichar_t *tit) {
    GRadio *b = (GRadio *) g;
    free(b->label);
    b->label = u_copy(tit);
}

static const unichar_t *_GRadioGetTitle(GGadget *g) {
    GRadio *b = (GRadio *) g;
return( b->label );
}

static void GRadioSetImageTitle(GGadget *g,GImage *img,const unichar_t *tit, int before) {
    GRadio *b = (GRadio *) g;
    if ( b->g.free_box )
	free( b->g.box );
    free(b->label);
    b->label = u_copy(tit);
    b->image = img;
    b->image_precedes = before;
    _ggadget_redraw(g);
}

static GImage *GRadioGetImage(GGadget *g) {
    GRadio *b = (GRadio *) g;
return( b->image );
}

static void GRadioSetFont(GGadget *g,FontInstance *new) {
    GRadio *b = (GRadio *) g;
    b->font = new;
}

static FontInstance *GRadioGetFont(GGadget *g) {
    GRadio *b = (GRadio *) g;
return( b->font );
}

static void _gradio_move(GGadget *g, int32_t x, int32_t y ) {
    GRadio *b = (GRadio *) g;
    b->onoffrect.x = x+(b->onoffrect.x-g->r.x);
    b->onoffinner.x = x+(b->onoffinner.x-g->r.x);
    _ggadget_move(g,x,y);
    b->onoffrect.y = g->r.y+(g->r.height-b->onoffrect.height)/2;
    b->onoffinner.y = g->r.y+(g->r.height-b->onoffinner.height)/2;
}

static void GRadioGetDesiredSize(GGadget *g, GRect *outer, GRect *inner) {
    GCheckBox *gl = (GCheckBox *) g;
    int iwidth=0, iheight=0;
    GTextBounds bounds;
    int as=0, ds, ld, fh=0, width=0;

    if ( gl->image!=NULL ) {
	iwidth = GImageGetScaledWidth(gl->g.base,gl->image);
	iheight = GImageGetScaledHeight(gl->g.base,gl->image);
    }
    GDrawWindowFontMetrics(g->base,gl->font,&as, &ds, &ld);
    if ( gl->label!=NULL ) {
	FontInstance *old = GDrawSetFont(gl->g.base,gl->font);
	width = GDrawGetTextBounds(gl->g.base,gl->label, -1, &bounds);
	GDrawSetFont(gl->g.base,old);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
    }
    fh = as+ds;

    if ( width!=0 && iwidth!=0 )
	width += GDrawPointsToPixels(gl->g.base,_GGadget_TextImageSkip);
    width += iwidth;
    if ( iheight<fh )
	iheight = fh;
    if ( iheight < gl->onoffrect.height )
	iheight = gl->onoffrect.height;
    width += gl->onoffrect.width + GDrawPointsToPixels(gl->g.base,5);
    if ( g->desired_width>0 ) width = g->desired_width;
    if ( g->desired_height>0 ) iheight = g->desired_height;
    if ( inner!=NULL ) {
	inner->x = inner->y = 0;
	inner->width = width;
	inner->height = iheight;
    }
    if ( outer!=NULL ) {
	outer->x = outer->y = 0;
	outer->width = width;
	outer->height = iheight;
	/*_ggadgetFigureSize(gl->g.base,gl->g.box,outer,false);*/
    }
}

void GVisibilityBoxSetToMinWH(GGadget *g)
{
    GRect size;
    GRect outer;
    GRect inner;
    GRadioGetDesiredSize(g, &outer, &inner );
    outer.width = MIN(outer.width,outer.height);
    outer.width -= 3;
    size.width  = outer.width;
    size.height = outer.height;
    GGadgetSetSize(g,&size);
}


struct gfuncs gradio_funcs = {
    0,
    sizeof(struct gfuncs),

    gradio_expose,
    gradio_mouse,
    gradio_key,
    NULL,
    gradio_focus,
    NULL,
    NULL,

    _ggadget_redraw,
    _gradio_move,
    _ggadget_resize,
    _ggadget_setvisible,
    _ggadget_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    gradio_destroy,

    GRadioSetTitle,
    _GRadioGetTitle,
    NULL,
    GRadioSetImageTitle,
    GRadioGetImage,

    GRadioSetFont,
    GRadioGetFont,

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

    GRadioGetDesiredSize,
    _ggadget_setDesiredSize,
    NULL,
    NULL
};

extern int GImageWriteGImage(GImage *gi, char *filename);
static void GRadioInit() {
    if ( !gradio_inited ) {
	GResEditDoInit(&gradio_ri);
	GResEditDoInit(&gradioon_ri);
	GResEditDoInit(&gradiooff_ri);
	GResEditDoInit(&gcheckbox_ri);
	GResEditDoInit(&gcheckboxon_ri);
	GResEditDoInit(&gcheckboxoff_ri);
	GResEditDoInit(&gvisibility_ri);
	GResEditDoInit(&gvisibilityon_ri);
	GResEditDoInit(&gvisibilityoff_ri);
    }
    if ( radio_on_box.depressed_background == radio_off_box.depressed_background ) {
	radio_on_box.depressed_background = radio_on_box.active_border;
	radio_off_box.depressed_background = radio_off_box.main_background;
    }
    if ( checkbox_on_box.depressed_background == checkbox_off_box.depressed_background ) {
	checkbox_on_box.depressed_background = checkbox_on_box.active_border;
	checkbox_off_box.depressed_background = checkbox_off_box.main_background;
    }
    gradio_inited = true;
}

static void GCheckBoxFit(GCheckBox *gl) {
    int as=0, ds, ld;
    GRect outer, inner, needed;
    int iwidth, iheight;

    needed.x = needed.y = 0;
    needed.width = needed.height = 1;
    if ( gl->on!=NULL && GResImageGetImage(gl->on)!=NULL ) {
	if (( iwidth = GImageGetScaledWidth(gl->g.base,GResImageGetImage(gl->on)))>needed.width )
	    needed.width = iwidth;
	if (( iheight = GImageGetScaledHeight(gl->g.base,GResImageGetImage(gl->on)))>needed.height )
	    needed.height = iheight;
    }
    if ( gl->off!=NULL && GResImageGetImage(gl->off)!=NULL ) {
	if (( iwidth = GImageGetScaledWidth(gl->g.base,GResImageGetImage(gl->off)))>needed.width )
	    needed.width = iwidth;
	if (( iheight = GImageGetScaledHeight(gl->g.base,GResImageGetImage(gl->off)))>needed.height )
	    needed.height = iheight;
    }
    gl->onoffinner = needed;
    _ggadgetFigureSize(gl->g.base,gl->onbox,&needed,false);
    gl->onoffrect = needed;

    GDrawWindowFontMetrics(gl->g.base,gl->font,&as, &ds, &ld);
    GRadioGetDesiredSize(&gl->g,&outer,&inner);
    _ggadgetSetRects(&gl->g,&outer, &inner, -1, 0);

    gl->as = as;
    gl->fh = as+ds;

    gl->onoffrect.x = gl->g.inner.x;
    gl->onoffrect.y = (gl->as>gl->onoffrect.height)?gl->g.inner.y+gl->as-gl->onoffrect.height:
	    gl->g.inner.y+(gl->g.inner.height-gl->onoffrect.height)/2;
    gl->onoffinner.x = gl->onoffrect.x + (gl->onoffrect.width-gl->onoffinner.width)/2;
    gl->onoffinner.y = gl->onoffrect.y + (gl->onoffrect.height-gl->onoffinner.height)/2;
}

static GCheckBox *_GCheckBoxCreate(GCheckBox *gl, struct gwindow *base, GGadgetData *gd,void *data, GBox *def) {

    GRadioInit();
    gl->g.funcs = &gradio_funcs;
    _GGadget_Create(&gl->g,base,gd,data,def);

    gl->g.takes_input = true; gl->g.takes_keyboard = true; gl->g.focusable = true;
    gl->font = checkbox_font.fi;
    if ( gd->label!=NULL ) {
	gl->image_precedes = gd->label->image_precedes;
	if ( gd->label->font!=NULL )
	    gl->font = gd->label->font;
	if ( gd->label->text_is_1byte )
	    gl->label = utf82u_mncopy((char *) gd->label->text,&gl->g.mnemonic);
	else
	    gl->label = u_copy(gd->label->text);
	gl->image = gd->label->image;
    }
    if ( gd->flags & gg_cb_on )
	gl->ison = true;
    if ( gl->isradio ) {
	gl->onbox = &radio_on_box;
	gl->offbox = &radio_off_box;
	gl->on = &radon;
	gl->off = &radoff;
	gl->ondis = &raddison;
	gl->offdis = &raddisoff;
    } else {
	gl->onbox = &checkbox_on_box;
	gl->offbox = &checkbox_off_box;
	gl->on = &checkon;
	gl->off = &checkoff;
	gl->ondis = &checkdison;
	gl->offdis = &checkdisoff;
    }
    GCheckBoxFit(gl);
    _GGadget_FinalPosition(&gl->g,base,gd);

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&gl->g);
return( gl );
}

GGadget *GCheckBoxCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GCheckBox *gl = _GCheckBoxCreate(calloc(1,sizeof(GCheckBox)),base,gd,data,&checkbox_box);

return( &gl->g );
}

GGadget *GVisibilityBoxCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GCheckBox *gl = _GCheckBoxCreate(calloc(1,sizeof(GCheckBox)),base,gd,data,&visibility_box);
    gl->onbox = &visibility_on_box;
    gl->offbox = &visibility_off_box;
    gl->on = &visibilityon;
    gl->off = &visibilityoff;
    gl->ondis = &visibilitydison;
    gl->offdis = &visibilitydisoff;

return( &gl->g );
}

GGadget *GRadioCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GRadio *gl = (GRadio *) calloc(1,sizeof(GRadio));
    GGadget *gr;

    gl->isradio = true;
    gl->radiogroup = gd->u.radiogroup;
    _GCheckBoxCreate((GCheckBox *) gl,base,gd,data,&radio_box);

    gl->post = gl;
    if ( gd->flags & gg_rad_startnew )
	/* Done */;
    else if ( gl->g.prev!=NULL && gl->radiogroup!=0 ) {
	for ( gr=gl->g.prev; gr!=NULL; gr = gr->prev ) {
	    if ( gr->funcs==&gradio_funcs && ((GRadio *) gr)->radiogroup == gl->radiogroup ) {
		gl->post = ((GRadio *) gr)->post;
		((GRadio *) gr)->post = gl;
	break;
	    }
	}
    } else if ( gl->g.prev!=NULL && gl->g.prev->funcs==&gradio_funcs &&
	    ((GRadio *) (gl->g.prev))->isradio ) {
	gl->post = ((GRadio *) (gl->g.prev))->post;
	((GRadio *) (gl->g.prev))->post = gl;
    } else if ( gd->flags & gg_rad_continueold ) {
	for ( gr=gl->g.prev; gr!=NULL && (gr->funcs!=&gradio_funcs ||
		!((GRadio *) gr)->isradio); gr = gr->prev );
	if ( gr!=NULL ) {
	    gl->post = ((GRadio *) gr)->post;
	    ((GRadio *) gr)->post = gl;
	}
    }

return( &gl->g );
}

void GGadgetSetChecked(GGadget *g, int ison) {
    GRadio *gr = (GRadio *) g;

    if ( gr->isradio ) {
	if ( ison && !gr->ison ) {
	    GRadio *other;
	    for ( other=gr->post; other!=gr; other = other->post ) {
		if ( other->ison ) {
		    other->ison = false;
		    _ggadget_redraw((GGadget *) other);
		}
	    }
	}
    }
    gr->ison = ison?1:0;
    _ggadget_redraw(g);
}

int GGadgetIsChecked(GGadget *g) {
return( ((GRadio *) g)->ison );
}
