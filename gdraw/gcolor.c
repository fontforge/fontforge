/* Copyright (C) 2008-2012 by George Williams */
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

#include "ggadget.h"
#include "gwidget.h"

#include <math.h>

static GImage *ColorWheel(int width,int height) {
    struct hslrgb col;
    GImage *wheel;
    struct _GImage *base;
    int i,j;
    double x,y, hh, hw;
    uint32_t *row;

    memset(&col,0,sizeof(col));
    col.v = 1.0;

    if ( width<10 ) width = 10;
    if ( height<10 ) height = 10;
    hh = height/2.0; hw = width/2.0;

    wheel = GImageCreate(it_true,width,height);
    base = wheel->u.image;
    for ( i=0; i<height; ++i ) {
	row = (uint32_t *) (base->data + i*base->bytes_per_line);
	y = (i-hh)/(hh-1);
	for ( j=0; j<width; ++j ) {
	    x = (j-hw)/(hw-1);
	    col.s = sqrt(x*x + y*y);
	    if ( col.s>1.0 ) {
		*row++ = 0xffffff;
	    } else {
		col.h = atan2(y,x)*180/FF_PI;
		if ( col.h < 0 ) col.h += 360;
		gHSV2RGB(&col);
		*row++ = COLOR_CREATE( (int) rint(255*col.r), (int) rint(255*col.g), (int) rint(255*col.b));
	    }
	}
    }
return( wheel );	
}

#define GRAD_WIDTH	20
static GImage *Gradient(int height) {
    GImage *grad;
    struct _GImage *base;
    int i,j,c;
    uint32_t *row;

    if ( height<10 ) height = 10;

    grad = GImageCreate(it_true,GRAD_WIDTH,height);
    base = grad->u.image;
    for ( i=0; i<height; ++i ) {
	row = (uint32_t *) (base->data + i*base->bytes_per_line);
	c = 255*(height-1-i)/(height-1);
	for ( j=0; j<GRAD_WIDTH; ++j ) {
	    *row++ = COLOR_CREATE(c,c,c);
	}
    }
return( grad );
}

/* ************************************************************************** */
#define USEFUL_MAX	6

static uint8_t image_data[] = {
    0x00, 0x00,
    0x7f, 0xfe,
    0x7f, 0xfe,
    0x7f, 0xfe,
    0x7f, 0xfe,
    0x7f, 0xfe,
    0x7f, 0xfe,
    0x7f, 0xfe,
    0x7f, 0xfe,
    0x7f, 0xfe,
    0x7f, 0xfe,
    0x00, 0x00,
};
static GClut cluts[2*USEFUL_MAX] = {
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY },
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY },
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY },
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY },
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY },
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY },
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY },
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY },
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY },
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY },
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY },
    { 2, 0, COLOR_UNKNOWN, GCLUT_CLUT_EMPTY }
};
static struct _GImage bases[2*USEFUL_MAX] = {
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[0], COLOR_UNKNOWN },
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[1], COLOR_UNKNOWN },
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[2], COLOR_UNKNOWN },
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[3], COLOR_UNKNOWN },
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[4], COLOR_UNKNOWN },
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[5], COLOR_UNKNOWN },
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[6], COLOR_UNKNOWN },
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[7], COLOR_UNKNOWN },
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[8], COLOR_UNKNOWN },
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[9], COLOR_UNKNOWN },
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[10], COLOR_UNKNOWN },
    { it_mono, 0,16,12,2, (uint8_t *) image_data, &cluts[11], COLOR_UNKNOWN }
};
static GImage blanks[2*USEFUL_MAX] = {
    { 0, { &bases[0] }, NULL },
    { 0, { &bases[1] }, NULL },
    { 0, { &bases[2] }, NULL },
    { 0, { &bases[3] }, NULL },
    { 0, { &bases[4] }, NULL },
    { 0, { &bases[5] }, NULL },
    { 0, { &bases[6] }, NULL },
    { 0, { &bases[7] }, NULL },
    { 0, { &bases[8] }, NULL },
    { 0, { &bases[9] }, NULL },
    { 0, { &bases[10] }, NULL },
    { 0, { &bases[11] }, NULL }
};
/* ************************************************************************** */
struct hslrgba recent_cols[USEFUL_MAX+1];

struct gcol_data {
    GImage *wheel;
    GImage *grad;
    int wheel_width, wheel_height;
    GWindow gw;
    GWindow wheelw, gradw, colw;
    int done, pressed;
    struct hslrgba col, origcol;
    struct hslrgba user_cols[USEFUL_MAX+1];
};

#define CID_Red		1001
#define CID_Green	1002
#define CID_Blue	1003

#define CID_Hue		1011
#define CID_Saturation	1012
#define CID_Value	1013

#define CID_Alpha	1019

#define CID_Wheel	1021
#define CID_Grad	1022
#define CID_Color	1023

static int cids[] = { CID_Hue, CID_Saturation, CID_Value, CID_Red, CID_Green, CID_Blue, CID_Alpha, 0 };
static char *labnames[] = { N_("Hue:"), N_("Saturation:"), N_("Value:"), N_("Red:"), N_("Green:"), N_("Blue:"), N_("Opacity:") };

static int GCol_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gcol_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	double *offs[7] = { &d->col.h, &d->col.s, &d->col.v, &d->col.r, &d->col.g, &d->col.b, &d->col.alpha };
	int err = false, i;
	double val;

	for ( i=0; i<7; ++i ) {
	    val = GetReal8(d->gw,cids[i],_(labnames[i]),&err);
	    if ( err )
return( true );
	    if ( i==0 ) {
		val = fmod(val,360);
		if ( val<0 ) val += 360;
	    } else {
		if ( val<0 || val>1 ) {
		    gwwv_post_error(_("Value out of bounds"), _("Saturation and Value, and the three colors must be between 0 and 1"));
return( true );
		}
	    }
	    *offs[i] = val;
	}

	d->done = true;
    }
return( true );
}

static int GCol_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gcol_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->col.rgb = d->col.hsv = false;
	d->done = true;
    }
return( true );
}

static int GCol_TextChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct gcol_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	double *offs[7] = { &d->col.h, &d->col.s, &d->col.v, &d->col.r, &d->col.g, &d->col.b, &d->col.alpha };
	int i, err = false;
	int low, high;
	double val;
	char text[50];

	if ( GGadgetGetCid(g)==CID_Alpha ) {
	    low = 3; high=7;
	    /* Didn't actually change the rgb values, but parse them */
	    /*  This is in case we need to clear the error flag */
	    d->col.hsv = false;
	    d->col.rgb = true;
	} else if ( GGadgetGetCid(g)>=CID_Hue ) {
	    low = 0; high=3;
	    d->col.hsv = true;
	    d->col.rgb = false;
	} else {
	    low = 3; high=6;
	    d->col.hsv = false;
	    d->col.rgb = true;
	}
	for ( i=low; i<high; ++i ) {
	    val = GetCalmReal8(d->gw,cids[i],_(labnames[i]),&err);
	    if ( err )
	break;
	    if ( i==0 ) {
		val = fmod(val,360);
		if ( val<0 ) val += 360;
	    } else {
		if ( val<0 || val>1 ) {
		    err = true;
	break;
		}
	    }
	    *offs[i] = val;
	}
	if ( err ) {
	    d->col.hsv = d->col.rgb = false;
	} else if ( d->col.hsv ) {
	    gHSV2RGB((struct hslrgb *) &d->col);
	    for ( i=3; i<6; ++i ) {
		sprintf( text, "%.2f", *offs[i]);
		GGadgetSetTitle8(GWidgetGetControl(d->gw,cids[i]),text);
	    }
	} else {
	    gRGB2HSV((struct hslrgb *) &d->col);
	    sprintf( text, "%3.0f", *offs[0]);
	    GGadgetSetTitle8(GWidgetGetControl(d->gw,cids[0]),text);
	    for ( i=1; i<3; ++i ) {
		sprintf( text, "%.2f", *offs[i]);
		GGadgetSetTitle8(GWidgetGetControl(d->gw,cids[i]),text);
	    }
	}
	GDrawRequestExpose(d->wheelw,NULL,false);
	GDrawRequestExpose(d->gradw,NULL,false);
	GDrawRequestExpose(d->colw,NULL,false);
    }
return( true );
}

static void GCol_ShowTexts(struct gcol_data *d) {
    double *offs[7] = { &d->col.h, &d->col.s, &d->col.v, &d->col.r, &d->col.g, &d->col.b, &d->col.alpha };
    int i;
    char text[50];

    gHSV2RGB((struct hslrgb *) &d->col);
    sprintf( text, "%3.0f", *offs[0]);
    GGadgetSetTitle8(GWidgetGetControl(d->gw,cids[0]),text);
    for ( i=1; i<7; ++i ) {
	sprintf( text, "%.2f", *offs[i]);
	GGadgetSetTitle8(GWidgetGetControl(d->gw,cids[i]),text);
    }
}

static int wheel_e_h(GWindow gw, GEvent *event) {
    struct gcol_data *d = GDrawGetUserData(gw);
    GRect size;
    if ( event->type==et_expose ) {
	GRect circle;
	GDrawGetSize(d->wheelw,&size);
	if ( d->wheel==NULL || 
		GImageGetHeight(d->wheel)!=size.height ||
		GImageGetWidth(d->wheel)!=size.width ) {
	    if ( d->wheel!=NULL )
		GImageDestroy(d->wheel);
	    d->wheel = ColorWheel(size.width,size.height);
	}
	GDrawDrawImage(gw,d->wheel,NULL,0,0);
	if ( d->col.hsv ) {
	    double s = sin(d->col.h*FF_PI/180.);
	    double c = cos(d->col.h*FF_PI/180.);
	    int y = (int) rint(d->col.s*(size.height-1)*s/2.0) + size.height/2;
	    int x = (int) rint(d->col.s*(size.width-1)*c/2.0) + size.width/2;
	    circle.x = x-3; circle.y = y-3;
	    circle.width = circle.height = 7;
	    GDrawDrawElipse(gw,&circle,0x000000);
	}
    } else if ( event->type == et_mousedown ||
	    (event->type==et_mousemove && d->pressed) ||
	    event->type==et_mouseup ) {
	Color rgb;
	struct hslrgba temp;
	GDrawGetSize(d->wheelw,&size);
	if ( event->u.mouse.y>=0 && event->u.mouse.y<size.height &&
		event->u.mouse.x>=0 && event->u.mouse.x<size.width ) {
	    rgb = GImageGetPixelRGBA(d->wheel,event->u.mouse.x,event->u.mouse.y);
	    temp.r = ((rgb>>16)&0xff)/255.;
	    temp.g = ((rgb>>8)&0xff)/255.;
	    temp.b = ((rgb   )&0xff)/255.;
	    gRGB2HSV((struct hslrgb *) &temp);
	    d->col.h = temp.h; d->col.s = temp.s;
	    GCol_ShowTexts(d);
	    GDrawRequestExpose(d->colw,NULL,false);
	    GDrawRequestExpose(d->wheelw,NULL,false);
	}
	if ( event->type == et_mousedown )
	    d->pressed = true;
	else if ( event->type == et_mouseup )
	    d->pressed = false;
    } else if ( event->type==et_resize ) {
	GDrawRequestExpose(gw,NULL,false);
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static int grad_e_h(GWindow gw, GEvent *event) {
    struct gcol_data *d = GDrawGetUserData(gw);
    GRect size;
    if ( event->type==et_expose ) {
	GDrawGetSize(d->wheelw,&size);
	if ( d->grad==NULL || GImageGetHeight(d->grad)!=size.height ) {
	    if ( d->grad!=NULL )
		GImageDestroy(d->grad);
	    d->grad = Gradient(size.height);
	}
	GDrawDrawImage(gw,d->grad,NULL,0,0);
	if ( d->col.hsv ) {
	    int y = size.height-1-(int) rint(size.height*d->col.v);
	    int col = d->col.v>.5 ? 0x000000 : 0xffffff;
	    GDrawDrawLine(gw,0,y,GRAD_WIDTH,y,col);
	}
    } else if ( event->type == et_mousedown ||
	    (event->type==et_mousemove && d->pressed) ||
	    event->type==et_mouseup ) {
	GDrawGetSize(d->wheelw,&size);
	if ( event->u.mouse.y<0 )
	    event->u.mouse.y =0;
	else if ( event->u.mouse.y>=size.height )
	    event->u.mouse.y = size.height-1;
	d->col.v = (size.height-1-event->u.mouse.y) / (double) (size.height-1);
	if ( d->col.v<0 ) d->col.v = 0;
	if ( d->col.v>1 ) d->col.v = 1;
	GCol_ShowTexts(d);
	GDrawRequestExpose(d->colw,NULL,false);
	GDrawRequestExpose(d->gradw,NULL,false);
	if ( event->type == et_mousedown )
	    d->pressed = true;
	else if ( event->type == et_mouseup )
	    d->pressed = false;
    } else if ( event->type==et_resize ) {
	GDrawRequestExpose(gw,NULL,false);
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static void TranslucentRect(GWindow gw,GRect *size,int col,double alpha, int h) {
    if ( alpha==1.0 )
	GDrawFillRect(gw,size,col);
    else {
	int r = COLOR_RED(col), g = COLOR_GREEN(col), b = COLOR_BLUE(col);
	double white_bias = 255.*(1.0-alpha);
	Color dark = (((int) rint(r*alpha))<<16) |
		     (((int) rint(g*alpha))<<8 ) |
		     (((int) rint(b*alpha)) );
	Color light= (((int) rint(white_bias+r*alpha))<<16) |
		     (((int) rint(white_bias+g*alpha))<<8 ) |
		     (((int) rint(white_bias+b*alpha)) );
	GRect small, old;
	int x,y;
	int smallh = (h+1)/2;

	small.height = small.width = smallh;
	GDrawPushClip(gw,size,&old);
	for ( y=0; y<2; ++y ) {
	    small.y = y*smallh;
	    for ( x=size->x/smallh; x<=(size->x+size->width)/smallh; ++x ) {
		small.x = x*smallh;
		GDrawFillRect(gw,&small,((x+y)&1)?dark : light);
	    }
	}
	GDrawPopClip(gw,&old);
    }
}

static int col_e_h(GWindow gw, GEvent *event) {
    GRect size, r;
    if ( event->type==et_expose ) {
	struct gcol_data *d = GDrawGetUserData(gw);
	GDrawGetSize(d->colw,&size);
	r = event->u.expose.rect;
	if ( r.x<size.width/2 ) {
	    int col = (((int) rint(255*d->origcol.r))<<16) |
		      (((int) rint(255*d->origcol.g))<<8 ) |
		      (((int) rint(255*d->origcol.b))    );
	    if ( r.x+r.width>size.width/2 )
		r.width = size.width/2-r.x;
	    TranslucentRect(gw,&r,col,d->origcol.alpha,size.height);
	    r.width = event->u.expose.rect.width;
	}
	if ( r.x+r.width>size.width/2 ) {
	    r.width = r.x+r.width - size.width/2;
	    r.x = size.width/2;
	    if ( d->col.rgb ) {
		int col = (((int) rint(255*d->col.r))<<16) |
			  (((int) rint(255*d->col.g))<<8 ) |
			  (((int) rint(255*d->col.b))    );
		TranslucentRect(gw,&r,col,d->col.alpha,size.height);
	    } else {
		GDrawSetStippled(gw,2,0,0);
		GDrawSetBackground(gw,0xffff00);
		GDrawFillRect(gw,&r,0x000000);
		GDrawSetStippled(gw,0,0,0);
	    }
	}
    } else if ( event->type == et_mousedown ) {
	struct gcol_data *d = GDrawGetUserData(gw);
	GDrawGetSize(d->colw,&size);
	if ( event->u.mouse.x<size.width/2 ) {
	    d->col = d->origcol;
	    GCol_ShowTexts(d);
	    GDrawRequestExpose(d->wheelw,NULL,false);
	    GDrawRequestExpose(d->colw,NULL,false);
	    GDrawRequestExpose(d->gradw,NULL,false);
	}
    } else if ( event->type==et_resize ) {
	GDrawRequestExpose(gw,NULL,false);
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static void do_popup_color(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    struct gcol_data *d = GDrawGetUserData(gw);
    const struct hslrgba *col = mi->ti.userdata;

    d->col = *col;
    GCol_ShowTexts(d);
    GDrawRequestExpose(d->wheelw,NULL,false);
    GDrawRequestExpose(d->colw,NULL,false);
    GDrawRequestExpose(d->gradw,NULL,false);
}

static int popup_e_h(GWindow gw, GEvent *event) {
    struct gcol_data *d = GDrawGetUserData(gw);
    if ( recent_cols[0].hsv || d->user_cols[0].hsv ) {
	if ( event->type==et_expose ) {
	    GDrawDrawLine(gw,1,1,GRAD_WIDTH-2,1,0x000000);
	    GDrawDrawLine(gw,1,1,GRAD_WIDTH/2-1,GRAD_WIDTH-2,0x000000);
	    GDrawDrawLine(gw,GRAD_WIDTH/2-1,GRAD_WIDTH-2,GRAD_WIDTH-2,1,0x000000);
	} else if ( event->type == et_mousedown ) {
	    GMenuItem menu[2*USEFUL_MAX+2];
	    int i,j,k;

	    memset(menu,0,sizeof(menu));
	    for ( i=0; i<USEFUL_MAX && recent_cols[i].hsv; ++i ) {
		menu[i].ti.image = &blanks[i];
		cluts[i].clut[1] = (((int) rint(255.*recent_cols[i].r))<<16 ) |
				    (((int) rint(255.*recent_cols[i].g))<<8 ) |
				    (((int) rint(255.*recent_cols[i].b)) );
		menu[i].ti.fg = menu[i].ti.bg = COLOR_DEFAULT;
		menu[i].ti.userdata = &recent_cols[i];
		menu[i].invoke = do_popup_color;
	    }
	    j=i;
	    if ( i!=0 && d->user_cols[0].hsv ) {
		menu[i].ti.line = true;
		menu[i].ti.fg = menu[i].ti.bg = COLOR_DEFAULT;
		++i;
	    }
	    for ( k=0; k<USEFUL_MAX && d->user_cols[k].hsv; ++k, ++i, ++j ) {
		menu[i].ti.image = &blanks[j];
		cluts[j].clut[1] = (((int) rint(255.*d->user_cols[k].r))<<16 ) |
				    (((int) rint(255.*d->user_cols[k].g))<<8 ) |
				    (((int) rint(255.*d->user_cols[k].b)) );
		menu[i].ti.fg = menu[i].ti.bg = COLOR_DEFAULT;
		menu[i].ti.userdata = &d->user_cols[k];
		menu[i].invoke = do_popup_color;
	    }
	    GMenuCreatePopupMenu(gw,event, menu);
	}
    }

    if ( event->type == et_char )
return( false );

return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gcol_data *d = GDrawGetUserData(gw);
	d->col.rgb = d->col.hsv = false;
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

struct hslrgba GWidgetColorA(const char *title,struct hslrgba *defcol,struct hslrgba *usercols) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[23], boxes[6], *txarray[9][3], *wheelarray[3][3], *barray[8], *hvarray[3][3];
    GTextInfo label[21];
    struct gcol_data d;
    int k, i;
    double *offs[7] = { &d.col.h, &d.col.s, &d.col.v, &d.col.r, &d.col.g, &d.col.b, &d.col.alpha };
    char values[7][40];

    GProgressPauseTimer();
    memset(&d,0,sizeof(d));
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.is_dlg = true;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = title;
    pos.x = pos.y = 0;
    pos.width = pos.height = 200;
    d.gw = gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    if ( defcol!=NULL && (defcol->rgb || defcol->hsv || defcol->hsl ))
	d.col = *defcol;
    else if ( recent_cols[0].hsv ) {
	d.col = recent_cols[0];
	d.col.has_alpha = true;
    } else if ( usercols!=NULL && (usercols[0].rgb || usercols[0].hsv || usercols[0].hsl )) {
	d.col = usercols[0];
	d.col.has_alpha = true;
    } else {
	d.col.rgb = true;
	d.col.r = d.col.g = d.col.b = 1.0;
	d.col.alpha = 1.0;
	d.col.has_alpha = true;
    }
    if ( d.col.rgb ) {
	if ( !d.col.hsv )
	    gRGB2HSV((struct hslrgb *) &d.col);
    } else if ( d.col.hsv )
	gHSV2RGB((struct hslrgb *) &d.col);
    else {
	gHSL2RGB((struct hslrgb *) &d.col);
	gRGB2HSV((struct hslrgb *) &d.col);
    }
    d.origcol = d.col;
    if ( usercols!=NULL ) {
	for ( i=0; i<6 && (usercols[i].rgb || usercols[i].hsv || usercols[i].hsl ); ++i ) {
	    d.user_cols[i] = usercols[i];
	    if ( d.user_cols[i].rgb ) {
		if ( !d.user_cols[i].hsv )
		    gRGB2HSV((struct hslrgb *) &d.user_cols[i]);
	    } else if ( d.user_cols[i].hsv )
		gHSV2RGB((struct hslrgb *) &d.user_cols[i]);
	    else {
		gHSL2RGB((struct hslrgb *) &d.user_cols[i]);
		gRGB2HSV((struct hslrgb *) &d.user_cols[i]);
	    }
	}
    }

    k=0;
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    memset(&boxes,0,sizeof(boxes));

    gcd[k].gd.pos.height = gcd[k].gd.pos.width = 150;
    if ( d.col.has_alpha )
	gcd[k].gd.pos.height = gcd[k].gd.pos.width = 170;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.u.drawable_e_h = wheel_e_h;
    gcd[k].gd.cid = CID_Wheel;
    gcd[k].creator = GDrawableCreate;
    wheelarray[0][0] = &gcd[k++];

    gcd[k].gd.pos.height = 150; gcd[k].gd.pos.width = GRAD_WIDTH;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.u.drawable_e_h = grad_e_h;
    gcd[k].gd.cid = CID_Grad;
    gcd[k].creator = GDrawableCreate;
    wheelarray[0][1] = &gcd[k++]; wheelarray[0][2] = NULL;

    gcd[k].gd.pos.width = 150; gcd[k].gd.pos.height = GRAD_WIDTH;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.u.drawable_e_h = col_e_h;
    gcd[k].gd.cid = CID_Color;
    gcd[k].creator = GDrawableCreate;
    wheelarray[1][0] = &gcd[k++];

    gcd[k].gd.pos.width = GRAD_WIDTH; gcd[k].gd.pos.height = GRAD_WIDTH;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.u.drawable_e_h = popup_e_h;
    gcd[k].creator = GDrawableCreate;
    wheelarray[1][1] = &gcd[k++]; wheelarray[1][2] = NULL;
    wheelarray[2][0] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = wheelarray[0];
    boxes[2].creator = GHVBoxCreate;

    for ( i=0; i<7; ++i ) {
	label[k].text = (unichar_t *) _(labnames[i]);
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.flags = gg_visible | gg_enabled ;
	gcd[k++].creator = GLabelCreate;
	txarray[i][0] = &gcd[k-1];

	if ( i==0 )
	    sprintf( values[0], "%3.0f", *offs[0]);
	else
	    sprintf( values[i], "%.2f", *offs[i]);
	label[k].text = (unichar_t *) values[i];
	label[k].text_is_1byte = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.width = 60;
	gcd[k].gd.flags = gg_visible | gg_enabled;
	gcd[k].gd.cid = cids[i];
	gcd[k].gd.handle_controlevent = GCol_TextChanged;
	gcd[k++].creator = i==0 ? GNumericFieldCreate : GTextFieldCreate;
	txarray[i][1] = &gcd[k-1]; txarray[i][2] = NULL;
    }
    if ( !d.col.has_alpha ) {
	gcd[k-1].gd.flags &= ~gg_visible;
	gcd[k-2].gd.flags &= ~gg_visible;
    }
    txarray[i][0] = txarray[i][1] = GCD_Glue; txarray[i][2] = NULL;
    txarray[i+1][0] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = txarray[0];
    boxes[3].creator = GHVBoxCreate;

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = GCol_OK;
    gcd[k++].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[k-1]; barray[2] = GCD_Glue;

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = GCol_Cancel;
    gcd[k].creator = GButtonCreate;
    barray[3] = GCD_Glue; barray[4] = &gcd[k]; barray[5] = GCD_Glue;
    barray[6] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = barray;
    boxes[4].creator = GHBoxCreate;

    hvarray[0][0] = &boxes[2];
    hvarray[0][1] = &boxes[3];
    hvarray[0][2] = NULL;
    hvarray[1][0] = &boxes[4];
    hvarray[1][1] = GCD_ColSpan;
    hvarray[1][2] = NULL;
    hvarray[2][0] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = hvarray[0];
    boxes[0].creator = GHVGroupCreate;
    GGadgetsCreate(gw,boxes);
    GHVBoxSetExpandableRow(boxes[0].ret,0);
    GHVBoxSetExpandableCol(boxes[0].ret,0);
    GHVBoxSetExpandableRow(boxes[2].ret,0);
    GHVBoxSetExpandableCol(boxes[2].ret,0);
    GHVBoxSetExpandableRow(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandgluesame);
    GHVBoxFitWindow(boxes[0].ret);
    d.wheelw = GDrawableGetWindow(GWidgetGetControl(gw,CID_Wheel));
    d.gradw  = GDrawableGetWindow(GWidgetGetControl(gw,CID_Grad));
    d.colw   = GDrawableGetWindow(GWidgetGetControl(gw,CID_Color));
    GDrawSetVisible(gw,true);

    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    if ( d.grad!=NULL )
	GImageDestroy(d.grad);
    if ( d.wheel!=NULL )
	GImageDestroy(d.wheel);
    if ( d.col.hsv || d.col.rgb ) {
	int j;
	for ( j=0; j<USEFUL_MAX; ++j )
	    if ( d.col.r==recent_cols[j].r && d.col.g==recent_cols[j].g &&
		    d.col.b==recent_cols[j].b &&
		    (!d.col.has_alpha || d.col.alpha==recent_cols[j].alpha))
	break;
	if ( j==USEFUL_MAX )
	    --j;
	for ( ; j>0; --j )
	    recent_cols[j] = recent_cols[j-1];
	recent_cols[0] = d.col;
    }
return( d.col );
}

struct hslrgb GWidgetColor(const char *title,struct hslrgb *defcol,struct hslrgb *usercols) {
    struct hslrgba def, users[USEFUL_MAX+1], *d, *u, temp;
    struct hslrgb ret;
    int i;
    if ( usercols==NULL )
	u = NULL;
    else {
	for ( i=0; i<6 && (usercols[i].rgb || usercols[i].hsv || usercols[i].hsl ); ++i ) {
	    memcpy(&users[i],&usercols[i],sizeof(struct hslrgb));
	    users[i].has_alpha = 0;
	    users[i].alpha = 1.0;
	}
	u = users;
    }

    if ( defcol!=NULL )
	memcpy(&def,defcol,sizeof(*defcol));
    else if ( recent_cols[0].hsv )
	def = recent_cols[0];
    else if ( u!=NULL && (u[0].rgb || u[0].hsv || u[0].hsl ))
	def = u[0];
    else {
	def.rgb = true;
	def.r = def.g = def.b = 1.0;
    }
    def.has_alpha = false;
    def.alpha = 1.0;
    d = &def;
    temp = GWidgetColorA(title,d,u);
    memcpy(&ret,&temp,sizeof(struct hslrgb));
return( ret );
}
