/* Copyright (C) 2000,2001 by George Williams */
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
#include "pfaeditui.h"
#include <gkeysym.h>
#include <math.h>
#include "splinefont.h"
#include "ustring.h"

static unichar_t poppointer[] = { 'P','o','i','n','t','e','r',  '\0' };
static unichar_t popmag[] = { 'M','a','g','n','i','f','y',' ','(','M','i','n','i','f','y',' ','w','i','t','h',' ','a','l','t',')',  '\0' };
static unichar_t popcurve[] = { 'A','d','d',' ','a',' ','c','u','r','v','e',' ','p','o','i','n','t',  '\0' };
static unichar_t popcorner[] = { 'A','d','d',' ','a',' ','c','o','r','n','e','r',' ','p','o','i','n','t',  '\0' };
static unichar_t poptangent[] = { 'A','d','d',' ','a',' ','t','a','n','g','e','n','t',' ','p','o','i','n','t',  '\0' };
static unichar_t poppen[] = { 'A','d','d',' ','a',' ','p','o','i','n','t',',',' ','d','r','a','g',' ','o','u','t',' ','i','t','s',' ','c','o','n','t','r','o','l',' ','p','o','i','n','t','s',  '\0' };
static unichar_t popknife[] = { 'C','u','t',' ','s','p','l','i','n','e','s',' ','i','n',' ','t','w','o',  '\0' };
static unichar_t popruler[] = { 'M','e','a','s','u','r','e',' ','d','i','s','t','a','n','c','e',',',' ','a','n','g','l','e',' ','b','e','t','w','e','e','n',' ','p','o','i','n','t','s',  '\0' };
static unichar_t popscale[] = { 'S','c','a','l','e',' ','t','h','e',' ','s','e','l','e','c','t','i','o','n',  '\0' };
static unichar_t popflip[] = { 'F','l','i','p',' ','t','h','e',' ','s','e','l','e','c','t','i','o','n',  '\0' };
static unichar_t poprotate[] = { 'R','o','t','a','t','e',' ','t','h','e',' ','s','e','l','e','c','t','i','o','n',  '\0' };
static unichar_t popskew[] = { 'S','k','e','w',' ','t','h','e',' ','s','e','l','e','c','t','i','o','n',  '\0' };
static unichar_t poprectelipse[] = { 'R','e','c','t', 'a','n','g','l','e',' ','o','r',' ','E','l','i','p','s','e',  '\0' };
static unichar_t poppolystar[] = { 'P','o','l','y', 'g','o','n',' ','o','r',' ','S','t','a','r',  '\0' };
static unichar_t poppencil[] = { 'S','e','t','/','C','l','e','a','r',' ','P','i','x','e','l', 's',  '\0' };
static unichar_t popline[] = { 'D','r','a','w',' ','a',' ','L','i','n','e',  '\0' };
static unichar_t popshift[] = { 'S','h','i','f','t',' ','E','n','t','i','r','e',' ','B','i', 't','m','a','p',  '\0' };
static unichar_t pophand[] = { 'S','c','r','o','l','l',' ','B','i', 't','m','a','p',  '\0' };
/* Note: If you change this ordering, change enum cvtools */
static unichar_t *popups[] = { poppointer, popmag,
				    popcurve, popcorner,
			            poptangent, poppen,
			            popknife, popruler,
			            popscale, popflip,
			            poprotate, popskew,
			            poprectelipse, poppolystar,
			            poprectelipse, poppolystar};
static int rectelipse=0, polystar=0, regular_star=1;
static double rr_radius=0;
static int ps_pointcnt=6;
static double star_percent=1.7320508;	/* Regular 6 pointed star */

double CVRoundRectRadius(void) {
return( rr_radius );
}

int CVPolyStarPoints(void) {
return( ps_pointcnt );
}

double CVStarRatio(void) {
    if ( regular_star )
return( sin(3.1415926535897932/ps_pointcnt)*tan(2*3.1415926535897932/ps_pointcnt)+cos(3.1415926535897932/ps_pointcnt) );
	
return( star_percent );
}
    
struct ask_info {
    GWindow gw;
    int done;
    int ret;
    double *val;
    GGadget *rb1;
    GGadget *reg;
    int isint;
    char *lab;
};
#define CID_ValText		1001
#define CID_PointPercent	1002

static int TA_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct ask_info *d = GDrawGetUserData(GGadgetGetWindow(g));
	double val, val2=0;
	int err=0;
	if ( d->isint ) {
	    val = GetInt(d->gw,CID_ValText,d->lab,&err);
	    if ( !(regular_star = GGadgetIsChecked(d->reg)))
		val2 = GetDouble(d->gw,CID_PointPercent,"Size of Points",&err);
	} else
	    val = GetDouble(d->gw,CID_ValText,d->lab,&err);
	if ( err )
return( true );
	*d->val = val;
	d->ret = !GGadgetIsChecked(d->rb1);
	d->done = true;
	if ( !regular_star && d->isint )
	    star_percent = val2/100;
    }
return( true );
}

static int TA_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct ask_info *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int toolask_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct ask_info *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

static int Ask(char *rb1, char *rb2, int rb, char *lab, double *val, int isint ) {
    struct ask_info d;
    char buffer[20], buf[20];
    static unichar_t title[] = { 'S','h','a','p','e',' ','T','y','p','e',  '\0' };
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[11];
    GTextInfo label[11];
    int off = isint?30:0;

    d.done = false;
    d.ret = rb;
    d.val = val;
    d.isint = isint;
    d.lab = lab;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = title;
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,190);
	pos.height = GDrawPointsToPixels(NULL,105+off);
	d.gw = GDrawCreateTopWindow(NULL,&pos,toolask_e_h,&d,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) rb1;
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible | (rb==0?gg_cb_on:0);
	gcd[0].creator = GRadioCreate;

	label[1].text = (unichar_t *) rb2;
	label[1].text_is_1byte = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = isint?65:75; gcd[1].gd.pos.y = 5; 
	gcd[1].gd.flags = gg_enabled|gg_visible | (rb==1?gg_cb_on:0);
	gcd[1].creator = GRadioCreate;

	label[2].text = (unichar_t *) lab;
	label[2].text_is_1byte = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = 25; 
	gcd[2].gd.flags = gg_enabled|gg_visible ;
	gcd[2].creator = GLabelCreate;

	sprintf( buffer, "%g", *val );
	label[3].text = (unichar_t *) buffer;
	label[3].text_is_1byte = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 40; 
	gcd[3].gd.flags = gg_enabled|gg_visible ;
	gcd[3].gd.cid = CID_ValText;
	gcd[3].creator = GTextFieldCreate;

	gcd[4].gd.pos.x = 20-3; gcd[4].gd.pos.y = 70+off;
	gcd[4].gd.pos.width = 55+6; gcd[4].gd.pos.height = 0;
	gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[4].text = (unichar_t *) "OK";
	label[4].text_is_1byte = true;
	gcd[4].gd.mnemonic = 'O';
	gcd[4].gd.label = &label[4];
	gcd[4].gd.handle_controlevent = TA_OK;
	gcd[4].creator = GButtonCreate;

	gcd[5].gd.pos.x = 170-55-20; gcd[5].gd.pos.y = 70+3+off;
	gcd[5].gd.pos.width = 55; gcd[5].gd.pos.height = 0;
	gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[5].text = (unichar_t *) "Cancel";
	label[5].text_is_1byte = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.mnemonic = 'C';
	gcd[5].gd.handle_controlevent = TA_Cancel;
	gcd[5].creator = GButtonCreate;

	if ( isint ) {
	    label[6].text = (unichar_t *) "Regular";
	    label[6].text_is_1byte = true;
	    gcd[6].gd.label = &label[6];
	    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = 70; 
	    gcd[6].gd.flags = gg_enabled|gg_visible | (rb==0?gg_cb_on:0);
	    gcd[6].creator = GRadioCreate;

	    label[7].text = (unichar_t *) "Points:";
	    label[7].text_is_1byte = true;
	    gcd[7].gd.label = &label[7];
	    gcd[7].gd.pos.x = 65; gcd[7].gd.pos.y = 70; 
	    gcd[7].gd.flags = gg_enabled|gg_visible | (rb==1?gg_cb_on:0);
	    gcd[7].creator = GRadioCreate;

	    sprintf( buf, "%4g", star_percent*100 );
	    label[8].text = (unichar_t *) buf;
	    label[8].text_is_1byte = true;
	    gcd[8].gd.label = &label[8];
	    gcd[8].gd.pos.x = 125; gcd[8].gd.pos.y = 70;  gcd[8].gd.pos.width=50;
	    gcd[8].gd.flags = gg_enabled|gg_visible ;
	    gcd[8].gd.cid = CID_PointPercent;
	    gcd[8].creator = GTextFieldCreate;

	    label[9].text = (unichar_t *) "%";
	    label[9].text_is_1byte = true;
	    gcd[9].gd.label = &label[9];
	    gcd[9].gd.pos.x = 180; gcd[9].gd.pos.y = 70; 
	    gcd[9].gd.flags = gg_enabled|gg_visible ;
	    gcd[9].creator = GLabelCreate;
	}
	GGadgetsCreate(d.gw,gcd);
    d.rb1 = gcd[0].ret;
    d.reg = gcd[6].ret;

    GWidgetHidePalettes();
    GDrawSetVisible(d.gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(d.gw);
return( d.ret );
}

static void CVRectElipse(CharView *cv) {
    rectelipse = Ask("Rectangle","Elipse",rectelipse,
	    "Round Rectangle Radius",&rr_radius,false);
    GDrawRequestExpose(cv->tools,NULL,false);
}

static void CVPolyStar(CharView *cv) {
    double temp = ps_pointcnt;
    polystar = Ask("Polygon","Star",polystar,
	    "Number of star points/Polygon verteces",&temp,true);
    ps_pointcnt = temp;
}

static void ToolsExpose(GWindow pixmap, CharView *cv, GRect *r) {
    GRect old;
    /* Note: If you change this ordering, change enum cvtools */
    static GImage *buttons[][2] = { { &GIcon_pointer, &GIcon_magnify },
				    { &GIcon_curve, &GIcon_corner },
				    { &GIcon_tangent, &GIcon_pen },
			            { &GIcon_knife, &GIcon_ruler },
			            { &GIcon_scale, &GIcon_flip },
			            { &GIcon_rotate, &GIcon_skew },
			            { &GIcon_rect, &GIcon_poly},
			            { &GIcon_elipse, &GIcon_star}};
    int i,j,norm, mi;
    int tool = cv->cntrldown?cv->cb1_tool:cv->b1_tool;
    int dither = GDrawSetDither(NULL,false);

    GDrawPushClip(pixmap,r,&old);
    for ( i=0; i<sizeof(buttons)/sizeof(buttons[0]); ++i ) for ( j=0; j<2; ++j ) {
	mi = i;
	if ( i==(cvt_rect)/2 && ((j==0 && rectelipse) || (j==1 && polystar)) )
	    ++mi;
	GDrawDrawImage(pixmap,buttons[mi][j],NULL,j*27+1,i*27+1);
	norm = (mi*2+j!=tool);
	GDrawDrawLine(pixmap,j*27,i*27,j*27+25,i*27,norm?0xe0e0e0:0x707070);
	GDrawDrawLine(pixmap,j*27,i*27,j*27,i*27+25,norm?0xe0e0e0:0x707070);
	GDrawDrawLine(pixmap,j*27,i*27+25,j*27+25,i*27+25,norm?0x707070:0xe0e0e0);
	GDrawDrawLine(pixmap,j*27+25,i*27,j*27+25,i*27+25,norm?0x707070:0xe0e0e0);
    }
    GDrawPopClip(pixmap,&old);
    GDrawSetDither(NULL,dither);
}

int TrueCharState(GEvent *event) {
    int bit = 0;
    /* X doesn't set the state until after the event. I want the state to */
    /*  reflect whatever key just got depressed/released */
    int keysym = event->u.chr.keysym;

    if ( keysym == GK_Meta_L || keysym == GK_Meta_R ||
	    keysym == GK_Alt_L || keysym == GK_Alt_R )
	bit = ksm_meta;
    else if ( keysym == GK_Shift_L || keysym == GK_Shift_R )
	bit = ksm_shift;
    else if ( keysym == GK_Control_L || keysym == GK_Control_R )
	bit = ksm_control;
    else if ( keysym == GK_Caps_Lock || keysym == GK_Shift_Lock )
	bit = ksm_capslock;
    else
return( event->u.chr.state );

    if ( event->type == et_char )
return( event->u.chr.state | bit );
    else
return( event->u.chr.state & ~bit );
}

void CVToolsSetCursor(CharView *cv, int state) {
    int shouldshow;
    static enum cvtools tools[cvt_max+1] = { cvt_none };
    int cntrl;

    if ( tools[0] == cvt_none ) {
	tools[cvt_pointer] = ct_mypointer;
	tools[cvt_magnify] = ct_magplus;
	tools[cvt_curve] = ct_circle;
	tools[cvt_corner] = ct_square;
	tools[cvt_tangent] = ct_triangle;
	tools[cvt_pen] = ct_pen;
	tools[cvt_knife] = ct_knife;
	tools[cvt_ruler] = ct_ruler;
	tools[cvt_scale] = ct_scale;
	tools[cvt_flip] = ct_flip;
	tools[cvt_rotate] = ct_rotate;
	tools[cvt_skew] = ct_skew;
	tools[cvt_rect] = ct_rect;
	tools[cvt_poly] = ct_poly;
	tools[cvt_elipse] = ct_elipse;
	tools[cvt_star] = ct_star;
	tools[cvt_minify] = ct_magminus;
    }

    if ( cv->active_tool!=cvt_none )
	shouldshow = cv->active_tool;
    else if ( cv->pressed_display!=cvt_none )
	shouldshow = cv->pressed_display;
    else if ( (state&ksm_control) && (state&ksm_button2) )
	shouldshow = cv->cb2_tool;
    else if ( (state&ksm_button2) )
	shouldshow = cv->b2_tool;
    else if ( (state&ksm_control) )
	shouldshow = cv->cb1_tool;
    else
	shouldshow = cv->b1_tool;
    if ( shouldshow==cvt_magnify && (state&ksm_meta))
	shouldshow = cvt_minify;
    if ( shouldshow!=cv->showing_tool ) {
	GDrawSetCursor(cv->v,tools[shouldshow]);
	GDrawSetCursor(cv->tools,tools[shouldshow]);
	cv->showing_tool = shouldshow;
    }

    cntrl = (state&ksm_control)?1:0;
    if ( cntrl != cv->cntrldown ) {
	cv->cntrldown = cntrl;
	GDrawRequestExpose(cv->tools,NULL,false);
    }
}

static void ToolsMouse(CharView *cv, GEvent *event) {
    int i = (event->u.mouse.y/27), j = (event->u.mouse.x/27), mi=i;
    int pos;

    if ( i==(cvt_rect)/2 && ((j==0 && rectelipse) || (j==1 && polystar)) )
	++mi;
    pos = mi*2 + j;
    GGadgetEndPopup();
    if ( pos<0 || pos>=cvt_max )
	pos = cvt_none;
    if ( event->type == et_mousedown ) {
	cv->pressed_tool = cv->pressed_display = pos;
	cv->had_control = (event->u.mouse.state&ksm_control)?1:0;
	event->u.chr.state |= (1<<(7+event->u.mouse.button));
    } else if ( event->type == et_mousemove ) {
	if ( cv->pressed_tool==cvt_none && pos!=cvt_none )
	    /* Not pressed */
	    GGadgetPreparePopup(cv->tools,popups[pos]);
	else if ( pos!=cv->pressed_tool || cv->had_control != ((event->u.mouse.state&ksm_control)?1:0) )
	    cv->pressed_display = cvt_none;
	else
	    cv->pressed_display = cv->pressed_tool;
    } else if ( event->type == et_mouseup ) {
	if ( i==cvt_rect/2 && event->u.mouse.clicks==2 ) {
	    ((j==0)?CVRectElipse:CVPolyStar)(cv);
	    mi = i;
	    if ( (j==0 && rectelipse) || (j==1 && polystar) )
		++mi;
	    pos = mi*2 + j;
	    cv->pressed_tool = cv->pressed_display = pos;
	}
	if ( pos!=cv->pressed_tool || cv->had_control != ((event->u.mouse.state&ksm_control)?1:0) )
	    cv->pressed_tool = cv->pressed_display = cvt_none;
	else {
	    if ( cv->had_control && event->u.mouse.button==2 )
		cv->cb2_tool = pos;
	    else if ( event->u.mouse.button==2 )
		cv->b2_tool = pos;
	    else if ( cv->had_control ) {
		if ( cv->cb1_tool!=pos ) {
		    cv->cb1_tool = pos;
		    GDrawRequestExpose(cv->tools,NULL,false);
		}
	    } else {
		if ( cv->b1_tool!=pos ) {
		    cv->b1_tool = pos;
		    GDrawRequestExpose(cv->tools,NULL,false);
		}
	    }
	    cv->pressed_tool = cv->pressed_display = cvt_none;
	}
	event->u.chr.state &= ~(1<<(7+event->u.mouse.button));
    }
    CVToolsSetCursor(cv,event->u.chr.state);
}

static int cvtools_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	ToolsExpose(gw,cv,&event->u.expose.rect);
      break;
      case et_mousedown:
	ToolsMouse(cv,event);
      break;
      case et_mousemove:
	ToolsMouse(cv,event);
      break;
      case et_mouseup:
	ToolsMouse(cv,event);
      break;
      case et_crossing:
	cv->pressed_display = cvt_none;
	CVToolsSetCursor(cv,event->u.mouse.state);
      break;
      case et_char: case et_charup:
	if ( cv->had_control != ((event->u.chr.state&ksm_control)?1:0) )
	    cv->pressed_display = cvt_none;
	CVChar(cv,event);
      break;
      case et_destroy:
      break;
      case et_close:
	GDrawSetVisible(gw,false);
      break;
    }
return( true );
}

GWindow CVMakeTools(CharView *cv) {
    GRect r;
    GWindowAttrs wattrs;
    unichar_t title[] = { 'T', 'o', 'o', 'l', 's', '\0' };
    GWindow tools;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_positioned|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.window_title = title;

    r.width = 53; r.height = 187;
    r.x = -r.width-6; r.y = cv->mbh+20;
    tools = GWidgetCreatePalette( cv->gw, &r, cvtools_e_h, cv, &wattrs );
    GWidgetRequestVisiblePalette(tools,true);
return( tools );
}

static void CVPopupInvoked(GWindow v, GMenuItem *mi, GEvent *e) {
    CharView *cv = (CharView *) GDrawGetUserData(v);
    int pos;

    pos = mi->mid;
    if ( (pos==12 && rectelipse) || (pos==13 && polystar ))
	pos += 2;
    if ( cv->had_control ) {
	if ( cv->cb1_tool!=pos ) {
	    cv->cb1_tool = pos;
	    GDrawRequestExpose(cv->tools,NULL,false);
	}
    } else {
	if ( cv->b1_tool!=pos ) {
	    cv->b1_tool = pos;
	    GDrawRequestExpose(cv->tools,NULL,false);
	}
    }
    CVToolsSetCursor(cv,cv->had_control?ksm_control:0);
}

void CVToolsPopup(CharView *cv, GEvent *event) {
    GMenuItem mi[16];
    int i;

    memset(mi,'\0',sizeof(mi));
    for ( i=0;i<14; ++i ) {
	mi[i].ti.text = popups[i];
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = i;
	mi[i].invoke = CVPopupInvoked;
    }
    cv->had_control = (event->u.mouse.state&ksm_control)?1:0;
    GMenuCreatePopupMenu(cv->v,event, mi);
}


#define CID_VFore	1001
#define CID_VBack	1002
#define CID_VGrid	1003
#define CID_VHints	1004
#define CID_EFore	1005
#define CID_EBack	1006
#define CID_EGrid	1007

static int cvlayers_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	GDrawSetVisible(gw,false);
      break;
      case et_char:
	CVChar(cv,event);
      break;
      case et_controlevent:
	if ( event->u.control.subtype == et_radiochanged ) {
	    switch(GGadgetGetCid(event->u.control.g)) {
	      case CID_VFore:
		CVShows.showfore = cv->showfore = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VBack:
		CVShows.showback = cv->showback = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VGrid:
		CVShows.showgrids = cv->showgrids = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VHints:
		CVShows.showhints = cv->showhints = GGadgetIsChecked(event->u.control.g);
		cv->back_img_out_of_date = true;	/* only this cv */
	      break;
	      case CID_EFore:
		cv->drawmode = dm_fore;
	      break;
	      case CID_EBack:
		cv->drawmode = dm_back;
	      break;
	      case CID_EGrid:
		cv->drawmode = dm_grid;
	      break;
	    }
	    GDrawRequestExpose(cv->v,NULL,false);
	}
      break;
    }
return( true );
}

GWindow CVMakeLayers(CharView *cv) {
    GRect r;
    GWindowAttrs wattrs;
    unichar_t title[] = { 'L', 'a', 'y', 'e', 'r', 's', '\0' };
    GWindow layers;
    GGadgetCreateData gcd[13];
    GTextInfo label[13];
    static GBox radio_box = { bt_none, bs_rect, 0, 0, 0, 0, 0,0,0,0, COLOR_DEFAULT,COLOR_DEFAULT };
    static unichar_t isvis[] =  { 'I', 's', ' ', 'L', 'a', 'y', 'e', 'r', ' ', 'V', 'i', 's', 'i', 'b', 'l', 'e', '?',  '\0' };
    static unichar_t isedit[] = { 'I', 's', ' ', 'L', 'a', 'y', 'e', 'r', ' ', 'E', 'd', 'i', 't', 'a', 'b', 'l', 'e', '?',  '\0' };
    static unichar_t helv[] = { 'h', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a', '\0' };
    GFont *font;
    FontRequest rq;
    int i;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_positioned|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.window_title = title;

    r.width = 85; r.height = 94;
    r.x = -r.width-6; r.y = cv->mbh+187+45/*25*/;	/* 45 is right if there's decor, 25 when none. twm gives none, kde gives decor */
    layers = GWidgetCreatePalette( cv->gw, &r, cvlayers_e_h, cv, &wattrs );

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    memset(&rq,'\0',sizeof(rq));
    rq.family_name = helv;
    rq.point_size = -12;
    rq.weight = 400;
    font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(layers),&rq);
    for ( i=0; i<sizeof(label)/sizeof(label[0]); ++i )
	label[i].font = font;

    label[0].text = (unichar_t *) "V";
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 7; gcd[0].gd.pos.y = 5; 
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[0].gd.popup_msg = isvis;
    gcd[0].creator = GLabelCreate;

    label[1].text = (unichar_t *) "E";
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 30; gcd[1].gd.pos.y = 5; 
    gcd[1].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[1].gd.popup_msg = isedit;
    gcd[1].creator = GLabelCreate;

    label[2].text = (unichar_t *) "Layer";
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 47; gcd[2].gd.pos.y = 5; 
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[2].gd.popup_msg = isedit;
    gcd[2].creator = GLabelCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 21; 
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[3].gd.cid = CID_VFore;
    gcd[3].gd.popup_msg = isvis;
    gcd[3].gd.box = &radio_box;
    gcd[3].creator = GCheckBoxCreate;

    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = 38; 
    gcd[4].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[4].gd.cid = CID_VBack;
    gcd[4].gd.popup_msg = isvis;
    gcd[4].gd.box = &radio_box;
    gcd[4].creator = GCheckBoxCreate;

    gcd[5].gd.pos.x = 5; gcd[5].gd.pos.y = 55; 
    gcd[5].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[5].gd.cid = CID_VGrid;
    gcd[5].gd.popup_msg = isvis;
    gcd[5].gd.box = &radio_box;
    gcd[5].creator = GCheckBoxCreate;

    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = 72; 
    gcd[6].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[6].gd.cid = CID_VHints;
    gcd[6].gd.popup_msg = isvis;
    gcd[6].gd.box = &radio_box;
    gcd[6].creator = GCheckBoxCreate;


    label[7].text = (unichar_t *) "Fore";
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = 27; gcd[7].gd.pos.y = 21; 
    gcd[7].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[7].gd.cid = CID_EFore;
    gcd[7].gd.popup_msg = isedit;
    gcd[7].gd.box = &radio_box;
    gcd[7].creator = GRadioCreate;

    label[8].text = (unichar_t *) "Back";
    label[8].text_is_1byte = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.pos.x = 27; gcd[8].gd.pos.y = 38; 
    gcd[8].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[8].gd.cid = CID_EBack;
    gcd[8].gd.popup_msg = isedit;
    gcd[8].gd.box = &radio_box;
    gcd[8].creator = GRadioCreate;

    label[9].text = (unichar_t *) "Grid";
    label[9].text_is_1byte = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.pos.x = 27; gcd[9].gd.pos.y = 55; 
    gcd[9].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[9].gd.cid = CID_EGrid;
    gcd[9].gd.popup_msg = isedit;
    gcd[9].gd.box = &radio_box;
    gcd[9].creator = GRadioCreate;

    label[10].text = (unichar_t *) "Hints";
    label[10].text_is_1byte = true;
    gcd[10].gd.label = &label[10];
    gcd[10].gd.pos.x = 47; gcd[10].gd.pos.y = 72; 
    gcd[10].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[10].creator = GLabelCreate;

    gcd[11].gd.pos.x = 1; gcd[11].gd.pos.y = 1;
    gcd[11].gd.pos.width = 83; gcd[11].gd.pos.height = 92;
    gcd[11].gd.flags = gg_enabled | gg_visible|gg_pos_in_pixels;
    gcd[11].creator = GGroupCreate;

    gcd[7+cv->drawmode].gd.flags |= gg_cb_on;
    if ( cv->showfore ) gcd[3].gd.flags |= gg_cb_on;
    if ( cv->showback ) gcd[4].gd.flags |= gg_cb_on;
    if ( cv->showgrids ) gcd[5].gd.flags |= gg_cb_on;
    if ( cv->showhints ) gcd[6].gd.flags |= gg_cb_on;

    GGadgetsCreate(layers,gcd);
    GWidgetRequestVisiblePalette(layers,true);
return( layers );
}

static int bvlayers_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	GDrawSetVisible(gw,false);
      break;
      case et_char:
	BVChar(bv,event);
      break;
      case et_controlevent:
	if ( event->u.control.subtype == et_radiochanged ) {
	    switch(GGadgetGetCid(event->u.control.g)) {
	      case CID_VFore:
		BVShows.showfore = bv->showfore = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VBack:
		BVShows.showoutline = bv->showoutline = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VGrid:
		BVShows.showgrid = bv->showgrid = GGadgetIsChecked(event->u.control.g);
	      break;
	    }
	    GDrawRequestExpose(bv->v,NULL,false);
	}
      break;
    }
return( true );
}

GWindow BVMakeLayers(BitmapView *bv) {
    GRect r;
    GWindowAttrs wattrs;
    unichar_t title[] = { 'L', 'a', 'y', 'e', 'r', 's', '\0' };
    GWindow layers;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    static GBox radio_box = { bt_none, bs_rect, 0, 0, 0, 0, 0,0,0,0, COLOR_DEFAULT,COLOR_DEFAULT };
    static unichar_t isvis[] =  { 'I', 's', ' ', 'L', 'a', 'y', 'e', 'r', ' ', 'V', 'i', 's', 'i', 'b', 'l', 'e', '?',  '\0' };
    static unichar_t helv[] = { 'h', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a', '\0' };
    GFont *font;
    FontRequest rq;
    int i;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_positioned|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.window_title = title;

    r.width = 73; r.height = 73;
    r.x = -r.width-6; r.y = bv->mbh+81+45/*25*/;	/* 45 is right if there's decor, is in kde, not in twm. Sigh */
    layers = GWidgetCreatePalette( bv->gw, &r, bvlayers_e_h, bv, &wattrs );

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    memset(&rq,'\0',sizeof(rq));
    rq.family_name = helv;
    rq.point_size = -12;
    rq.weight = 400;
    font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(layers),&rq);
    for ( i=0; i<sizeof(label)/sizeof(label[0]); ++i )
	label[i].font = font;

    label[0].text = (unichar_t *) "V";
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 7; gcd[0].gd.pos.y = 5; 
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[0].gd.popup_msg = isvis;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 1; gcd[1].gd.pos.y = 1;
    gcd[1].gd.pos.width = r.width-2; gcd[1].gd.pos.height = r.height-1;
    gcd[1].gd.flags = gg_enabled | gg_visible|gg_pos_in_pixels;
    gcd[1].creator = GGroupCreate;

    label[2].text = (unichar_t *) "Layer";
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 23; gcd[2].gd.pos.y = 5; 
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[2].gd.popup_msg = isvis;
    gcd[2].creator = GLabelCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 21; 
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[3].gd.cid = CID_VFore;
    gcd[3].gd.popup_msg = isvis;
    gcd[3].gd.box = &radio_box;
    gcd[3].creator = GCheckBoxCreate;
    label[3].text = (unichar_t *) "Bitmap";
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];

    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = 37; 
    gcd[4].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[4].gd.cid = CID_VBack;
    gcd[4].gd.popup_msg = isvis;
    gcd[4].gd.box = &radio_box;
    gcd[4].creator = GCheckBoxCreate;
    label[4].text = (unichar_t *) "Outline";
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];

    gcd[5].gd.pos.x = 5; gcd[5].gd.pos.y = 53; 
    gcd[5].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[5].gd.cid = CID_VGrid;
    gcd[5].gd.popup_msg = isvis;
    gcd[5].gd.box = &radio_box;
    gcd[5].creator = GCheckBoxCreate;
    label[5].text = (unichar_t *) "Grid";
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];

    if ( bv->showfore ) gcd[3].gd.flags |= gg_cb_on;
    if ( bv->showoutline ) gcd[4].gd.flags |= gg_cb_on;
    if ( bv->showgrid ) gcd[5].gd.flags |= gg_cb_on;

    GGadgetsCreate(layers,gcd);
    GWidgetRequestVisiblePalette(layers,true);
return( layers );
}

static unichar_t *bvpopups[] = { poppointer, popmag,
				    poppencil, popline,
			            popshift, pophand };

static void BVToolsExpose(GWindow pixmap, BitmapView *bv, GRect *r) {
    GRect old;
    /* Note: If you change this ordering, change enum bvtools */
    static GImage *buttons[][2] = { { &GIcon_pointer, &GIcon_magnify },
				    { &GIcon_pencil, &GIcon_line },
			            { &GIcon_shift, &GIcon_hand }};
    int i,j,norm;
    int tool = bv->cntrldown?bv->cb1_tool:bv->b1_tool;
    int dither = GDrawSetDither(NULL,false);

    GDrawPushClip(pixmap,r,&old);
    for ( i=0; i<sizeof(buttons)/sizeof(buttons[0]); ++i ) for ( j=0; j<2; ++j ) {
	GDrawDrawImage(pixmap,buttons[i][j],NULL,j*27+1,i*27+1);
	norm = (i*2+j!=tool);
	GDrawDrawLine(pixmap,j*27,i*27,j*27+25,i*27,norm?0xe0e0e0:0x707070);
	GDrawDrawLine(pixmap,j*27,i*27,j*27,i*27+25,norm?0xe0e0e0:0x707070);
	GDrawDrawLine(pixmap,j*27,i*27+25,j*27+25,i*27+25,norm?0x707070:0xe0e0e0);
	GDrawDrawLine(pixmap,j*27+25,i*27,j*27+25,i*27+25,norm?0x707070:0xe0e0e0);
    }
    GDrawPopClip(pixmap,&old);
    GDrawSetDither(NULL,dither);
}

void BVToolsSetCursor(BitmapView *bv, int state) {
    int shouldshow;
    static enum bvtools tools[bvt_max2+1] = { bvt_none };
    int cntrl;

    if ( tools[0] == bvt_none ) {
	tools[bvt_pointer] = ct_mypointer;
	tools[bvt_magnify] = ct_magplus;
	tools[bvt_pencil] = ct_pencil;
	tools[bvt_line] = ct_line;
	tools[bvt_shift] = ct_shift;
	tools[bvt_hand] = ct_myhand;
	tools[bvt_minify] = ct_magminus;
	tools[bvt_setwidth] = ct_setwidth;
	tools[bvt_rect] = ct_rect;
	tools[bvt_filledrect] = ct_filledrect;
	tools[bvt_elipse] = ct_elipse;
	tools[bvt_filledelipse] = ct_filledelipse;
    }

    if ( bv->active_tool!=bvt_none )
	shouldshow = bv->active_tool;
    else if ( bv->pressed_display!=bvt_none )
	shouldshow = bv->pressed_display;
    else if ( (state&ksm_control) && (state&ksm_button2) )
	shouldshow = bv->cb2_tool;
    else if ( (state&ksm_button2) )
	shouldshow = bv->b2_tool;
    else if ( (state&ksm_control) )
	shouldshow = bv->cb1_tool;
    else
	shouldshow = bv->b1_tool;
    if ( shouldshow==bvt_magnify && (state&ksm_meta))
	shouldshow = bvt_minify;
    if ( shouldshow!=bv->showing_tool ) {
	GDrawSetCursor(bv->v,tools[shouldshow]);
	GDrawSetCursor(bv->tools,tools[shouldshow]);
	bv->showing_tool = shouldshow;
    }

    cntrl = (state&ksm_control)?1:0;
    if ( cntrl != bv->cntrldown ) {
	bv->cntrldown = cntrl;
	GDrawRequestExpose(bv->tools,NULL,false);
    }
}

static void BVToolsMouse(BitmapView *bv, GEvent *event) {
    int i = (event->u.mouse.y/27), j = (event->u.mouse.x/27);
    int pos;

    pos = i*2 + j;
    GGadgetEndPopup();
    if ( pos<0 || pos>=bvt_max )
	pos = bvt_none;
    if ( event->type == et_mousedown ) {
	bv->pressed_tool = bv->pressed_display = pos;
	bv->had_control = (event->u.mouse.state&ksm_control)?1:0;
	event->u.chr.state |= (1<<(7+event->u.mouse.button));
    } else if ( event->type == et_mousemove ) {
	if ( bv->pressed_tool==bvt_none && pos!=bvt_none )
	    /* Not pressed */
	    GGadgetPreparePopup(bv->tools,bvpopups[pos]);
	else if ( pos!=bv->pressed_tool || bv->had_control != ((event->u.mouse.state&ksm_control)?1:0) )
	    bv->pressed_display = bvt_none;
	else
	    bv->pressed_display = bv->pressed_tool;
    } else if ( event->type == et_mouseup ) {
	if ( pos!=bv->pressed_tool || bv->had_control != ((event->u.mouse.state&ksm_control)?1:0) )
	    bv->pressed_tool = bv->pressed_display = bvt_none;
	else {
	    if ( bv->had_control && event->u.mouse.button==2 )
		bv->cb2_tool = pos;
	    else if ( event->u.mouse.button==2 )
		bv->b2_tool = pos;
	    else if ( bv->had_control ) {
		if ( bv->cb1_tool!=pos ) {
		    bv->cb1_tool = pos;
		    GDrawRequestExpose(bv->tools,NULL,false);
		}
	    } else {
		if ( bv->b1_tool!=pos ) {
		    bv->b1_tool = pos;
		    GDrawRequestExpose(bv->tools,NULL,false);
		}
	    }
	    bv->pressed_tool = bv->pressed_display = bvt_none;
	}
	event->u.chr.state &= ~(1<<(7+event->u.mouse.button));
    }
    BVToolsSetCursor(bv,event->u.chr.state);
}

static int bvtools_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	BVToolsExpose(gw,bv,&event->u.expose.rect);
      break;
      case et_mousedown:
	BVToolsMouse(bv,event);
      break;
      case et_mousemove:
	BVToolsMouse(bv,event);
      break;
      case et_mouseup:
	BVToolsMouse(bv,event);
      break;
      case et_crossing:
	bv->pressed_display = bvt_none;
	BVToolsSetCursor(bv,event->u.mouse.state);
      break;
      case et_char: case et_charup:
	if ( bv->had_control != ((event->u.chr.state&ksm_control)?1:0) )
	    bv->pressed_display = bvt_none;
	BVChar(bv,event);
      break;
      case et_destroy:
      break;
      case et_close:
	GDrawSetVisible(gw,false);
      break;
    }
return( true );
}

GWindow BVMakeTools(BitmapView *bv) {
    GRect r;
    GWindowAttrs wattrs;
    unichar_t title[] = { 'T', 'o', 'o', 'l', 's', '\0' };
    GWindow tools;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_positioned|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.window_title = title;

    r.width = 53; r.height = 80;
    r.x = -r.width-6; r.y = bv->mbh+20;
    tools = GWidgetCreatePalette( bv->gw, &r, bvtools_e_h, bv, &wattrs );
    GWidgetRequestVisiblePalette(tools,true);
return( tools );
}

static void BVPopupInvoked(GWindow v, GMenuItem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(v);
    int pos;

    pos = mi->mid;
    if ( bv->had_control ) {
	if ( bv->cb1_tool!=pos ) {
	    bv->cb1_tool = pos;
	    GDrawRequestExpose(bv->tools,NULL,false);
	}
    } else {
	if ( bv->b1_tool!=pos ) {
	    bv->b1_tool = pos;
	    GDrawRequestExpose(bv->tools,NULL,false);
	}
    }
    BVToolsSetCursor(bv,bv->had_control?ksm_control:0);
}

void BVToolsPopup(BitmapView *bv, GEvent *event) {
    GMenuItem mi[21];
    int i, j;

    memset(mi,'\0',sizeof(mi));
    for ( i=0;i<6; ++i ) {
	mi[i].ti.text = bvpopups[i];
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = i;
	mi[i].invoke = BVPopupInvoked;
    }

    mi[i].ti.text = (unichar_t *) "Rectangle"; mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_rect;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) "Filled Rectangle"; mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_filledrect;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) "Elipse"; mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_elipse;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) "Filled Elipse"; mi[i].ti.text_is_1byte = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_filledelipse;
    mi[i++].invoke = BVPopupInvoked;

    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i++].ti.line = true;
    for ( j=0; j<6; ++j, ++i ) {
	mi[i].ti.text = BVFlipNames[j];
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = j;
	mi[i].invoke = BVMenuRotateInvoked;
    }
    if ( bv->fv->sf->onlybitmaps ) {
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i++].ti.line = true;
	mi[i].ti.text = (unichar_t *) "Set Width"; mi[i].ti.text_is_1byte = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = bvt_setwidth;
	mi[i].invoke = BVPopupInvoked;
    }
    bv->had_control = (event->u.mouse.state&ksm_control)?1:0;
    GMenuCreatePopupMenu(bv->v,event, mi);
}
