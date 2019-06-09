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

/*
 This file exists because the XFree wacom_drv.o does not work for me.
 When properly configured it sends no device events. It sends no core events.

 However thanks to John E. Joganic wacdump program I know what the event
 stream on /dev/input/event0 looks like
	http://linux.joganic.com/wacom/
 Some docs are at: http://www.frogmouth.net/hid-doco/x286.html

 To the best of my knowledge this is entirely linux specific and won't work
 anywhere else.

 This file then, attempts to generate the correct device events and pass them
 on to my windows

The following options configure it:
	_WACOM_DRV_BROKEN	if defined then enable this driver.
	_WACOM_SILENT		Don't print anything if we can't open the device
	_WACOM_SEND_EVENTS	Send (core-like) events even to windows that aren't ours
		Other windows can detect that these aren't "real" events
		so this often doesn't work, and we can't get the state field
		right.
	_WACOM_EXTRA_TOP
	_WACOM_EXTRA_BOTTOM
	_WACOM_EXTRA_LEFT
	_WACOM_EXTRA_RIGHT
		My tablet has a rectangle drawn on it, but even outside this
		rectangle the tablet is active. These bounds allow you to
		scale just this rectangle to the screen. If you set them all
		to 0 the entire table (even the bits outside the rectangle)
		is scaled to the screen.
*/

#ifndef _WACOM_DRV_BROKEN
static void *a_file_must_define_something = (void *) &a_file_must_define_something;
#else

# ifndef _WACOM_EXTRA_TOP
#  define _WACOM_EXTRA_TOP	1120		/* About this many lines above the rectangle */
#  define _WACOM_EXTRA_LEFT	0		/* No columns to the left of the rectangle */
#  define _WACOM_EXTRA_BOTTOM	(-240)		/* Actually the ymax value reported by the driver is less than what I get at the edge of the rectangle, but the tablet will generate values greater than this maximum */
#  define _WACOM_EXTRA_RIGHT	0
# endif

# include "gxdrawP.h"

# include <X11/Xatom.h>

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

# include <linux/input.h>
# include <sys/ioctl.h>

# include <errno.h>
# include <fcntl.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <sys/types.h>

# include <math.h>

typedef struct wacom_state {
    int sizex, sizey;
    double scale;
    int lastx, lasty;
    int screenx, screeny;
    int last_pressure;
    int max_pressure;
    int last_distance;
    int last_xtilt;
    int last_ytilt;
    int last_wheel;	/* always seems to be 0? */
    int mouse_button_state;
    int stylus_button_state;
    int eraser_button_state;
    enum tools { at_mouse=2, at_stylus=4, at_eraser=8 } active_tool;

    struct timeval time_off;		/* offset from gettimeofday to X server timestamp */
    int win_x, win_y;
    int x_state;
    Window xw;
    GXWindow gw, grabbed;
} WState;

static void Wacom_FigureTimeOffset(GXDisplay *gdisp) {
    /* Figure the offset between real time and server timestamps */
    /* First I go through a lot of rigamaroul to get an event with a time */
    /*  stamp from the server */
    struct timeval tv, server_tv;
    WState *ws = gdisp->wacom_state;
    XSetWindowAttributes attrs;
    XEvent e;
    char prop_data[4];

    attrs.event_mask = PropertyChangeMask;
    XChangeWindowAttributes(gdisp->display,gdisp->root,CWEventMask,&attrs);
    XChangeProperty(gdisp->display,gdisp->root,XA_STRING,XA_STRING,8,PropModeReplace,
	    prop_data,sizeof(prop_data));
    XWindowEvent(gdisp->display,gdisp->root,PropertyChangeMask,&e);

    gettimeofday(&tv,NULL);
    server_tv.tv_sec = e.xproperty.time/1000;
    server_tv.tv_usec = (e.xproperty.time%1000)*1000;
    ws->time_off.tv_usec = tv.tv_usec-server_tv.tv_usec;
    ws->time_off.tv_sec = tv.tv_sec-server_tv.tv_sec;
    if ( ws->time_off.tv_usec<0 ) {
	ws->time_off.tv_usec += 1000000;
	--ws->time_off.tv_sec;
    }

    attrs.event_mask = 0;
    XChangeWindowAttributes(gdisp->display,gdisp->root,CWEventMask,&attrs);
}

void _GXDraw_Wacom_Init(GXDisplay *gdisp) {
    WState *ws;
/* Some of this code is stolen from xf86Wacom.c, the XFree driver for wacom */
    char name[256];
    uint8 bit[2][(KEY_MAX-1)/8];
    int abs[5];
    int j;
/* End stolen section */
    double xscale, yscale;

    gdisp->wacom_fd = open("/dev/input/event0",O_RDONLY);
    if ( gdisp->wacom_fd==-1 ) {
# ifndef _WACOM_SILENT
	if ( errno==EACCES )
	    fprintf( stderr,
"/dev/input/event0 is protected so you cannot read from it\n"
"If you wish PfaEdit to process wacom events directly you must:\n"
"\t$ su\n"
"\t# chmod 644 /dev/input/event0\n" );
	else if ( errno==ENODEV )
	    fprintf( stderr,
"The system claims no device is associated with /dev/input/event0.\n"
"This is probably because the wacom driver isn't properly initialized on\n"
"your system. Try the following:\n\t$ su\n"
"\t# modprobe wacom.o\n"
"\t# modprobe evdev\n"
"You may want to alter your /etc/rc.d/rc.local file to do this automagically.\n" );
	else if ( errno==ENOENT )
	    fprintf( stderr,
"The file /dev/input/event0 does not exist on your system. This means that\n"
"your machine is not configured to handle the type of event processing used\n"
"by the current wacom driver. All I can suggest is that you upgrade to a newer\n"
"version of the OS" );
	else
	    perror("Failed to open /dev/input/event0");
#endif
return;
    }
    fcntl(gdisp->wacom_fd,F_SETFL,O_NONBLOCK);

    ws = gdisp->wacom_state = calloc(1,sizeof(WState));

    ws->screenx = -1;
    ws->screeny = -1;

    /* See http://www.frogmouth.net/hid-doco/x286.html for docs */
    ioctl(gdisp->wacom_fd, EVIOCGNAME(sizeof(name)), name);
    fprintf( stderr, "GDraw Wacom Driver opened device %s\n", name);

    memset(bit, 0, sizeof(bit));
    ioctl(gdisp->wacom_fd, EVIOCGBIT(0, EV_MAX), bit[0]);

    if (bit[0][EV_ABS/8] & (1<<(EV_ABS%8)) ) {
	ioctl(gdisp->wacom_fd, EVIOCGBIT(EV_ABS, KEY_MAX), bit[1]);
	for (j = 0; j < ABS_MAX; j++) if (bit[1][j/8] & (1<<(j%8))) {
	    ioctl(gdisp->wacom_fd, EVIOCGABS(j), abs);
	    /* abs contains current-val, min-val, max-val ?flat? ?fuzz? */
	    /* get initial state variables that have values. Also their bounds, if relevant */
	    switch (j) {
	      case ABS_X:
		ws->sizex = abs[2];
		ws->lastx = abs[0];
	      break;
	      case ABS_Y:
		ws->sizey = abs[2];
		ws->lasty = abs[0];
	      break;
	      case ABS_WHEEL:
		ws->last_wheel = abs[0];
	      break;
	      case ABS_PRESSURE:
		ws->max_pressure = abs[2];
		ws->last_pressure = abs[0];
	      break;
	      case ABS_DISTANCE:
		ws->last_distance = abs[0];
	      break;
	      case ABS_TILT_X:
		ws->last_xtilt = abs[0];
	      break;
	      case ABS_TILT_Y:
		ws->last_ytilt = abs[0];
	      break;
	    }
	}
    }

    fprintf( stderr, "Wacom XMax=%d YMax=%d PressureMax=%d\n",
	    ws->sizex, ws->sizey, ws->max_pressure );
    ws->sizex -= _WACOM_EXTRA_LEFT+_WACOM_EXTRA_RIGHT;
    ws->sizey -= _WACOM_EXTRA_TOP+_WACOM_EXTRA_BOTTOM;
    if ( ws->sizex<=0 )
	xscale = 1;
    else
	xscale = gdisp->groot->pos.width / (double) ws->sizex;
    if ( ws->sizey<=0 )
	yscale = xscale;
    else {
	yscale = gdisp->groot->pos.height / (double) ws->sizey;
	if ( ws->sizex==0 )
	    xscale = yscale;
    }
    if ( xscale>yscale )
	yscale = xscale;
    ws->scale = yscale;

    if ( ws->lastx!=0 )
	ws->screenx = rint((ws->lastx-_WACOM_EXTRA_LEFT)*ws->scale);
    if ( ws->lasty!=0 )
	ws->screeny = rint((ws->lasty-_WACOM_EXTRA_TOP)*ws->scale);

    Wacom_FigureTimeOffset(gdisp);
}

# ifdef _WACOM_SEND_EVENTS
static void Wacom_SendCoreEvent(GXDisplay *gdisp,int button,int type) {
    WState *ws = gdisp->wacom_state;
    XEvent e;
    struct timeval tv;

    /* We don't need to send MotionNotify events becasue WarpPointer does */
    /*  that for us (it won't get the state as we might like it, but there's */
    /*  nothing I can do about that) */
    if ( ws->xw==None )
return;

    memset(&e,0,sizeof(e));
    e.type = type==et_mousedown ? ButtonPress : ButtonRelease;
    e.xbutton.display = gdisp->display;
    e.xbutton.window = ws->xw;
    e.xbutton.root = gdisp->root;
    e.xbutton.subwindow = None;
    e.xbutton.x = ws->win_x;
    e.xbutton.y = ws->win_y;
    e.xbutton.x_root = ws->screenx;
    e.xbutton.y_root = ws->screeny;
    e.xbutton.state = ws->x_state;
    e.xbutton.same_screen = true;

    gettimeofday(&tv,NULL);
    tv.tv_sec -= ws->time_off.tv_sec;
    tv.tv_usec -= ws->time_off.tv_usec;
    if ( tv.tv_usec<0 ) {
	tv.tv_usec += 1000000;
	--tv.tv_sec;
    }
    e.xbutton.time = tv.tv_sec*1000 + tv.tv_usec/1000;
    XSendEvent(gdisp->display,e.xbutton.window,true,
	    type==et_mousedown ? ButtonPressMask : ButtonReleaseMask,
	    &e );
}
# endif
static void Wacom_SendEvent(GXDisplay *gdisp,char *device,int button,int type) {
    WState *ws = gdisp->wacom_state;
    GEvent e;
    struct timeval tv;
    Window junk;

    e.type = type;
    if ( ws->grabbed!=NULL && ws->grabbed!=ws->gw ) {
	XTranslateCoordinates(gdisp->display,ws->gw->w,ws->grabbed->w,
		ws->win_x,ws->win_y,
		&ws->win_x,&ws->win_y,&junk);
	ws->gw = ws->grabbed;
	ws->xw = ws->grabbed->w;
    }
    if ( ws->gw==NULL ) {		/* Not one of our windows */
# ifdef _WACOM_SEND_EVENTS
	if ( type==et_mouseup || type==et_mousedown )
	    Wacom_SendCoreEvent(gdisp,button,type);
# endif
return;
    }
    e.w = (GWindow) (ws->gw);
    if ( type==et_mousedown )
	ws->grabbed = ws->gw;
    else if ( type==et_mouseup )
	ws->grabbed = NULL;

    gettimeofday(&tv,NULL);
    tv.tv_sec -= ws->time_off.tv_sec;
    tv.tv_usec -= ws->time_off.tv_usec;
    if ( tv.tv_usec<0 ) {
	tv.tv_usec += 1000000;
	--tv.tv_sec;
    }
    e.u.mouse.time = tv.tv_sec*1000 + tv.tv_usec/1000;

    e.u.mouse.x = ws->win_x;
    e.u.mouse.y = ws->win_y;
    e.u.mouse.state = ws->active_tool == at_mouse ? ws->mouse_button_state :
		      ws->active_tool == at_stylus ? ws->stylus_button_state :
			  ws->eraser_button_state;
    e.u.mouse.state |= (ws->x_state&0xff);
    e.u.mouse.device = device;
    e.u.mouse.button = button;

    if ( type==et_mousedown ) {
	int diff, temp;
	if (( diff = ws->win_x-gdisp->bs.release_x )<0 ) diff= -diff;
	if (( temp = ws->win_y-gdisp->bs.release_y )<0 ) temp= -temp;
	if ( diff+temp<gdisp->bs.double_wiggle &&
		ws->xw == gdisp->bs.release_w &&
		button == gdisp->bs.release_button &&
		e.u.mouse.time-gdisp->bs.release_time < gdisp->bs.double_time &&
		e.u.mouse.time >= gdisp->bs.release_time )	/* Time can wrap */
	    ++ gdisp->bs.cur_click;
	else
	    gdisp->bs.cur_click = 1;
	e.u.mouse.clicks = gdisp->bs.cur_click;
    } else if ( type==et_mouseup ) {
	gdisp->bs.release_time = e.u.mouse.time;
	gdisp->bs.release_w = ws->xw;
	gdisp->bs.release_x = ws->win_x;
	gdisp->bs.release_y = ws->win_y;
	gdisp->bs.release_button = button;
	e.u.mouse.clicks = gdisp->bs.cur_click;
    }

    e.u.mouse.pressure = ws->last_pressure;
    e.u.mouse.xtilt = ws->last_xtilt;
    e.u.mouse.ytilt = ws->last_ytilt;
    e.u.mouse.separation = ws->last_distance;

    GDrawPostEvent(&e);
}

static Bool Wacom_FindMotion(Display *d,XEvent *e,char *arg) {
    WState *ws = (WState *) arg;

return( e->type==MotionNotify &&
	e->xmotion.x_root==ws->screenx &&
	e->xmotion.y_root==ws->screeny );
}

static void Wacom_MovePointer(GXDisplay *gdisp) {
    WState *ws = gdisp->wacom_state;
    XEvent e;
    void *ret;

    /* Old docs say that XWarpPointer generates events just as if the user */
    /*  moved the mouse. New docs do not say this, but there does seem to */
    /*  be at least one MotionNotify event (we'll only get it for our own */
    /*  windows, of course */
    XWarpPointer(gdisp->display,None,gdisp->root,0,0,0,0,ws->screenx,ws->screeny);

    ws->gw = NULL;
    ws->xw = None;
    XSync(gdisp->display,false);
    if ( XCheckIfEvent(gdisp->display,&e,Wacom_FindMotion,(char *)ws)) {
	ws->xw = e.xany.window;
	if ( XFindContext(gdisp->display,e.xany.window,gdisp->mycontext,(void *) &ret)==0 )
	    ws->gw = (GXWindow) ret;
	if ( ws->gw!=NULL && _GXDraw_WindowOrParentsDying((GXWindow) ws->gw))
	    ws->gw = NULL;
	ws->win_x = e.xmotion.x;
	ws->win_y = e.xmotion.y;
	ws->x_state = e.xmotion.state;
    }
# ifdef _WACOM_SEND_EVENTS
    if ( ws->xw==None ) {
	int x,y,junk;
	unsigned int state;
	Window child, cur, root;

	cur = gdisp->root;
	x = ws->screenx; y = ws->screeny;
	for (;;) {
	    XQueryPointer(gdisp->display,cur,&root,&child,&junk,&junk,&x,&y,&state);
	    if ( child==None )
	break;
	    cur = child;
	}

	ws->xw = cur;
	ws->win_x = x;
	ws->win_y = y;
	ws->x_state = state;
    }
# endif
}

static void Wacom_TestMotion(GXDisplay *gdisp) {
    int x, y;
    WState *ws = gdisp->wacom_state;
    char *device;

    x = rint((ws->lastx-_WACOM_EXTRA_LEFT)*ws->scale);
    y = rint((ws->lasty-_WACOM_EXTRA_TOP)*ws->scale);
    if ( x==ws->screenx && y==ws->screeny )
return;

    /* Don't move cursor during init process */
    if ( ws->screenx==-1 || ws->screeny==-1 ) {
	if ( x!=0 )
	    ws->screenx = x;
	if ( y!=0 )
	    ws->screeny = y;
return;
    }

    ws->screenx = x; ws->screeny = y;
    if ( ws->active_tool==0 ) {
	fprintf( stderr, "Warning: Don't know what the current tool is on the wacom tablet.\n Assuming Mouse. If that is wrong lift the tool up off the palette\n and replace it.\n" );
	ws->active_tool = at_mouse;
    }
    device = ws->active_tool==at_mouse ? "Mouse1" :
	     ws->active_tool==at_stylus ? "stylus" :
		 "eraser" ;
    Wacom_MovePointer(gdisp);
    Wacom_SendEvent(gdisp,device,0,et_mousemove);
}

static void Wacom_ProcessEvent(GXDisplay *gdisp,WState *ws,struct input_event *ev) {

    switch ( ev->type ) {
      case EV_MSC:
	/* I don't care about the serial number */
      break;
      case EV_KEY:
	if ( ev->code == BTN_LEFT ) {
	    if ( ev->value ) ws->mouse_button_state |= 0x100;
	    else ws->mouse_button_state &= ~0x100;
	    Wacom_SendEvent(gdisp,"Mouse1",1,ev->value?et_mousedown:et_mouseup);
	} else if ( ev->code == BTN_RIGHT ) {
	    if ( ev->value ) ws->mouse_button_state |= 0x400;
	    else ws->mouse_button_state &= ~0x400;
	    Wacom_SendEvent(gdisp,"Mouse1",3,ev->value?et_mousedown:et_mouseup);
	} else if ( ev->code == BTN_MIDDLE ) {
	    if ( ev->value ) ws->mouse_button_state |= 0x200;
	    else ws->mouse_button_state &= ~0x200;
	    Wacom_SendEvent(gdisp,"Mouse1",2,ev->value?et_mousedown:et_mouseup);
	} else if ( ev->code == BTN_TOUCH ) {
	    if ( ws->active_tool == at_stylus ) {
		if ( ev->value ) ws->stylus_button_state |= 0x100;
		else ws->stylus_button_state &= ~0x100;
		Wacom_SendEvent(gdisp,"stylus",1,ev->value?et_mousedown:et_mouseup);
	    } else {
		if ( ev->value ) ws->eraser_button_state |= 0x100;
		else ws->eraser_button_state &= ~0x100;
		Wacom_SendEvent(gdisp,"eraser",1,ev->value?et_mousedown:et_mouseup);
	    }
	} else if ( ev->code==BTN_STYLUS2 ) {
	    /* Button on the pen was pushed */
	    if ( ev->value ) ws->stylus_button_state |= 0x200;
	    else ws->stylus_button_state &= ~0x200;
	    Wacom_SendEvent(gdisp,"stylus",2,ev->value?et_mousedown:et_mouseup);
	} else if ( ev->code == BTN_TOOL_PEN && ev->value ) {
	    ws->active_tool = at_stylus;
	} else if ( ev->code == BTN_TOOL_RUBBER && ev->value ) {
	    ws->active_tool = at_eraser;
	} else if ( ev->code == BTN_TOOL_MOUSE && ev->value ) {
	    ws->active_tool = at_mouse;
	}
      break;
      case EV_ABS:
	if ( ev->code==ABS_X ) {
	    ws->lastx = ev->value;
	    Wacom_TestMotion(gdisp);
	} else if ( ev->code==ABS_Y ) {
	    ws->lasty = ev->value;
	    Wacom_TestMotion(gdisp);
	} else if ( ev->code==ABS_WHEEL )
	    ws->last_wheel = ev->value;
	else if ( ev->code==ABS_PRESSURE )
	    ws->last_pressure = ev->value;
	else if ( ev->code==ABS_DISTANCE )
	    ws->last_distance = ev->value;
	else if ( ev->code==ABS_TILT_X )
	    ws->last_xtilt = ev->value;
	else if ( ev->code==ABS_TILT_Y )
	    ws->last_ytilt = ev->value;
      break;
    }
}

void _GXDraw_Wacom_TestEvents(GXDisplay *gdisp) {
    struct input_event ev[EV_MAX];
    int i,len;

    while ( (len=read(gdisp->wacom_fd,ev,sizeof(ev)))>0 ) {
	len /= sizeof(ev[0]);
	for ( i=0; i<len; ++i )
	    Wacom_ProcessEvent(gdisp,gdisp->wacom_state,&ev[i]);
    }
}
#endif
