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

#include "basics.h"
#include "ffglib.h"
#include "gdraw.h"
#include "ggadgetP.h"
#include "gkeysym.h"
#include "gresource.h"
#include "gwidget.h"
#include "ustring.h"
#include "utype.h"

GBox _ggadget_Default_Box = { bt_raised, bs_rect, 2, 2, 0, 0, 
    COLOR_CREATE(0xd8,0xd8,0xd8),		/* border left */ /* brightest */
    COLOR_CREATE(0xd0,0xd0,0xd0),		/* border top */
    COLOR_CREATE(0x80,0x80,0x80),		/* border right */
    COLOR_CREATE(0x66,0x66,0x66),		/* border bottom */ /* darkest */
    COLOR_DEFAULT,				/* normal background */
    COLOR_DEFAULT,				/* normal foreground */
    COLOR_CREATE(0xd8,0xd8,0xd8),		/* disabled background */
    COLOR_CREATE(0x66,0x66,0x66),		/* disabled foreground */
    COLOR_CREATE(0xff,0xff,0x00),		/* active border */
    COLOR_CREATE(0xa0,0xa0,0xa0),		/* pressed background */
    COLOR_CREATE(0x00,0x00,0x00),		/* gradient bg end */
    COLOR_CREATE(0x00,0x00,0x00),		/* border inner */
    COLOR_CREATE(0x00,0x00,0x00),		/* border outer */
};
GBox _GListMark_Box = GBOX_EMPTY; /* Don't initialize here */
GResFont _ggadget_default_font = GRESFONT_INIT("400 10pt " SANS_UI_FAMILIES);
GResFont popup_font = GRESFONT_INIT("400 8pt " SANS_UI_FAMILIES);
int _GListMarkSize = 12;
GResImage _GListMark_Image = GRESIMAGE_INIT("downarrow.png");
GResImage _GListMark_DisImage = GRESIMAGE_INIT("downarrow_disabled.png");
static int _GGadget_FirstLine = 6;
static int _GGadget_LeftMargin = 6;
static int _GGadget_LineSkip = 3;
int _GGadget_Skip = 6;
int _GGadget_TextImageSkip = 4;
static Color popup_foreground=0, popup_background=COLOR_CREATE(0xff,0xff,0xc0);
static int popup_delay=1000, popup_lifetime=20000;
static char *_GGadget_ImagePathRes;

static int GGadgetRIInit(GResInfo *ri) {
    if ( ri->is_initialized )
	return false;

    ri->overrides.main_background = GDrawGetDefaultBackground(NULL);
    ri->overrides.main_foreground = GDrawGetDefaultForeground(NULL);
    _GResEditInitialize(ri);
    return true;
}

static void *_GGadgetImagePathResSet(char *res, void *def) {
    GGadgetSetImagePath(res);
    return res;
}

static struct resed ggadget_re[] = {
    {N_("Skip"), "Skip", rt_int, &_GGadget_Skip, N_("Space (in points) to skip between gadget elements"), NULL, { 0 }, 0, 0 },
    {N_("Line Skip"), "LineSkip", rt_int, &_GGadget_LineSkip, N_("Space (in points) to skip after a line"), NULL, { 0 }, 0, 0 },
    {N_("First Line Skip"), "FirstLine", rt_int, &_GGadget_FirstLine, N_("Space (in points) before a line when it is the first element"), NULL, { 0 }, 0, 0 },
    {N_("Left Margin"), "LeftMargin", rt_int, &_GGadget_LeftMargin, N_("The default left margin (in points)"), NULL, { 0 }, 0, 0 },
    {N_("Text Image Skip"), "TextImageSkip", rt_int, &_GGadget_TextImageSkip, N_("Space (in points) left between images and text in any labels, buttons, menu items, etc. that have both"), NULL, { 0 }, 0, 0 },
    {N_("Image Path"), "ImagePath", rt_stringlong, &_GGadget_ImagePathRes, N_("List of directories to search for images, separated by colons"), _GGadgetImagePathResSet, { 0 }, 0, 0 },
    RESED_EMPTY
};
GResInfo ggadget2_ri = {
    &listmark_ri, NULL,NULL, NULL,
    NULL,
    NULL,
    NULL,
    ggadget_re,
    N_("GGadget 2"),
    N_("More configuration parameters for the \"abstract\" gadget."),
    "GGadget",
    "Gdraw",
    false,
    false,
    0,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
GResInfo ggadget_ri = {
    &ggadget2_ri, NULL,NULL, NULL,
    &_ggadget_Default_Box,
    &_ggadget_default_font,
    NULL,
    NULL,
    N_("GGadget"),
    N_("This is an \"abstract\" gadget. It will never appear on the screen\nbut it is the root of gadget tree from which all others inherit"),
    "GGadget",
    "Gdraw",
    false,
    false,
    omf_main_foreground|omf_main_background,
    GBOX_EMPTY,
    GBOX_EMPTY,
    NULL,
    GGadgetRIInit,
    NULL
};
static struct resed popup_re[] = {
    {N_("Color|Foreground"), "Foreground", rt_color, &popup_foreground, N_("Text color for popup windows"), NULL, { 0 }, 0, 0 },
    {N_("Color|Background"), "Background", rt_color, &popup_background, N_("Background color for popup windows"), NULL, { 0 }, 0, 0 },
    {N_("Delay"), "Delay", rt_int, &popup_delay, N_("Delay (in milliseconds) before popup windows appear"), NULL, { 0 }, 0, 0 },
    {N_("Life Time"), "LifeTime", rt_int, &popup_lifetime, N_("Time (in milliseconds) that popup windows remain visible"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
static void popup_refresh(void);
extern GResInfo gprogress_ri;
GResInfo gpopup_ri = {
    &gprogress_ri, NULL, NULL,NULL,
    NULL,	/* No box */
    &popup_font,
    NULL,
    popup_re,
    N_("Popup"),
    N_("Popup windows"),
    "GGadget.Popup",
    "Gdraw",
    false,
    false,
    omf_refresh,
    GBOX_EMPTY,
    GBOX_EMPTY,
    popup_refresh,
    NULL,
    NULL
};
static struct resed listmark_re[] = {
    { N_("Image"), "Image", rt_image, &_GListMark_Image, N_("Image used for enabled listmarks (overrides the box)"), NULL, { 0 }, 0, 0 },
    { N_("Disabled Image"), "DisabledImage", rt_image, &_GListMark_DisImage, N_("Image used for disabled listmarks (overrides the box)"), NULL, { 0 }, 0, 0 },
    { N_("Width"), "Width", rt_int, &_GListMarkSize, N_("Size of the list mark"), NULL, { 0 }, 0, 0 },
    RESED_EMPTY
};
static GTextInfo list_choices[] = {
    { (unichar_t *) "1", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "2", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "3", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};
static GGadgetCreateData droplist_gcd[] = {
    { GListFieldCreate, { {0, 0, 80, 0 }, NULL, 0, 0, 0, 0, 0, &list_choices[0], { list_choices }, gg_visible, NULL, NULL }, NULL, NULL },
    { GListFieldCreate, { {0, 0, 80, 0 }, NULL, 0, 0, 0, 0, 0, &list_choices[1], { list_choices }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *dlarray[] = { GCD_Glue, &droplist_gcd[0], GCD_Glue, &droplist_gcd[1], GCD_Glue, NULL, NULL };
static GGadgetCreateData droplistbox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) dlarray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
extern GResInfo glabel_ri;
GResInfo listmark_ri = {
    &glabel_ri, &ggadget_ri, NULL,NULL,
    &_GListMark_Box,	/* No box */
    NULL,
    &droplistbox,
    listmark_re,
    N_("List Mark"),
    N_("This is the mark that differentiates ComboBoxes and ListButtons\n"
	"from TextFields and normal Buttons." ),
    "GListMark",
    "Gdraw",
    false,
    false,
    omf_border_width|omf_padding,
    { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};


static GWindow popup;
static GTimer *popup_timer, *popup_vanish_timer;
static int popup_visible = false;

void GGadgetInit(void) {
    GResEditDoInit(&ggadget_ri);
    GResEditDoInit(&ggadget2_ri);
    GResEditDoInit(&gpopup_ri);
    GResEditDoInit(&listmark_ri);
    GTabSetInit();
}

void GListMarkDraw(GWindow pixmap,int x, int y, int height, enum gadget_state state ) {
    GRect r, old;
    GImage *mi, *mdi;
    int gmsize = GDrawPointsToPixels(pixmap,_GListMarkSize);
    if ( (mi=GResImageGetImage(&_GListMark_Image))!=NULL ) {
	int size = GImageGetWidth(mi);
	if ( size>gmsize )
	    gmsize = size;
    }
    int marklen = gmsize;

    if ( state == gs_disabled && (mdi=GResImageGetImage(&_GListMark_DisImage))!=NULL) {
	GDrawDrawScaledImage(pixmap,mdi,x,
		y + (height-GImageGetScaledHeight(pixmap,mdi))/2);
    } else if ( mi!=NULL ) {
	GDrawDrawScaledImage(pixmap,mi,x,
		y + (height-GImageGetScaledHeight(pixmap,mi))/2);
    } else {
	r.x = x; r.width = marklen;
	r.height = 2*GDrawPointsToPixels(pixmap,_GListMark_Box.border_width) +
		    GDrawPointsToPixels(pixmap,3);
	r.y = y + (height-r.height)/2;
	GDrawPushClip(pixmap,&r,&old);

	GBoxDrawBackground(pixmap,&r,&_GListMark_Box, state,false);
	GBoxDrawBorder(pixmap,&r,&_GListMark_Box,state,false);
	GDrawPopClip(pixmap,&old);
    }
}

static struct popup_info {
    const unichar_t *msg;
    GImage *img;
    const void *data;
    GImage *(*get_image)(const void *data);
    void (*free_image)(const void *data,GImage *img);
    GWindow gw;
} popup_info;

void GGadgetEndPopup() {
    if ( popup_visible ) {
	GDrawSetVisible(popup,false);
	popup_visible = false;
    }
    if ( popup_timer!=NULL ) {
	GDrawCancelTimer(popup_timer);
	popup_timer = NULL;
    }
    if ( popup_vanish_timer!=NULL ) {
	GDrawCancelTimer(popup_vanish_timer);
	popup_vanish_timer = NULL;
    }
    if ( popup_info.img!=NULL ) {
	if ( popup_info.free_image!=NULL )
	    (popup_info.free_image)(popup_info.data,popup_info.img);
	else
	    GImageDestroy(popup_info.img);
    }
	    
    memset(&popup_info,0,sizeof(popup_info));
}

void GGadgetPopupExternalEvent(GEvent *e) {
    /* Depress control key to keep popup alive */
    if ( e->type == et_char &&
	    ( e->u.chr.keysym == GK_Control_L || e->u.chr.keysym == GK_Control_R )) {
	if ( popup_vanish_timer!=NULL ) {
	    GDrawCancelTimer(popup_vanish_timer);
	    popup_vanish_timer = NULL;
	}
return;
    }
    if ( e->type==et_char || e->type==et_charup || e->type==et_mousemove ||
	    e->type == et_mousedown || e->type==et_mouseup ||
	    e->type==et_destroy || (e->type==et_create && popup!=e->w))
	GGadgetEndPopup();
}

static int GGadgetPopupTest(GEvent *e) {
    unichar_t *msg;
    int lines, temp, width;
    GWindow root = GDrawGetRoot(GDrawGetDisplayOfWindow(popup));
    GRect pos, size;
    unichar_t *pt, *ept;
    int as, ds, ld, img_height=0;
    GEvent where;

    if ( e->type!=et_timer || e->u.timer.timer!=popup_timer || popup==NULL )
return( false );
    popup_timer = NULL;

    /* Is the cursor still in the original window? */
    if (!GDrawWindowIsAncestor(popup_info.gw, GDrawGetPointerWindow(popup_info.gw)))
        return true;

    lines = 0; width = 1;
    if ( popup_info.img==NULL && popup_info.get_image!=NULL ) {
	popup_info.img = (popup_info.get_image)(popup_info.data);
	popup_info.get_image = NULL;
    }
    if ( popup_info.img!=NULL ) {
	img_height = GImageGetHeight(popup_info.img);
	width = GImageGetWidth(popup_info.img);
    }
    pt = msg = (unichar_t *) popup_info.msg;
    if ( msg!=NULL ) {
	GDrawSetFont(popup,popup_font.fi);
	do {
	    temp = -1;
	    if (( ept = u_strchr(pt,'\n'))!=NULL )
		temp = ept-pt;
	    temp = GDrawGetTextWidth(popup,pt,temp);
	    if ( temp>width ) width = temp;
	    ++lines;
	    pt = ept+1;
	} while ( ept!=NULL && *pt!='\0' );
    }
    GDrawWindowFontMetrics(popup,popup_font.fi,&as, &ds, &ld);
    pos.width = width+2*GDrawPointsToPixels(popup,2);
    pos.height = lines*(as+ds) + img_height + 2*GDrawPointsToPixels(popup,2);

    GDrawGetPointerPosition(root,&where);
    pos.x = where.u.mouse.x+10; pos.y = where.u.mouse.y+10;
    GDrawGetSize(root,&size);
    if ( pos.x + pos.width > size.width )
	pos.x = (pos.x - 20 - pos.width );
    if ( pos.x<0 ) pos.x = 0;
    if ( pos.y + pos.height > size.height )
	pos.y = (pos.y - 20 - pos.height );
    if ( pos.y<0 ) pos.y = 0;
    GDrawMoveResize(popup,pos.x,pos.y,pos.width,pos.height);
    GDrawSetVisible(popup,true);
    GDrawRaise(popup);
    GDrawSetUserData(popup,msg);
    popup_vanish_timer = GDrawRequestTimer(popup,popup_lifetime,0,NULL);
return( true );
}

static int msgpopup_eh(GWindow popup,GEvent *event) {
    if ( event->type == et_expose ) {
	unichar_t *msg, *pt, *ept;
	int x,y, fh, temp;
	int as, ds, ld;

	popup_visible = true;
	pt = msg = (unichar_t *) popup_info.msg;
	if ( (pt==NULL || *pt=='\0') && popup_info.img==NULL ) {
	    GGadgetEndPopup();
return( true );
	}
	y = x = GDrawPointsToPixels(popup,2);
	if ( popup_info.img!=NULL ) {
	    GDrawDrawImage(popup,popup_info.img,NULL,x,y);
	    y += GImageGetHeight(popup_info.img);
	}
	if ( pt!=NULL ) {
	    GDrawWindowFontMetrics(popup,popup_font.fi,&as, &ds, &ld);
	    fh = as+ds;
	    y += as;
	    while ( *pt!='\0' ) {
                /* Find end of this line, if multiline text */
		if (( ept = u_strchr(pt,'\n'))!=NULL )
		    temp = ept-pt;  /* display next segment of multiline text */
                else
                    temp = -1;     /* last line, so display remainder of text */
		GDrawDrawText(popup,x,y,pt,temp,popup_foreground);
		y += fh;
		pt = ept+1;
                if ( ept==NULL )
                    break;
	    }
	}
    } else if ( event->type == et_timer && event->u.timer.timer==popup_timer ) {
	GGadgetPopupTest(event);
    } else if ( event->type == et_mousemove || event->type == et_mouseup ||
	    event->type == et_mousedown || event->type == et_char ||
	    event->type == et_timer || event->type == et_crossing ) {
	GGadgetEndPopup();
    }
return( true );
}

void GGadgetPreparePopupImage(GWindow base,const unichar_t *msg, const void *data,
	GImage *(*get_image)(const void *data),
	void (*free_image)(const void *data,GImage *img)) {

    GGadgetEndPopup();
    if ( msg==NULL && get_image==NULL )
return;

    memset(&popup_info,0,sizeof(popup_info));
    popup_info.msg = msg;
    popup_info.data = data;
    popup_info.get_image = get_image;
    popup_info.free_image = free_image;
    popup_info.gw = base;

    if ( popup==NULL ) {
	GWindowAttrs pattrs;
	GRect pos;

	pattrs.mask = wam_events|wam_nodecor|wam_positioned|wam_cursor|wam_backcol/*|wam_transient*/;
	pattrs.event_masks = -1;
	pattrs.nodecoration = true;
	pattrs.positioned = true;
	pattrs.cursor = ct_pointer;
	pattrs.background_color = popup_background;
	/*pattrs.transient = GWidgetGetTopWidget(base);*/
	pos.x = pos.y = 0; pos.width = pos.height = 1;
	popup = GDrawCreateTopWindow(GDrawGetDisplayOfWindow(base),&pos,
		msgpopup_eh,NULL,&pattrs);
	GDrawSetFont(popup,popup_font.fi);
    }
    popup_timer = GDrawRequestTimer(popup,popup_delay,0,(void *) msg);
}

static void popup_refresh(void) {
    if ( popup!=NULL ) {
	GDrawSetWindowBackground(popup,popup_background);
    }
}

void GGadgetPreparePopup(GWindow base,const unichar_t *msg) {
    GGadgetPreparePopupImage(base,msg,NULL,NULL,NULL);
}

void GGadgetPreparePopup8(GWindow base, const char *msg) {
    static unichar_t popup_msg[500];
    utf82u_strncpy(popup_msg,msg,sizeof(popup_msg)/sizeof(popup_msg[0]));
    popup_msg[sizeof(popup_msg)/sizeof(popup_msg[0])-1]=0;
    GGadgetPreparePopupImage(base,popup_msg,NULL,NULL,NULL);
}

void _ggadget_redraw(GGadget *g) {
    GDrawRequestExpose(g->base, &g->r, false);
}

int _ggadget_noop(GGadget *g, GEvent *event) {
return( false );
}

static void GBoxFigureRect(GWindow gw, GBox *design, GRect *r, int isdef) {
    int scale = GDrawPointsToPixels(gw,1);
    int bp = GDrawPointsToPixels(gw,design->border_width)+
	     GDrawPointsToPixels(gw,design->padding)+
	     ((design->flags & box_foreground_border_outer)?scale:0)+
	     ((design->flags &
		 (box_foreground_border_inner|box_active_border_inner))?scale:0)+
	     (isdef && (design->flags & box_draw_default)?scale+GDrawPointsToPixels(gw,2):0);
    r->width += 2*bp;
    r->height += 2*bp;
}

static void GBoxFigureDiamond(GWindow gw, GBox *design, GRect *r, int isdef) {
    int scale = GDrawPointsToPixels(gw,1);
    int p = GDrawPointsToPixels(gw,design->padding);
    int b = GDrawPointsToPixels(gw,design->border_width)+
	     ((design->flags & box_foreground_border_outer)?scale:0)+
	     ((design->flags &
		 (box_foreground_border_inner|box_active_border_inner))?scale:0)+
	     (isdef && (design->flags & box_draw_default)?scale+GDrawPointsToPixels(gw,2):0);
    int xoff = r->width/2, yoff = r->height/2;

    if ( xoff<2*p ) xoff = 2*p;
    if ( yoff<2*p ) yoff = 2*p;
     r->width += 2*b+xoff;
     r->height += 2*b+yoff;
}

void _ggadgetFigureSize(GWindow gw, GBox *design, GRect *r, int isdef) {
    /* given that we want something with a client rectangle as big as r */
    /*  Then given the box shape, figure out how much of a rectangle we */
    /*  need around it */

    if ( r->width<=0 ) r->width = 1;
    if ( r->height<=0 ) r->height = 1;

    switch ( design->border_shape ) {
      case bs_rect:
	GBoxFigureRect(gw,design,r,isdef);
      break;
      case bs_roundrect:
	GBoxFigureRect(gw,design,r,isdef);
      break;
      case bs_elipse:
	GBoxFigureDiamond(gw,design,r,isdef);
      break;
      case bs_diamond:
	GBoxFigureDiamond(gw,design,r,isdef);
      break;
    }
}

void _ggadgetSetRects(GGadget *g, GRect *outer, GRect *inner, int xjust, int yjust) {
    int bp = GBoxBorderWidth(g->base,g->box);


    if ( g->r.width==0 )
	g->r.width = outer->width;
    if ( g->r.height==0 )
	g->r.height = outer->height;

    if ( g->inner.width==0 ) {
	if ( inner->width<g->r.width ) {
	    g->inner.width = g->r.width - 2*bp;
	    if ( xjust==-1 )
		g->inner.x = g->r.x + bp;
	    else if ( xjust==0 ) {
		g->inner.x = g->r.x + (g->r.width-inner->width)/2;
		g->inner.width = inner->width;
	    } else
		g->inner.x = g->r.x + (g->r.width-bp-g->inner.width);
	} else {
	    g->inner.x = g->r.x;
	    g->inner.width = g->r.width;
	}
    }
    if ( g->inner.height==0 ) {
	if ( inner->height<g->r.height ) {
	    if ( yjust==-1 )
		g->inner.y = g->r.y + bp;
	    else if ( yjust==0 )
		g->inner.y = g->r.y + (g->r.height-inner->height)/2;
	    else
		g->inner.y = g->r.y + (g->r.height-bp-inner->height);
	    g->inner.height = inner->height;
	} else {
	    g->inner.y = g->r.y;
	    g->inner.height = g->r.height;
	}
    }
}

static GGadget *GGadgetFindLastOpenGroup(GGadget *g) {
    for ( g=g->prev; g!=NULL && !g->opengroup; g = g->prev );
return( g );
}

static int GGadgetLMargin(GGadget *sigh, GGadget *lastOpenGroup) {
    if ( lastOpenGroup==NULL )
return( GDrawPointsToPixels(sigh->base,_GGadget_LeftMargin));
    else
return( lastOpenGroup->r.x + GDrawPointsToPixels(lastOpenGroup->base,_GGadget_Skip));
}

int GGadgetScale(int xpos) {
return( xpos*GIntGetResource(_NUM_ScaleFactor)/100 );
}

GGadget *_GGadget_Create(GGadget *g, struct gwindow *base, GGadgetData *gd,void *data, GBox *def) {
    GGadget *last, *lastopengroup;

    g->desired_width = g->desired_height = -1;
    _GWidget_AddGGadget(base,g);
    g->r = gd->pos;
    if ( !(gd->flags&gg_pos_in_pixels) ) {
	g->r.x = GDrawPointsToPixels(base,g->r.x);
	g->r.y = GDrawPointsToPixels(base,g->r.y);
	if ( g->r.width!=-1 )
	    g->r.width = GDrawPointsToPixels(base,g->r.width);
	if ( !(gd->flags&gg_pos_use0)) {
	    g->r.x = GGadgetScale(g->r.x);
	    if ( g->r.width!=-1 )
		g->r.width = GGadgetScale(g->r.width);
	}
	g->r.height = GDrawPointsToPixels(base,g->r.height);
    }
    if ( gd->pos.width>0 ) g->desired_width = g->r.width;
    if ( gd->pos.height>0 ) g->desired_height = g->r.height;
    last = g->prev;
    lastopengroup = GGadgetFindLastOpenGroup(g);
    if ( g->r.y==0 && !(gd->flags & gg_pos_use0)) {
	if ( last==NULL ) {
	    g->r.y = GDrawPointsToPixels(base,_GGadget_FirstLine);
	    if ( g->r.x==0 )
		g->r.x = GGadgetLMargin(g,lastopengroup);
	} else if ( gd->flags & gg_pos_newline ) {
	    int temp, y = last->r.y, nexty = last->r.y+last->r.height;
	    while ( last!=NULL && last->r.y==y ) {
		temp = last->r.y+last->r.height;
		if ( temp>nexty ) nexty = temp;
		last = last->prev;
	    }
	    g->r.y = nexty + GDrawPointsToPixels(base,_GGadget_LineSkip);
	    if ( g->r.x==0 )
		g->r.x = GGadgetLMargin(g,lastopengroup);
	} else {
	    g->r.y = last->r.y;
	}
    }
    if ( g->r.x == 0 && !(gd->flags & gg_pos_use0)) {
	last = g->prev;
	if ( last==NULL )
	    g->r.x = GGadgetLMargin(g,lastopengroup);
	else if ( gd->flags & gg_pos_under ) {
	    int onthisline=0, onprev=0, i;
	    GGadget *prev, *prevline;
	    /* see if we can find a gadget on the previous line that has the */
	    /*  same number of gadgets between it and the start of the line  */
	    /*  as this one has from the start of its line, then put at the  */
	    /*  same x offset */
	    for ( prev = last; prev!=NULL && prev->r.y==g->r.y; prev = prev->prev )
		++onthisline;
	    prevline = prev;
	    for ( ; prev!=NULL && prev->r.y==prevline->r.y ; prev = prev->prev )
		onprev++;
	    for ( prev = prevline, i=0; prev!=NULL && i<onprev-onthisline; prev = prev->prev);
	    if ( prev!=NULL )
		g->r.x = prev->r.x;
	}
	if ( g->r.x==0 )
	    g->r.x = last->r.x + last->r.width + GDrawPointsToPixels(base,_GGadget_Skip);
    }

    g->mnemonic = islower(gd->mnemonic)?toupper(gd->mnemonic):gd->mnemonic;
    g->shortcut = islower(gd->shortcut)?toupper(gd->shortcut):gd->shortcut;
    g->short_mask = gd->short_mask;
    g->cid = gd->cid;
    g->data = data;
    g->popup_msg = utf82u_copy(gd->popup_msg);
    g->handle_controlevent = gd->handle_controlevent;
    if ( gd->box == NULL )
	g->box = def;
    else if ( gd->flags & gg_dontcopybox )
	g->box = gd->box;
    else {
	g->free_box = true;
	g->box = malloc(sizeof(GBox));
	*g->box = *gd->box;
    }
    g->state = !(gd->flags&gg_visible) ? gs_invisible :
		  !(gd->flags&gg_enabled) ? gs_disabled :
		  gs_enabled;
    if ( !(gd->flags&gg_enabled) ) g->was_disabled = true;
return( g );
}

void _GGadget_FinalPosition(GGadget *g, struct gwindow *base, GGadgetData *gd) {
    if ( g->r.x<0 && !(gd->flags&gg_pos_use0)) {
	GRect size;
	GDrawGetSize(base,&size);
	g->r.x += size.width-g->r.width;
	g->inner.x += size.width-g->r.width;
    }
}

void _ggadget_destroy(GGadget *g) {
    if ( g==NULL )
return;
    _GWidget_RemoveGadget(g);
    GGadgetEndPopup();
    if ( g->free_box )
	free( g->box );
    free(g->popup_msg);
    free(g);
}

void _GGadgetCloseGroup(GGadget *g) {
    GGadget *group = GGadgetFindLastOpenGroup(g);
    GGadget *prev;
    int maxx=0, maxy=0, temp;
    int bp = GBoxBorderWidth(g->base,g->box);

    if ( group==NULL )
return;
    for ( prev=g ; prev!=group; prev = prev->prev ) {
	temp = prev->r.x + prev->r.width;
	if ( temp>maxx ) maxx = temp;
	temp = prev->r.y + prev->r.height;
	if ( temp>maxy ) maxy = temp;
    }
    if ( group->prevlabel ) {
	prev = group->prev;
	temp = prev->r.x + prev->r.width;
	if ( temp>maxx ) maxx = temp;
	temp = prev->r.y + prev->r.height/2 ;
	if ( temp>maxy ) maxy = temp;
    }
    maxx += GDrawPointsToPixels(g->base,_GGadget_Skip);
    maxy += GDrawPointsToPixels(g->base,_GGadget_LineSkip);

    if ( group->r.width==0 ) {
	group->r.width = maxx - group->r.x;
	group->inner.width = group->r.width - 2*bp;
    }
    if ( group->r.height==0 ) {
	group->r.height = maxy - group->r.y;
	group->inner.height = group->r.y + group->r.height - bp -
		group->inner.y;
    }
    group->opengroup = false;
}

int GGadgetWithin(GGadget *g, int x, int y) {
    register GRect *r = &g->r;

    if ( x<r->x || y<r->y || x>=r->x+r->width || y>=r->y+r->height )
return( false );

return( true );
}

int GGadgetInnerWithin(GGadget *g, int x, int y) {
    register GRect *r = &g->inner;

    if ( x<r->x || y<r->y || x>=r->x+r->width || y>=r->y+r->height )
return( false );

return( true );
}

void _ggadget_underlineMnemonic(GWindow gw,int32_t x,int32_t y,unichar_t *label,
	unichar_t mnemonic, Color fg, int maxy) {
    int point = GDrawPointsToPixels(gw,1);
    int width;
    /*GRect clip;*/

    if ( mnemonic=='\0' )
return;
    char *ctext = u2utf8_copy(label);
    char *cpt = utf8_strchr(ctext,mnemonic);
    GRect space;
    if ( cpt==NULL && isupper(mnemonic))
	cpt = utf8_strchr(ctext,tolower(mnemonic));
    if ( cpt==NULL )
return;
    GDrawLayoutInit(gw,ctext,-1,NULL);
    GDrawLayoutIndexToPos(gw, cpt-ctext, &space);
    free(ctext);
    x += space.x;
    width = space.width;
    GDrawSetLineWidth(gw,point);
    y += 2*point;
    if ( y+point-1 >= maxy )
	y = maxy-point;
    GDrawDrawLine(gw,x,y,x+width,y,fg);
    GDrawSetLineWidth(gw,0);
}

void _ggadget_move(GGadget *g, int32_t x, int32_t y ) {
    g->inner.x = x+(g->inner.x-g->r.x);
    g->inner.y = y+(g->inner.y-g->r.y);
    g->r.x = x;
    g->r.y = y;
}

void _ggadget_resize(GGadget *g, int32_t width, int32_t height ) {
    g->inner.width = width-(g->r.width-g->inner.width);
    g->inner.height = height-(g->r.height-g->inner.height);
    g->r.width = width;
    g->r.height = height;
}

void _ggadget_getDesiredSize(GGadget *g, GRect *outer, GRect *inner) {

    if ( inner!=NULL ) {
	inner->x = inner->y = 0;
	inner->width = g->desired_width;
	inner->height = g->desired_height;
    }
    if ( outer!=NULL ) {
	outer->x = outer->y = 0;
	outer->width = g->desired_width;
	outer->height = g->desired_height;
    }
}

void _ggadget_setDesiredSize(GGadget *g,GRect *outer, GRect *inner) {
    int bp = GBoxBorderWidth(g->base,g->box);

    if ( outer!=NULL ) {
	g->desired_width = outer->width;
	g->desired_height = outer->height;
    } else if ( inner!=NULL ) {
	g->desired_width = inner->width<=0 ? -1 : inner->width+2*bp;
	g->desired_height = inner->height<=0 ? -1 : inner->height+2*bp;
    }
}

GRect *_ggadget_getsize(GGadget *g,GRect *rct) {
    *rct = g->r;
return( rct );
}

GRect *_ggadget_getinnersize(GGadget *g,GRect *rct) {
    *rct = g->inner;
return( rct );
}

void _ggadget_setvisible(GGadget *g,int visible) {
    g->state = !visible ? gs_invisible :
		g->was_disabled ? gs_disabled :
		gs_enabled;
    /* Make sure it isn't the focused _ggadget in the container !!!! */
    _ggadget_redraw(g);
}

void _ggadget_setenabled(GGadget *g,int enabled) {
    g->was_disabled = !enabled;
    if ( g->state!=gs_invisible ) {
	g->state = enabled? gs_enabled : gs_disabled;
	_ggadget_redraw(g);
    }
    /* Make sure it isn't the focused _ggadget in the container !!!! */
}

void GGadgetDestroy(GGadget *g) {
    (g->funcs->destroy)(g);
}

void GGadgetRedraw(GGadget *g) {
    (g->funcs->redraw)(g);
}

void GGadgetMove(GGadget *g,int32_t x, int32_t y ) {
    (g->funcs->move)(g,x,y);
}

void GGadgetMoveAddToY(GGadget *g, int32_t yoffset )
{
    GRect sz;
    GGadgetGetSize( g, &sz );
    GGadgetMove( g, sz.x, sz.y + yoffset );
}



int32_t GGadgetGetX(GGadget *g)
{
    return g->r.x;
}

int32_t GGadgetGetY(GGadget *g)
{
    return g->r.y;
}

void  GGadgetSetY(GGadget *g, int32_t y )
{
    int32_t x = GGadgetGetX(g);
    GGadgetMove( g, x, y );
}



void GGadgetResize(GGadget *g,int32_t width, int32_t height ) {
    (g->funcs->resize)(g,width,height);
}

GRect *GGadgetGetSize(GGadget *g,GRect *rct) {
return( (g->funcs->getsize)(g,rct) );
}

void GGadgetSetSize(GGadget *g,GRect *rct)
{
    GGadgetResize(g,rct->width,rct->height);
}


int GGadgetContainsEventLocation(GGadget *g, GEvent* e )
{
    switch( e->type )
    {
        case et_mousemove:
        case et_mouseup: case et_mousedown:
            return( GGadgetContains( g, e->u.mouse.x, e->u.mouse.y ));
        default: break;
    }
    
    return 0;
}

int GGadgetContains(GGadget *g, int x, int y )
{
    GRect r;
    GGadgetGetSize( g, &r );
    return r.x < x && r.x+r.width > x
        && r.y < y && r.y+r.height > y;
}

GRect *GGadgetGetInnerSize(GGadget *g,GRect *rct) {
return( (g->funcs->getinnersize)(g,rct) );
}

void GGadgetSetVisible(GGadget *g,int visible) {
    (g->funcs->setvisible)(g,visible);
}

int GGadgetIsVisible(GGadget *g) {
return( g->state!=gs_invisible );
}

void GGadgetSetEnabled(GGadget *g,int enabled) {
    if(g)
	(g->funcs->setenabled)(g,enabled);
}

int GGadgetIsEnabled(GGadget *g) {
    if ( g->state==gs_invisible )
return( !g->was_disabled );
return( g->state==gs_enabled );
}

void GGadgetSetUserData(GGadget *g,void *data) {
    g->data = data;
}

void GGadgetSetPopupMsg(GGadget *g,const unichar_t *msg) {
    free(g->popup_msg);
    g->popup_msg = u_copy(msg);
}

GWindow GGadgetGetWindow(GGadget *g) {
return( g->base );
}

int GGadgetGetCid(GGadget *g) {
return( g->cid );
}

void *GGadgetGetUserData(GGadget *g) {
return( g->data );
}

int GGadgetEditCmd(GGadget *g,enum editor_commands cmd) {
    if ( g->funcs->handle_editcmd!=NULL )
return(g->funcs->handle_editcmd)(g,cmd);
return( false );
}

void GGadgetSetTitle(GGadget *g,const unichar_t *title) {
    if ( g->funcs->set_title!=NULL )
	(g->funcs->set_title)(g,title);
}

void GGadgetSetTitle8(GGadget *g,const char *title) {
    if ( g->funcs->set_title!=NULL ) {
	unichar_t *temp = utf82u_copy(title);
	(g->funcs->set_title)(g,temp);
	free(temp);
    }
}

void GGadgetSetTitle8WithMn(GGadget *g,const char *title) {
    char *pt = strchr(title,'_');
    char *freeme=NULL;
    int mnc;

    if ( pt!=NULL ) {
	char *pos = pt+1;
	mnc = utf8_ildb((const char **) &pos);
	g->mnemonic = mnc;
	freeme = copy(title);
	for ( pt = freeme + (pt-title); *pt; ++pt )
	    *pt = pt[1];
	title = freeme;
    } else
	g->mnemonic = 0;
    GGadgetSetTitle8(g,title);
    free(freeme);
}

const unichar_t *_GGadgetGetTitle(GGadget *g) {
    if ( g && g->funcs->_get_title!=NULL ) return( (g->funcs->_get_title)(g) );

    return( NULL );
}

unichar_t *GGadgetGetTitle(GGadget *g) {
    if ( g ) {
	if ( g->funcs->get_title!=NULL )
	    return( (g->funcs->get_title)(g) );
	else if ( g->funcs->_get_title!=NULL )
	    return( u_copy( (g->funcs->_get_title)(g) ));
    }

    return( NULL );
}

char *GGadgetGetTitle8(GGadget *g) {
    if ( g->funcs->_get_title!=NULL )
return( u2utf8_copy( (g->funcs->_get_title)(g) ));
    else if ( g->funcs->get_title!=NULL ) {
	unichar_t *temp = (g->funcs->get_title)(g);
	char *ret = u2utf8_copy(temp);
	free(temp);
return( ret );
    }

return( NULL );
}

void GGadgetSetFont(GGadget *g,GFont *font) {
    if ( g->funcs->set_font!=NULL )
	(g->funcs->set_font)(g,font);
}

GFont *GGadgetGetFont(GGadget *g) {
    if ( g==NULL )
return( _ggadget_default_font.fi );
    if ( g->funcs->get_font!=NULL )
return( (g->funcs->get_font)(g) );

return( NULL );
}

void GGadgetSetHandler(GGadget *g, GGadgetHandler handler) {
    g->handle_controlevent = handler;
}

GGadgetHandler GGadgetGetHandler(GGadget *g) {
return( g->handle_controlevent );
}

void GGadgetClearList(GGadget *g) {
    if ( g->funcs->clear_list!=NULL )
	(g->funcs->clear_list)(g);
}

void GGadgetSetList(GGadget *g, GTextInfo **ti, int32_t copyit) {
    if ( g->funcs->set_list!=NULL )
	(g->funcs->set_list)(g,ti,copyit);
}

GTextInfo **GGadgetGetList(GGadget *g,int32_t *len) {
    if ( g->funcs->get_list!=NULL )
return((g->funcs->get_list)(g,len));

    if ( len!=NULL ) *len = 0;
return( NULL );
}

GTextInfo *GGadgetGetListItem(GGadget *g,int32_t pos) {
    if ( g->funcs->get_list_item!=NULL )
return((g->funcs->get_list_item)(g,pos));

return( NULL );
}

GTextInfo *GGadgetGetListItemSelected(GGadget *g) {
    int pos = GGadgetGetFirstListSelectedItem(g);

    if ( pos!=-1 && g->funcs->get_list_item!=NULL )
return((g->funcs->get_list_item)(g,pos));

return( NULL );
}

void GGadgetSelectListItem(GGadget *g,int32_t pos,int32_t sel) {
    if ( g->funcs->select_list_item!=NULL )
	(g->funcs->select_list_item)(g,pos,sel);
}

void GGadgetSelectOneListItem(GGadget *g,int32_t pos) {
    if ( g->funcs->select_one_list_item!=NULL )
	(g->funcs->select_one_list_item)(g,pos);
}

int32_t GGadgetIsListItemSelected(GGadget *g,int32_t pos) {
    if ( g->funcs->is_list_item_selected!=NULL )
return((g->funcs->is_list_item_selected)(g,pos));

return( 0 );
}

int32_t GGadgetGetFirstListSelectedItem(GGadget *g) {
    if ( g->funcs->get_first_selection!=NULL )
return((g->funcs->get_first_selection)(g));
return( -1 );
}

void GGadgetScrollListToPos(GGadget *g,int32_t pos) {
    if ( g->funcs->scroll_list_to_pos!=NULL )
	(g->funcs->scroll_list_to_pos)(g,pos);
}

void GGadgetScrollListToText(GGadget *g,const unichar_t *lab,int32_t sel) {
    if ( g->funcs->scroll_list_to_text!=NULL )
	(g->funcs->scroll_list_to_text)(g,lab,sel);
}

void GGadgetSetListOrderer(GGadget *g,int (*orderer)(const void *, const void *)) {
    if ( g->funcs->set_list_orderer!=NULL )
	(g->funcs->set_list_orderer)(g,orderer);
}

void GGadgetGetDesiredSize(GGadget *g,GRect *outer, GRect *inner) {
    if ( g->state==gs_invisible ) {
	if ( outer!=NULL )
	    memset(outer,0,sizeof(*outer));
	if ( inner!=NULL )
	    memset(inner,0,sizeof(*inner));
    } else if ( ((char *) &g->funcs->get_desired_size) - ((char *) g->funcs) < g->funcs->size &&
	    g->funcs->get_desired_size!=NULL )
	(g->funcs->get_desired_size)(g,outer,inner);
    else {
	if ( outer!=NULL )
	    *outer = g->r;
	if ( inner!=NULL )
	    *inner = g->inner;
    }
}

void GGadgetGetDesiredVisibleSize(GGadget *g,GRect *outer, GRect *inner) {
    if ( ((char *) &g->funcs->get_desired_size) - ((char *) g->funcs) < g->funcs->size &&
	    g->funcs->get_desired_size!=NULL )
	(g->funcs->get_desired_size)(g,outer,inner);
    else {
	if ( outer!=NULL )
	    *outer = g->r;
	if ( inner!=NULL )
	    *inner = g->inner;
    }
}

void GGadgetSetDesiredSize(GGadget *g,GRect *outer, GRect *inner) {
    if ( ((char *) &g->funcs->set_desired_size) - ((char *) g->funcs) < g->funcs->size &&
	    g->funcs->set_desired_size!=NULL )
	(g->funcs->set_desired_size)(g,outer,inner);
}

int GGadgetFillsWindow(GGadget *g) {
    if ( ((char *) &g->funcs->fills_window) - ((char *) g->funcs) < g->funcs->size &&
	    g->funcs->fills_window!=NULL )
return( (g->funcs->fills_window)(g) );

return( false );
}

int GGadgetIsDefault(GGadget *g) {
    if ( ((char *) &g->funcs->is_default) - ((char *) g->funcs) < g->funcs->size &&
	    g->funcs->is_default!=NULL )
return( (g->funcs->is_default)(g) );

return( false );
}

void GGadgetsCreate(GWindow base, GGadgetCreateData *gcd) {
    int i;

    for ( i=0; gcd[i].creator!=NULL; ++i )
	gcd[i].ret = (gcd[i].creator)(base,&gcd[i].gd,gcd[i].data);
}

int GGadgetDispatchEvent(GGadget *g, GEvent *event) {

    if ( g==NULL || event==NULL )
return( false );
    switch ( event->type ) {
      case et_expose:
	  if ( g->funcs->handle_expose )
return( (g->funcs->handle_expose)(g->base,g,event) );
      break;
      case et_mouseup: case et_mousedown: case et_mousemove: case et_crossing:
	  if ( g->funcs->handle_mouse )
return( (g->funcs->handle_mouse)(g,event) );
      break;
      case et_char: case et_charup:
	if ( g->funcs->handle_key ) {
	    int ret;
	    int old = g->takes_keyboard;
	    g->takes_keyboard = true;
	    ret =(g->funcs->handle_key)(g,event);
	    g->takes_keyboard = old;
return( ret );
	}
      break;
      case et_drag: case et_dragout: case et_drop: case et_selclear:
	  if ( g->funcs->handle_sel )
return( (g->funcs->handle_sel)(g,event) );
      break;
      case et_timer:
	  if ( g->funcs->handle_timer )
return( (g->funcs->handle_timer)(g,event) );
      break;
      case et_controlevent:
	if ( g->handle_controlevent!=NULL )
return( (g->handle_controlevent)(g,event) );
	else
	    GDrawPostEvent(event);
return( true );
      default: break;
    }
return( false );
}

void GGadgetTakesKeyboard(GGadget *g, int takes_keyboard) {
    g->takes_keyboard = takes_keyboard;
}

void GGadgetSetSkipHotkeyProcessing( GGadget *g, int v )
{
    if( !g )
	return;

    printf("GGadgetSetSkipHotkeyProcessing1 %d\n", g->state );
    g->gg_skip_hotkey_processing = v;
    printf("GGadgetSetSkipHotkeyProcessing2 %d\n", g->state );
}

int GGadgetGetSkipHotkeyProcessing( GGadget *g )
{
    if( !g )
	return 0;

    return (g->gg_skip_hotkey_processing);
}

void GGadgetSetSkipUnQualifiedHotkeyProcessing( GGadget *g, int v )
{
    if( !g )
	return;
    g->gg_skip_unqualified_hotkey_processing = v;
}

int GGadgetGetSkipUnQualifiedHotkeyProcessing( GGadget *g )
{
    if( !g )
	return 0;

    return (g->gg_skip_unqualified_hotkey_processing);
}

