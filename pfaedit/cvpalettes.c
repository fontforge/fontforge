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
#include "pfaeditui.h"
#include <gkeysym.h>
#include <math.h>
#include "splinefont.h"
#include "ustring.h"

static int cvvisible[2] = { 1, 1}, bvvisible[2]= { 1,1 };
static GWindow cvlayers, cvtools, bvlayers, bvtools;
static GPoint cvtoolsoff = { -9999 }, cvlayersoff = { -9999 }, bvlayersoff = { -9999 }, bvtoolsoff = { -9999 };
static int palettesmoved=0;

static unichar_t helv[] = { 'h', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a',',','c','a','l','i','b','a','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
static GFont *font;

static GWindow CreatePalette(GWindow w, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs) {
    GWindow gw;
    GPoint pt, base;
    GRect newpos;
    GWindow root;
    GRect ownerpos, screensize;

    pt.x = pos->x; pt.y = pos->y;
    root = GDrawGetRoot(NULL);
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
    if ( pt.x+pos->width>screensize.width )
	pt.x = screensize.width-pos->width;
    if ( pt.y+pos->height>screensize.height )
	pt.y = screensize.height-pos->height;

    newpos.x = pt.x; newpos.y = pt.y; newpos.width = pos->width; newpos.height = pos->height;
    wattrs->mask |= wam_positioned;
    wattrs->positioned = true;
    gw = GDrawCreateTopWindow(NULL,&newpos,eh,user_data,wattrs);
return( gw );
}

static void SaveOffsets(GWindow main, GWindow palette, GPoint *off) {
    if ( GDrawIsVisible(palette)) {
	GRect mr, pr;
	GDrawGetSize(main,&mr);
	GDrawGetSize(palette,&pr);
	off->x = pr.x-mr.x;
	off->y = pr.y-mr.y;
#if 0
 printf( "%s is offset (%d,%d)\n", palette==cvtools?"CVTools":
     palette==cvlayers?"CVLayers":palette==bvtools?"BVTools":"BVLayers", off->x, off->y );
#endif
    }
}

static void RestoreOffsets(GWindow main, GWindow palette, GPoint *off) {
    GPoint pt;
    GWindow root;
    GRect screensize, pos;

    pt = *off;
    root = GDrawGetRoot(NULL);
    GDrawGetSize(root,&screensize);
    GDrawGetSize(palette,&pos);
    GDrawTranslateCoordinates(main,root,&pt);
    if ( pt.x<0 ) pt.x=0;
    if ( pt.y<0 ) pt.y=0;
    if ( pt.x+pos.width>screensize.width )
	pt.x = screensize.width-pos.width;
    if ( pt.y+pos.height>screensize.height )
	pt.y = screensize.height-pos.height;
    palettesmoved = true;
    GDrawTrueMove(palette,pt.x,pt.y);
    GDrawRaise(palette);
}

/* Note: If you change this ordering, change enum cvtools */
static int popupsres[] = { _STR_Pointer, _STR_PopMag,
				    _STR_AddCurvePoint, _STR_AddCornerPoint,
			            _STR_AddTangentPoint, _STR_AddPenPoint,
			            _STR_PopKnife, _STR_PopRuler,
			            _STR_PopScale, _STR_PopFlip,
			            _STR_PopRotate, _STR_PopSkew,
			            _STR_PopRectElipse, _STR_PopPolyStar,
			            _STR_PopRectElipse, _STR_PopPolyStar};
static int rectelipse=0, polystar=0, regular_star=1;
static real rr_radius=0;
static int ps_pointcnt=6;
static real star_percent=1.7320508;	/* Regular 6 pointed star */

real CVRoundRectRadius(void) {
return( rr_radius );
}

int CVPolyStarPoints(void) {
return( ps_pointcnt );
}

real CVStarRatio(void) {
    if ( regular_star )
return( sin(3.1415926535897932/ps_pointcnt)*tan(2*3.1415926535897932/ps_pointcnt)+cos(3.1415926535897932/ps_pointcnt) );
	
return( star_percent );
}
    
struct ask_info {
    GWindow gw;
    int done;
    int ret;
    real *val;
    GGadget *rb1;
    GGadget *reg;
    int isint;
    int lab;
};
#define CID_ValText		1001
#define CID_PointPercent	1002

static int TA_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct ask_info *d = GDrawGetUserData(GGadgetGetWindow(g));
	real val, val2=0;
	int err=0;
	if ( d->isint ) {
	    val = GetIntR(d->gw,CID_ValText,d->lab,&err);
	    if ( !(regular_star = GGadgetIsChecked(d->reg)))
		val2 = GetRealR(d->gw,CID_PointPercent,_STR_SizeOfPoints,&err);
	} else
	    val = GetRealR(d->gw,CID_ValText,d->lab,&err);
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
return( event->type!=et_char );
}

static int Ask(int rb1, int rb2, int rb, int lab, real *val, int isint ) {
    struct ask_info d;
    char buffer[20], buf[20];
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
	wattrs.window_title = GStringGetResource(_STR_ShapeType,NULL);
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,190);
	pos.height = GDrawPointsToPixels(NULL,105+off);
	d.gw = GDrawCreateTopWindow(NULL,&pos,toolask_e_h,&d,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) rb1;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible | (rb==0?gg_cb_on:0);
	gcd[0].creator = GRadioCreate;

	label[1].text = (unichar_t *) rb2;
	label[1].text_in_resource = true;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = isint?65:75; gcd[1].gd.pos.y = 5; 
	gcd[1].gd.flags = gg_enabled|gg_visible | (rb==1?gg_cb_on:0);
	gcd[1].creator = GRadioCreate;

	label[2].text = (unichar_t *) lab;
	label[2].text_in_resource = true;
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
	gcd[4].gd.pos.width = -1; gcd[4].gd.pos.height = 0;
	gcd[4].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[4].text = (unichar_t *) _STR_OK;
	label[4].text_in_resource = true;
	gcd[4].gd.mnemonic = 'O';
	gcd[4].gd.label = &label[4];
	gcd[4].gd.handle_controlevent = TA_OK;
	gcd[4].creator = GButtonCreate;

	gcd[5].gd.pos.x = 170-GIntGetResource(_NUM_Buttonsize)-20; gcd[5].gd.pos.y = 70+3+off;
	gcd[5].gd.pos.width = -1; gcd[5].gd.pos.height = 0;
	gcd[5].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[5].text = (unichar_t *) _STR_Cancel;
	label[5].text_in_resource = true;
	gcd[5].gd.label = &label[5];
	gcd[5].gd.mnemonic = 'C';
	gcd[5].gd.handle_controlevent = TA_Cancel;
	gcd[5].creator = GButtonCreate;

	if ( isint ) {
	    label[6].text = (unichar_t *) _STR_Regular;
	    label[6].text_in_resource = true;
	    gcd[6].gd.label = &label[6];
	    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = 70; 
	    gcd[6].gd.flags = gg_enabled|gg_visible | (rb==0?gg_cb_on:0);
	    gcd[6].creator = GRadioCreate;

	    label[7].text = (unichar_t *) _STR_Points;
	    label[7].text_in_resource = true;
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
    rectelipse = Ask(_STR_Rectangle,_STR_Elipse,rectelipse,
	    _STR_RRRad,&rr_radius,false);
    GDrawRequestExpose(cvtools,NULL,false);
}

static void CVPolyStar(CharView *cv) {
    real temp = ps_pointcnt;
    polystar = Ask(_STR_Polygon,_STR_Star,polystar,
	    _STR_NumPSVert,&temp,true);
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
    static GImage *smalls[] = { &GIcon_smallpointer, &GIcon_smallmag,
				    &GIcon_smallcurve, &GIcon_smallcorner,
				    &GIcon_smalltangent, &GIcon_smallpen,
			            &GIcon_smallknife, &GIcon_smallruler,
			            &GIcon_smallscale, &GIcon_smallflip,
			            &GIcon_smallrotate, &GIcon_smallskew,
			            &GIcon_smallrect, &GIcon_smallpoly,
			            &GIcon_smallelipse, &GIcon_smallstar };
    static const unichar_t _Mouse[][9] = {
	    { 'M', 's', 'e', '1',  '\0' },
	    { '^', 'M', 's', 'e', '1',  '\0' },
	    { 'M', 's', 'e', '2',  '\0' },
	    { '^', 'M', 's', 'e', '2',  '\0' }};
    int i,j,norm, mi;
    int tool = cv->cntrldown?cv->cb1_tool:cv->b1_tool;
    int dither = GDrawSetDither(NULL,false);
    GRect temp;

    GDrawPushClip(pixmap,r,&old);
    for ( i=0; i<sizeof(buttons)/sizeof(buttons[0])-1; ++i ) for ( j=0; j<2; ++j ) {
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
    GDrawSetFont(pixmap,font);
    temp.x = 52-16; temp.y = i*27; temp.width = 16; temp.height = 4*12;
    GDrawFillRect(pixmap,&temp,GDrawGetDefaultBackground(NULL));
    for ( j=0; j<4; ++j ) {
	GDrawDrawText(pixmap,2,i*27+j*12+10,_Mouse[j],-1,NULL,0x000000);
	GDrawDrawImage(pixmap,smalls[(&cv->b1_tool)[j]],NULL,52-16,i*27+j*12);
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
    else if ( keysym == GK_Super_L || keysym == GK_Super_L )
	bit = ksm_super;
    else if ( keysym == GK_Hyper_L || keysym == GK_Hyper_L )
	bit = ksm_hyper;
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
    else if ( (state&ksm_control) && (state&(ksm_button2|ksm_super)) )
	shouldshow = cv->cb2_tool;
    else if ( (state&(ksm_button2|ksm_super)) )
	shouldshow = cv->b2_tool;
    else if ( (state&ksm_control) )
	shouldshow = cv->cb1_tool;
    else
	shouldshow = cv->b1_tool;
    if ( shouldshow==cvt_magnify && (state&ksm_meta))
	shouldshow = cvt_minify;
    if ( shouldshow!=cv->showing_tool ) {
	GDrawSetCursor(cv->v,tools[shouldshow]);
	GDrawSetCursor(cvtools,tools[shouldshow]);
	cv->showing_tool = shouldshow;
    }

    cntrl = (state&ksm_control)?1:0;
    if ( cntrl != cv->cntrldown ) {
	cv->cntrldown = cntrl;
	GDrawRequestExpose(cvtools,NULL,false);
    }
}

static void ToolsMouse(CharView *cv, GEvent *event) {
    int i = (event->u.mouse.y/27), j = (event->u.mouse.x/27), mi=i;
    int pos;

    if ( i==(cvt_rect)/2 && ((j==0 && rectelipse) || (j==1 && polystar)) )
	++mi;
    pos = mi*2 + j;
    GGadgetEndPopup();
    /* we have two fewer buttons than commands as two bottons each control two commands */
    if ( pos<0 || pos>=cvt_max-2 )
	pos = cvt_none;
    if ( event->type == et_mousedown ) {
	cv->pressed_tool = cv->pressed_display = pos;
	cv->had_control = (event->u.mouse.state&ksm_control)?1:0;
	event->u.chr.state |= (1<<(7+event->u.mouse.button));
    } else if ( event->type == et_mousemove ) {
	if ( cv->pressed_tool==cvt_none && pos!=cvt_none )
	    /* Not pressed */
	    GGadgetPreparePopupR(cvtools,popupsres[pos]);
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
		}
	    } else {
		if ( cv->b1_tool!=pos ) {
		    cv->b1_tool = pos;
		}
	    }
	    cv->pressed_tool = cv->pressed_display = cvt_none;
	}
	GDrawRequestExpose(cvtools,NULL,false);
	event->u.chr.state &= ~(1<<(7+event->u.mouse.button));
    }
    CVToolsSetCursor(cv,event->u.chr.state);
}

static void PostCharToWindow(GWindow to, GEvent *e) {
    GPoint p;

    p.x = e->u.chr.x; p.y = e->u.chr.y;
    GDrawTranslateCoordinates(e->w,to,&p);
    e->u.chr.x = p.x; e->u.chr.y = p.y;
    e->w = to;
    GDrawPostEvent(e);
}

static int cvtools_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv==NULL )
return( true );

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
	PostCharToWindow(cv->gw,event);
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
    FontRequest rq;

    if ( cvtools!=NULL )
return( cvtools );

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.window_title = GStringGetResource(_STR_Tools,NULL);

    r.width = 53; r.height = 187+4*12+2;
    if ( cvtoolsoff.x==-9999 ) {
	cvtoolsoff.x = -r.width-6; cvtoolsoff.y = cv->mbh+20;
    }
    r.x = cvtoolsoff.x; r.y = cvtoolsoff.y;
    cvtools = CreatePalette( cv->gw, &r, cvtools_e_h, cv, &wattrs );

    memset(&rq,0,sizeof(rq));
    rq.family_name = helv;
    rq.point_size = -10;
    rq.weight = 400;
    font = GDrawInstanciateFont(NULL,&rq);

    GDrawSetVisible(cvtools,true);
return( cvtools );
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
	    GDrawRequestExpose(cvtools,NULL,false);
	}
    } else {
	if ( cv->b1_tool!=pos ) {
	    cv->b1_tool = pos;
	    GDrawRequestExpose(cvtools,NULL,false);
	}
    }
    CVToolsSetCursor(cv,cv->had_control?ksm_control:0);
}

void CVToolsPopup(CharView *cv, GEvent *event) {
    GMenuItem mi[16];
    int i;

    memset(mi,'\0',sizeof(mi));
    for ( i=0;i<14; ++i ) {
	mi[i].ti.text = (unichar_t *) popupsres[i];
	mi[i].ti.text_in_resource = true;
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
#define CID_VHHints	1005
#define CID_VVHints	1006
#define CID_VDHints	1007
#define CID_EFore	1008
#define CID_EBack	1009
#define CID_EGrid	1010
#define CID_VHMetrics	1011
#define CID_VVMetrics	1012
#define CID_VVMetricsLab	1013

static void CVLayersSet(CharView *cv) {
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VFore),cv->showfore);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VBack),cv->showback);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VGrid),cv->showgrids);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VVHints),cv->showvhints);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VHHints),cv->showhhints);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VDHints),cv->showdhints);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VHMetrics),cv->showhmetrics);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,CID_VVMetrics),cv->showvmetrics);
    GGadgetSetChecked(GWidgetGetControl(cvlayers,
		cv->drawmode==dm_fore?CID_EFore:
		cv->drawmode==dm_back?CID_EBack:CID_EGrid ),true);
    GGadgetSetEnabled(GWidgetGetControl(cvlayers,CID_VVMetrics),
	    cv->sc->parent->hasvmetrics);
    GGadgetSetEnabled(GWidgetGetControl(cvlayers,CID_VVMetricsLab),
	    cv->sc->parent->hasvmetrics);
}

static int cvlayers_e_h(GWindow gw, GEvent *event) {
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if ( cv==NULL )
return( true );

    switch ( event->type ) {
      case et_close:
	GDrawSetVisible(gw,false);
      break;
      case et_char:
	PostCharToWindow(cv->gw,event);
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
	      case CID_VHHints:
		CVShows.showhhints = cv->showhhints =
		    CVShows.showmdy = cv->showmdy = 
			GGadgetIsChecked(event->u.control.g);
		cv->back_img_out_of_date = true;	/* only this cv */
	      break;
	      case CID_VVHints:
		CVShows.showvhints = cv->showvhints =
		    CVShows.showmdx = cv->showmdx = 
			GGadgetIsChecked(event->u.control.g);
		cv->back_img_out_of_date = true;	/* only this cv */
	      break;
	      case CID_VDHints:
		CVShows.showdhints = cv->showdhints =
			GGadgetIsChecked(event->u.control.g);
		cv->back_img_out_of_date = true;	/* only this cv */
	      break;
	      case CID_VHMetrics:
		CVShows.showhmetrics = cv->showhmetrics = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_VVMetrics:
		CVShows.showvmetrics = cv->showvmetrics = GGadgetIsChecked(event->u.control.g);
	      break;
	      case CID_EFore:
		cv->drawmode = dm_fore;
		cv->lastselpt = NULL;
	      break;
	      case CID_EBack:
		cv->drawmode = dm_back;
		cv->lastselpt = NULL;
	      break;
	      case CID_EGrid:
		cv->drawmode = dm_grid;
		cv->lastselpt = NULL;
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
    GGadgetCreateData gcd[21];
    GTextInfo label[21];
    static GBox radio_box = { bt_none, bs_rect, 0, 0, 0, 0, 0,0,0,0, COLOR_DEFAULT,COLOR_DEFAULT };
    GFont *font;
    FontRequest rq;
    int i, base;

    if ( cvlayers!=NULL )
return( cvlayers );
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.window_title = GStringGetResource(_STR_Layers,NULL);

    r.width = 104; r.height = 162;
    if ( cvlayersoff.x==-9999 ) {
	cvlayersoff.x = -r.width-6;
	cvlayersoff.y = cv->mbh+187+50+45/*25*/;	/* 45 is right if there's decor, 25 when none. twm gives none, kde gives decor */
    }
    r.x = cvlayersoff.x; r.y = cvlayersoff.y;
    cvlayers = CreatePalette( cv->gw, &r, cvlayers_e_h, cv, &wattrs );

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    memset(&rq,'\0',sizeof(rq));
    rq.family_name = helv;
    rq.point_size = -12;
    rq.weight = 400;
    font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(cvlayers),&rq);
    for ( i=0; i<sizeof(label)/sizeof(label[0]); ++i )
	label[i].font = font;

    label[0].text = (unichar_t *) _STR_V;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 7; gcd[0].gd.pos.y = 5; 
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[0].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[0].creator = GLabelCreate;

    label[1].text = (unichar_t *) _STR_E;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 30; gcd[1].gd.pos.y = 5; 
    gcd[1].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[1].gd.popup_msg = GStringGetResource(_STR_IsEdit,NULL);
    gcd[1].creator = GLabelCreate;

    label[2].text = (unichar_t *) _STR_Layer;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 47; gcd[2].gd.pos.y = 5; 
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[2].gd.popup_msg = GStringGetResource(_STR_IsEdit,NULL);
    gcd[2].creator = GLabelCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 21; 
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[3].gd.cid = CID_VFore;
    gcd[3].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[3].gd.box = &radio_box;
    gcd[3].creator = GCheckBoxCreate;

    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = 38; 
    gcd[4].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[4].gd.cid = CID_VBack;
    gcd[4].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[4].gd.box = &radio_box;
    gcd[4].creator = GCheckBoxCreate;

    gcd[5].gd.pos.x = 5; gcd[5].gd.pos.y = 55; 
    gcd[5].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[5].gd.cid = CID_VGrid;
    gcd[5].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[5].gd.box = &radio_box;
    gcd[5].creator = GCheckBoxCreate;

    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = 72; 
    gcd[6].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[6].gd.cid = CID_VHHints;
    gcd[6].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[6].gd.box = &radio_box;
    gcd[6].creator = GCheckBoxCreate;

    gcd[7].gd.pos.x = 5; gcd[7].gd.pos.y = 89; 
    gcd[7].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[7].gd.cid = CID_VVHints;
    gcd[7].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[7].gd.box = &radio_box;
    gcd[7].creator = GCheckBoxCreate;

    gcd[8].gd.pos.x = 5; gcd[8].gd.pos.y = 106; 
    gcd[8].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[8].gd.cid = CID_VDHints;
    gcd[8].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[8].gd.box = &radio_box;
    gcd[8].creator = GCheckBoxCreate;

    gcd[9].gd.pos.x = 5; gcd[9].gd.pos.y = 123; 
    gcd[9].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[9].gd.cid = CID_VHMetrics;
    gcd[9].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[9].gd.box = &radio_box;
    gcd[9].creator = GCheckBoxCreate;

    gcd[10].gd.pos.x = 5; gcd[10].gd.pos.y = 140; 
    gcd[10].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[10].gd.cid = CID_VVMetrics;
    gcd[10].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[10].gd.box = &radio_box;
    gcd[10].creator = GCheckBoxCreate;
    base = 11;


    label[base].text = (unichar_t *) _STR_Fore;
    label[base].text_in_resource = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 27; gcd[base].gd.pos.y = 21; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base].gd.cid = CID_EFore;
    gcd[base].gd.popup_msg = GStringGetResource(_STR_IsEdit,NULL);
    gcd[base].gd.box = &radio_box;
    gcd[base].creator = GRadioCreate;

    label[base+1].text = (unichar_t *) _STR_Back;
    label[base+1].text_in_resource = true;
    gcd[base+1].gd.label = &label[base+1];
    gcd[base+1].gd.pos.x = 27; gcd[base+1].gd.pos.y = 38; 
    gcd[base+1].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base+1].gd.cid = CID_EBack;
    gcd[base+1].gd.popup_msg = GStringGetResource(_STR_IsEdit,NULL);
    gcd[base+1].gd.box = &radio_box;
    gcd[base+1].creator = GRadioCreate;

    label[base+2].text = (unichar_t *) _STR_Grid;
    label[base+2].text_in_resource = true;
    gcd[base+2].gd.label = &label[base+2];
    gcd[base+2].gd.pos.x = 27; gcd[base+2].gd.pos.y = 55; 
    gcd[base+2].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base+2].gd.cid = CID_EGrid;
    gcd[base+2].gd.popup_msg = GStringGetResource(_STR_IsEdit,NULL);
    gcd[base+2].gd.box = &radio_box;
    gcd[base+2].creator = GRadioCreate;

    gcd[base+cv->drawmode].gd.flags |= gg_cb_on;
    base += 3;

    label[base].text = (unichar_t *) _STR_HHints;
    label[base].text_in_resource = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 72; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base++].creator = GLabelCreate;

    label[base].text = (unichar_t *) _STR_VHints;
    label[base].text_in_resource = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 89; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base++].creator = GLabelCreate;

    label[base].text = (unichar_t *) _STR_DHints;
    label[base].text_in_resource = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 106; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base++].creator = GLabelCreate;

    label[base].text = (unichar_t *) _STR_HMetrics;
    label[base].text_in_resource = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 123; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base++].creator = GLabelCreate;

    label[base].text = (unichar_t *) _STR_VMetrics;
    label[base].text_in_resource = true;
    gcd[base].gd.label = &label[base];
    gcd[base].gd.pos.x = 47; gcd[base].gd.pos.y = 140; 
    gcd[base].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[base].gd.cid = CID_VVMetricsLab;
    gcd[base++].creator = GLabelCreate;

    gcd[base].gd.pos.x = 1; gcd[base].gd.pos.y = 1;
    gcd[base].gd.pos.width = r.width-2; gcd[base].gd.pos.height = r.height-2;
    gcd[base].gd.flags = gg_enabled | gg_visible|gg_pos_in_pixels;
    gcd[base++].creator = GGroupCreate;

    if ( cv->showfore ) gcd[3].gd.flags |= gg_cb_on;
    if ( cv->showback ) gcd[4].gd.flags |= gg_cb_on;
    if ( cv->showgrids ) gcd[5].gd.flags |= gg_cb_on;
    if ( cv->showhhints ) gcd[6].gd.flags |= gg_cb_on;
    if ( cv->showvhints ) gcd[7].gd.flags |= gg_cb_on;
    if ( cv->showdhints ) gcd[8].gd.flags |= gg_cb_on;
    if ( cv->showhmetrics ) gcd[9].gd.flags |= gg_cb_on;
    if ( cv->showvmetrics ) gcd[10].gd.flags |= gg_cb_on;
    if ( !cv->sc->parent->hasvmetrics ) {
	gcd[10].gd.flags &= ~gg_enabled;
	gcd[base-2].gd.flags &= ~gg_enabled;
    }

    GGadgetsCreate(cvlayers,gcd);
    GDrawSetVisible(cvlayers,true);
return( cvlayers );
}

static void CVPaletteCheck(CharView *cv) {
    if ( cvtools==NULL ) {
	CVMakeTools(cv);
	CVMakeLayers(cv);
    }
}

int CVPaletteIsVisible(CharView *cv,int which) {
    CVPaletteCheck(cv);
    if ( which==1 )
return( cvtools!=NULL && GDrawIsVisible(cvtools) );

return( cvlayers!=NULL && GDrawIsVisible(cvlayers) );
}

void CVPaletteSetVisible(CharView *cv,int which,int visible) {
    CVPaletteCheck(cv);
    if ( which==1 && cvtools!=NULL)
	GDrawSetVisible(cvtools,visible );
    else if ( which==0 && cvlayers!=NULL )
	GDrawSetVisible(cvlayers,visible );
    cvvisible[which] = visible;
}

void CVPaletteActivate(CharView *cv) {
    CharView *old;

    CVPaletteCheck(cv);
    if ( (old = GDrawGetUserData(cvtools))!=cv ) {
	if ( old!=NULL ) {
	    SaveOffsets(old->gw,cvtools,&cvtoolsoff);
	    SaveOffsets(old->gw,cvlayers,&cvlayersoff);
	}
	GDrawSetUserData(cvtools,cv);
	GDrawSetUserData(cvlayers,cv);
	if ( cvvisible[0])
	    RestoreOffsets(cv->gw,cvlayers,&cvlayersoff);
	if ( cvvisible[1])
	    RestoreOffsets(cv->gw,cvtools,&cvtoolsoff);
	GDrawSetVisible(cvtools,cvvisible[1]);
	GDrawSetVisible(cvlayers,cvvisible[0]);
	if ( cvvisible[1]) {
	    cv->showing_tool = cvt_none;
	    CVToolsSetCursor(cv,0);
	    GDrawRequestExpose(cvtools,NULL,false);
	}
	if ( cvvisible[0])
	    CVLayersSet(cv);
    }
    if ( bvtools!=NULL ) {
	BitmapView *bv = GDrawGetUserData(bvtools);
	if ( bv!=NULL ) {
	    SaveOffsets(bv->gw,bvtools,&bvtoolsoff);
	    SaveOffsets(bv->gw,bvlayers,&bvlayersoff);
	    GDrawSetUserData(bvtools,NULL);
	    GDrawSetUserData(bvlayers,NULL);
	}
	GDrawSetVisible(bvtools,false);
	GDrawSetVisible(bvlayers,false);
    }
}

void CVPalettesHideIfMine(CharView *cv) {
    if ( cvtools==NULL )
return;
    if ( GDrawGetUserData(cvtools)==cv ) {
	SaveOffsets(cv->gw,cvtools,&cvtoolsoff);
	SaveOffsets(cv->gw,cvlayers,&cvlayersoff);
	GDrawSetVisible(cvtools,false);
	GDrawSetVisible(cvlayers,false);
	GDrawSetUserData(cvtools,NULL);
	GDrawSetUserData(cvlayers,NULL);
    }
}

/* ************************************************************************** */
/* **************************** Bitmap Palettes ***************************** */
/* ************************************************************************** */

static void BVLayersSet(BitmapView *bv) {
    GGadgetSetChecked(GWidgetGetControl(bvlayers,CID_VFore),bv->showfore);
    GGadgetSetChecked(GWidgetGetControl(bvlayers,CID_VBack),bv->showoutline);
    GGadgetSetChecked(GWidgetGetControl(bvlayers,CID_VGrid),bv->showgrid);
}

static int bvlayers_e_h(GWindow gw, GEvent *event) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(gw);

    if ( bv==NULL )
return( true );

    switch ( event->type ) {
      case et_close:
	GDrawSetVisible(gw,false);
      break;
      case et_char:
	PostCharToWindow(bv->gw,event);
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
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    static GBox radio_box = { bt_none, bs_rect, 0, 0, 0, 0, 0,0,0,0, COLOR_DEFAULT,COLOR_DEFAULT };
    static unichar_t helv[] = { 'h', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a',',','c','a','l','i','b','a','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    GFont *font;
    FontRequest rq;
    int i;

    if ( bvlayers!=NULL )
return(bvlayers);
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.window_title = GStringGetResource(_STR_Layers,NULL);

    r.width = 73; r.height = 73;
    r.x = -r.width-6; r.y = bv->mbh+81+45/*25*/;	/* 45 is right if there's decor, is in kde, not in twm. Sigh */
    bvlayers = CreatePalette( bv->gw, &r, bvlayers_e_h, bv, &wattrs );

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    memset(&rq,'\0',sizeof(rq));
    rq.family_name = helv;
    rq.point_size = -12;
    rq.weight = 400;
    font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(bvlayers),&rq);
    for ( i=0; i<sizeof(label)/sizeof(label[0]); ++i )
	label[i].font = font;

    label[0].text = (unichar_t *) _STR_V;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 7; gcd[0].gd.pos.y = 5; 
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[0].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 1; gcd[1].gd.pos.y = 1;
    gcd[1].gd.pos.width = r.width-2; gcd[1].gd.pos.height = r.height-1;
    gcd[1].gd.flags = gg_enabled | gg_visible|gg_pos_in_pixels;
    gcd[1].creator = GGroupCreate;

    label[2].text = (unichar_t *) "Layer";
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 23; gcd[2].gd.pos.y = 5; 
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
    gcd[2].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[2].creator = GLabelCreate;

    gcd[3].gd.pos.x = 5; gcd[3].gd.pos.y = 21; 
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[3].gd.cid = CID_VFore;
    gcd[3].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[3].gd.box = &radio_box;
    gcd[3].creator = GCheckBoxCreate;
    label[3].text = (unichar_t *) _STR_Bitmap;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];

    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = 37; 
    gcd[4].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[4].gd.cid = CID_VBack;
    gcd[4].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[4].gd.box = &radio_box;
    gcd[4].creator = GCheckBoxCreate;
    label[4].text = (unichar_t *) _STR_Outline;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];

    gcd[5].gd.pos.x = 5; gcd[5].gd.pos.y = 53; 
    gcd[5].gd.flags = gg_enabled|gg_visible|gg_dontcopybox|gg_pos_in_pixels;
    gcd[5].gd.cid = CID_VGrid;
    gcd[5].gd.popup_msg = GStringGetResource(_STR_IsVis,NULL);
    gcd[5].gd.box = &radio_box;
    gcd[5].creator = GCheckBoxCreate;
    label[5].text = (unichar_t *) _STR_Grid;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];

    if ( bv->showfore ) gcd[3].gd.flags |= gg_cb_on;
    if ( bv->showoutline ) gcd[4].gd.flags |= gg_cb_on;
    if ( bv->showgrid ) gcd[5].gd.flags |= gg_cb_on;

    GGadgetsCreate(bvlayers,gcd);
    GDrawSetVisible(bvlayers,true);
return( bvlayers );
}

static int bvpopups[] = { _STR_Pointer, _STR_PopMag,
				    _STR_PopPencil, _STR_PopLine,
			            _STR_PopShift, _STR_PopHand };

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
    else if ( (state&ksm_control) && (state&(ksm_button2|ksm_super)) )
	shouldshow = bv->cb2_tool;
    else if ( (state&(ksm_button2|ksm_super)) )
	shouldshow = bv->b2_tool;
    else if ( (state&ksm_control) )
	shouldshow = bv->cb1_tool;
    else
	shouldshow = bv->b1_tool;
    if ( shouldshow==bvt_magnify && (state&ksm_meta))
	shouldshow = bvt_minify;
    if ( shouldshow!=bv->showing_tool ) {
	GDrawSetCursor(bv->v,tools[shouldshow]);
	GDrawSetCursor(bvtools,tools[shouldshow]);
	bv->showing_tool = shouldshow;
    }

    cntrl = (state&ksm_control)?1:0;
    if ( cntrl != bv->cntrldown ) {
	bv->cntrldown = cntrl;
	GDrawRequestExpose(bvtools,NULL,false);
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
	    GGadgetPreparePopupR(bvtools,bvpopups[pos]);
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
		    GDrawRequestExpose(bvtools,NULL,false);
		}
	    } else {
		if ( bv->b1_tool!=pos ) {
		    bv->b1_tool = pos;
		    GDrawRequestExpose(bvtools,NULL,false);
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

    if ( bv==NULL )
return( true );

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
	PostCharToWindow(bv->gw,event);
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

    if ( bvtools!=NULL )
return( bvtools );
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_positioned|wam_isdlg;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.positioned = true;
    wattrs.is_dlg = true;
    wattrs.window_title = GStringGetResource(_STR_Tools,NULL);

    r.width = 53; r.height = 80;
    r.x = -r.width-6; r.y = bv->mbh+20;
    bvtools = CreatePalette( bv->gw, &r, bvtools_e_h, bv, &wattrs );
    GDrawSetVisible(bvtools,true);
return( bvtools );
}

static void BVPopupInvoked(GWindow v, GMenuItem *mi,GEvent *e) {
    BitmapView *bv = (BitmapView *) GDrawGetUserData(v);
    int pos;

    pos = mi->mid;
    if ( bv->had_control ) {
	if ( bv->cb1_tool!=pos ) {
	    bv->cb1_tool = pos;
	    GDrawRequestExpose(bvtools,NULL,false);
	}
    } else {
	if ( bv->b1_tool!=pos ) {
	    bv->b1_tool = pos;
	    GDrawRequestExpose(bvtools,NULL,false);
	}
    }
    BVToolsSetCursor(bv,bv->had_control?ksm_control:0);
}

void BVToolsPopup(BitmapView *bv, GEvent *event) {
    GMenuItem mi[21];
    int i, j;

    memset(mi,'\0',sizeof(mi));
    for ( i=0;i<6; ++i ) {
	mi[i].ti.text = (unichar_t *) bvpopups[i];
	mi[i].ti.text_in_resource = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = i;
	mi[i].invoke = BVPopupInvoked;
    }

    mi[i].ti.text = (unichar_t *) _STR_Rectangle; mi[i].ti.text_in_resource = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_rect;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) _STR_FilledRectangle; mi[i].ti.text_in_resource = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_filledrect;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) _STR_Elipse; mi[i].ti.text_in_resource = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_elipse;
    mi[i++].invoke = BVPopupInvoked;
    mi[i].ti.text = (unichar_t *) _STR_FilledElipse; mi[i].ti.text_in_resource = true;
    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i].mid = bvt_filledelipse;
    mi[i++].invoke = BVPopupInvoked;

    mi[i].ti.fg = COLOR_DEFAULT;
    mi[i].ti.bg = COLOR_DEFAULT;
    mi[i++].ti.line = true;
    for ( j=0; j<6; ++j, ++i ) {
	mi[i].ti.text = (unichar_t *) BVFlipNames[j];
	mi[i].ti.text_in_resource = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = j;
	mi[i].invoke = BVMenuRotateInvoked;
    }
    if ( bv->fv->sf->onlybitmaps ) {
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i++].ti.line = true;
	mi[i].ti.text = (unichar_t *) _STR_Setwidth; mi[i].ti.text_in_resource = true;
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = bvt_setwidth;
	mi[i].invoke = BVPopupInvoked;
    }
    bv->had_control = (event->u.mouse.state&ksm_control)?1:0;
    GMenuCreatePopupMenu(bv->v,event, mi);
}

static void BVPaletteCheck(BitmapView *bv) {
    if ( bvtools==NULL ) {
	BVMakeTools(bv);
	BVMakeLayers(bv);
    }
}

int BVPaletteIsVisible(BitmapView *bv,int which) {
    BVPaletteCheck(bv);
    if ( which==1 )
return( bvtools!=NULL && GDrawIsVisible(bvtools) );

return( bvlayers!=NULL && GDrawIsVisible(bvlayers) );
}

void BVPaletteSetVisible(BitmapView *bv,int which,int visible) {
    BVPaletteCheck(bv);
    if ( which==1 && bvtools!=NULL)
	GDrawSetVisible(bvtools,visible );
    else if ( which==0 && bvlayers!=NULL )
	GDrawSetVisible(bvlayers,visible );
    bvvisible[which] = visible;
}

void BVPaletteActivate(BitmapView *bv) {
    BitmapView *old;

    BVPaletteCheck(bv);
    if ( (old = GDrawGetUserData(bvtools))!=bv ) {
	if ( old!=NULL ) {
	    SaveOffsets(old->gw,bvtools,&bvtoolsoff);
	    SaveOffsets(old->gw,bvlayers,&bvlayersoff);
	}
	GDrawSetUserData(bvtools,bv);
	GDrawSetUserData(bvlayers,bv);
	if ( bvvisible[0])
	    RestoreOffsets(bv->gw,bvlayers,&bvlayersoff);
	if ( bvvisible[1])
	    RestoreOffsets(bv->gw,bvtools,&bvtoolsoff);
	GDrawSetVisible(bvtools,bvvisible[1]);
	GDrawSetVisible(bvlayers,bvvisible[0]);
	if ( bvvisible[1]) {
	    bv->showing_tool = bvt_none;
	    BVToolsSetCursor(bv,0);
	    GDrawRequestExpose(bvtools,NULL,false);
	}
	if ( bvvisible[0])
	    BVLayersSet(bv);
    }
    if ( cvtools!=NULL ) {
	CharView *cv = GDrawGetUserData(cvtools);
	if ( cv!=NULL ) {
	    SaveOffsets(cv->gw,cvtools,&cvtoolsoff);
	    SaveOffsets(cv->gw,cvlayers,&cvlayersoff);
	    GDrawSetUserData(cvtools,NULL);
	    GDrawSetUserData(cvlayers,NULL);
	}
	GDrawSetVisible(cvtools,false);
	GDrawSetVisible(cvlayers,false);
    }
}

void BVPalettesHideIfMine(BitmapView *bv) {
    if ( bvtools==NULL )
return;
    if ( GDrawGetUserData(bvtools)==bv ) {
	SaveOffsets(bv->gw,bvtools,&bvtoolsoff);
	SaveOffsets(bv->gw,bvlayers,&bvlayersoff);
	GDrawSetVisible(bvtools,false);
	GDrawSetVisible(bvlayers,false);
	GDrawSetUserData(bvtools,NULL);
	GDrawSetUserData(bvlayers,NULL);
    }
}

void CVPaletteDeactivate(void) {
    if ( cvtools!=NULL ) {
	CharView *cv = GDrawGetUserData(cvtools);
	if ( cv!=NULL ) {
	    SaveOffsets(cv->gw,cvtools,&cvtoolsoff);
	    SaveOffsets(cv->gw,cvlayers,&cvlayersoff);
	    GDrawSetUserData(cvtools,NULL);
	    GDrawSetUserData(cvlayers,NULL);
	}
	GDrawSetVisible(cvtools,false);
	GDrawSetVisible(cvlayers,false);
    }
    if ( bvtools!=NULL ) {
	BitmapView *bv = GDrawGetUserData(bvtools);
	if ( bv!=NULL ) {
	    SaveOffsets(bv->gw,bvtools,&bvtoolsoff);
	    SaveOffsets(bv->gw,bvlayers,&bvlayersoff);
	    GDrawSetUserData(bvtools,NULL);
	    GDrawSetUserData(bvlayers,NULL);
	}
	GDrawSetVisible(bvtools,false);
	GDrawSetVisible(bvlayers,false);
    }
}
