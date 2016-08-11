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

/* There are two configurable option here that the configure script can't
    figure out:

	_WACOM_DRV_BROKEN
on my system the XFree driver for the WACOM tablet sends no events. I don't
understand the driver, so I'm not able to attempt fixing it. However thanks
to John E. Joganic wacdump program I do know what the event stream on
	/dev/input/event0
looks like, and I shall attempt to simulate driver behavior from that

So set macro this in your makefile if you have problems too, and then
change the protection on /dev/input/event0 so that it is world readable

(there is now a working XFree driver for wacom, but you have to get it from
John, it's not part of standard XFree yet).

	_COMPOSITE_BROKEN
on XFree with the composite extension active, scrolling windows with docket
palletes may result in parts of the palettes duplicated outside their dock
rectangles.

Defining this macro forces redisplay of the entire window area, which is
slightly slower, but should make no significant difference on a machine
capable of using composite.
*/

#ifndef _XDRAW_H
#define _XDRAW_H

#include <fontforge-config.h>

#ifndef FONTFORGE_CAN_USE_GDK

#ifndef X_DISPLAY_MISSING
# include <X11/X.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# ifndef _NO_XINPUT
#  include <X11/extensions/XInput.h>
#  include <X11/extensions/XI.h>
# endif
# ifndef _NO_XKB
#   include <X11/XKBlib.h>
/*# include <X11/extensions/XKBgeom.h>*/
# endif
# ifndef _NO_LIBCAIRO
#  include <cairo/cairo.h>
#  include <cairo/cairo-xlib.h>
# endif
#  define GTimer GTimer_GTK
#  include <ft2build.h>
#  include <X11/Xft/Xft.h>
#  include <pango/pango.h>
#  undef GTimer
#endif

#include "gdrawP.h"

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

typedef struct gcstate {
    void *gc;
    Color fore_col;		/* desired */
    Color back_col;		/* desired */
    GRect clip;
    unsigned int copy_through_sub_windows: 1;
    unsigned int bitmap_col: 1;			/* fore_col is mapped for bitmap */
    int16 dash_len, skip_len;
    int16 line_width;
    int16 dash_offset;
    int16 ts;
    int32 ts_xoff, ts_yoff;
    struct font_data *cur_font;
} GCState;

#ifndef X_DISPLAY_MISSING
struct gxinput_context {
    GWindow w;
    enum gic_style style;
    XIC ic;
    struct gxinput_context *next;
    XPoint ploc;
    XPoint sloc;
};

typedef struct gxwindow /* :GWindow */ {
    GGC *ggc;
    struct gxdisplay *display;
    int (*eh)(GWindow,GEvent *);
    GRect pos;				/* Filled in when Resize events happen */
    struct gxwindow *parent;
    void *user_data;
    void *widget_data;
    Window w;
    unsigned int is_visible: 1;		/* Filled in when MapNotify events happen */
    unsigned int is_pixmap: 1;
    unsigned int is_toplevel: 1;
    unsigned int visible_request: 1;
    unsigned int is_dying: 1;
    unsigned int is_popup: 1;
    unsigned int disable_expose_requests: 1;
    unsigned int usecairo: 1;		/* use a cairo context */
    unsigned int is_dlg: 1;
    unsigned int not_restricted: 1;
    unsigned int was_positioned: 1;
	/* is_bitmap can be found in the bitmap_col field of the ggc */
    unsigned int restrict_input_to_me: 1;/* for dialogs, no input outside of dlg */
    unsigned int redirect_chars_to_me: 1;/* ditto, we get any input outside of us */
    unsigned int istransient: 1;	/* has transient for hint set */
    unsigned int isverytransient: 1;
    GWindow redirect_from;		/* only redirect input from this window and its children */
    GCursor cursor;
    Window parentissimus;
    struct gxinput_context *gic, *all;
    PangoLayout  *pango_layout;
#ifndef _NO_LIBCAIRO
    cairo_t *cc;
    cairo_surface_t *cs;
    struct gcstate cairo_state;
#endif
    XftDraw *xft_w;
    Window transient_owner;
} *GXWindow;

struct colstate {
    int16 red_shift, green_shift, blue_shift;
    int32 red_bits_mask, green_bits_mask, blue_bits_mask;
    int16 red_bits_shift, green_bits_shift, blue_bits_shift;
    int32 alpha_bits;
    RevCMap *rev;
    unsigned int is_grey: 1;
};

struct gatoms {
    Atom wm_del_window;
    Atom wm_protocols;
    Atom drag_and_drop;
};

/* Input states:
    normal => input goes to the expected window
    restricted => input only goes to one window (and its children)
    redirected => characters from any window go to one window
    targetted_redirect => characters from one special window (and its children) go to another window
*/
enum inputtype { it_normal, it_restricted, it_redirected, it_targetted };
struct inputRedirect {
    enum inputtype it;
    GWindow cur_dlg;		/* This one always gets input */
    GWindow inactive;		/* This one gives its input to the dlg */
    struct inputRedirect *prev;
};

struct button_state {
    Time release_time;
    Window release_w;
    int16 release_x, release_y;
    int16 release_button;
    int16 cur_click;
    int16 double_time;		/* max milliseconds between release & click */
    int16 double_wiggle;	/* max pixel wiggle allowed between release&click */
};

struct seldata {
    int32 typeatom;
    int32 cnt;
    int32 unitsize;
    void *data;
    void *(*gendata)(void *,int32 *len);
    /* Either the data are stored here, or we use this function to generate them on the fly */
    void (*freedata)(void *);
    struct seldata *next;
};

struct gxselinfo {
    int32 sel_atom;		/* Either XA_PRIMARY or CLIPBOARD */
    GXWindow owner;
    Time timestamp;
    struct seldata *datalist;
};

struct gxseltypes {
    Time timestamp;		/* for last request for selection types */
    int cnt;			/* number of types return */
    Atom *types;		/* array of selection types */
};

struct things_to_do {
    void (*func)(void *);
    void *data;
    struct things_to_do *next;
};

struct xthreaddata {
# ifdef HAVE_PTHREAD_H
    pthread_mutex_t sync_mutex;		/* controls access to the rest of this structure */
    struct things_to_do *things_to_do;
# endif
    int sync_sock, send_sock;		/* socket on which to send sync events to thread displaying screen */
};

struct gimageglobals {
    XImage *img, *mask;
    int16 *red_dith, *green_dith, *blue_dith;
    int32 iwidth, iheight;
};

struct atomdata { char *atomname; int32 xatom; };

struct inputdevices {
    char *name;
    int devid;
# ifndef _NO_XINPUT
    XDevice *dev;
# else
    int *dev;
# endif
    int event_types[5];	/* mousemove, mousedown, mouseup, char, charup */
};

struct xkb {
    int opcode, event, error;
};

typedef struct gxdisplay /* : GDisplay */ {
    struct displayfuncs *funcs;
    void *semaphore;				/* To lock the display against multiple threads */
    struct font_state *fontstate;
    int16 res;
    int16 scale_screen_by;			/* When converting screen pixels to printer pixels: multiply by this then divide by 16 */
    GXWindow groot;
    Color def_background, def_foreground;
    uint16 mykey_state;
    uint16 mykey_keysym;
    uint16 mykey_mask;
    fd_callback_t fd_callbacks[ gdisplay_fd_callbacks_size ];
    int fd_callbacks_last;
    unsigned int mykeybuild: 1;
    unsigned int default_visual: 1;
    unsigned int do_dithering: 1;
    unsigned int focusfollowsmouse: 1;
    unsigned int top_offsets_set: 1;
    unsigned int wm_breaks_raiseabove: 1;
    unsigned int wm_raiseabove_tested: 1;
    unsigned int endian_mismatch: 1;
    unsigned int macosx_cmd: 1;		/* if set then map state=0x20 to control */
    unsigned int twobmouse_win: 1;	/* if set then map state=0x40 to mouse button 2 */
    unsigned int devicesinit: 1;	/* the devices structure has been initialized. Else call XListInputDevices */
    unsigned int expecting_core_event: 1;/* when we move an input extension device we generally get two events, one for the device, one later for the core device. eat the core event */
    unsigned int has_xkb: 1;		/* we were able to initialize the XKB extension */
    unsigned int supports_alpha_images: 1;
    unsigned int supports_alpha_windows: 1;
    int err_flag;
    char * err_report;
    struct gcstate gcstate[2];			/* 0 is state for normal images, 1 for bitmap (pixmaps) */
    Display *display;
    Window root;
    Window virtualRoot;				/* Some window managers create a "virtual root" that is bigger than the real root and all decoration windows live in it */
    int16 screen;
    int16 depth;
    int16 pixel_size;				/* 32bit displays usually have a 24bit depth */
    int16 bitmap_pad;				/* 8bit displays sometimes pad on 32bit boundaries */
    Visual *visual;
    Colormap cmap;
    struct colstate cs;
    struct gatoms atoms;
    struct button_state bs;
    XComposeStatus buildingkeys;
    struct inputRedirect *input;
    struct gimageglobals gg;
    Pixmap grey_stipple;
    Pixmap fence_stipple;
    int32 mycontext;
    int16 top_window_count;
    GTimer *timers;
    Time last_event_time;
    struct gxselinfo selinfo[sn_max];
    int amax, alen;
    struct atomdata *atomdata;
    struct gxseltypes seltypes;
    int32 SelNotifyTimeout;		/* In seconds (time to give up on requests for selections) */
    struct {
	Window w;
	GWindow gw;
	int x,y;
	int rx,ry;
    } last_dd;
    struct xthreaddata xthread;
    int16 off_x, off_y;			/* The difference between where I asked */
    					/*  to put a top level window, and where */
			                /*  it ended up */
    GWindow grab_window;		/* For reasons I don't understand the */
	/* X Server seems to deliver events to my windows even when the pointer*/
	/* is grabbed by another window. If the pointer is outside of any of my*/
	/* windows then the event goes to my grab window, but if it's in one of*/
	/* my other windows, then that window gets it and is mightily confused*/
	/* So this field lets us do it right. when the pointer is grabbed the */
	/* events go to the grab window. It seems so simple... */
    int16 desired_depth, desired_vc, desired_cm;
    int16 xres;				/* What X Thinks the resolution is */
    XIM im;				/* Input method for current locale */
    XFontSet def_im_fontset;
    struct inputdevices *inputdevices;
    int n_inputdevices;
# ifdef _WACOM_DRV_BROKEN
    struct wacom_state *wacom_state;
    int wacom_fd;
# endif
    GXWindow default_icon;
    struct xkb xkb;
    PangoFontMap *pango_fontmap;
    PangoContext *pango_context;
# if !defined(_NO_LIBCAIRO)
    PangoFontMap *pangoc_fontmap;
    PangoContext *pangoc_context;
# endif
# if defined(__MINGW32__) || __CygWin
    int32  mousemove_last_x;
    int32  mousemove_last_y;
#endif
    Window last_nontransient_window;
} GXDisplay;

# define Pixel32(gdisp,col) ( Pixel16(gdisp,col) | (gdisp)->cs.alpha_bits )
# define Pixel24(gdisp,col) ( ((((col)>>16)&0xff)<<(gdisp)->cs.red_shift) | ((((col)>>8)&0xff)<<(gdisp)->cs.green_shift) | (((col)&0xff)<<(gdisp)->cs.blue_shift) )
# define Pixel16(gdisp,col) ( ((((col)>>(gdisp)->cs.red_bits_shift)&(gdisp)->cs.red_bits_mask)<<(gdisp)->cs.red_shift) | ((((col)>>(gdisp)->cs.green_bits_shift)&(gdisp)->cs.green_bits_mask)<<(gdisp)->cs.green_shift) | (((col>>(gdisp)->cs.blue_bits_shift)&(gdisp)->cs.blue_bits_mask)<<(gdisp)->cs.blue_shift) )
# define FixEndian16(col)	((((col)&0xff)<<8) | ((col>>8)&0xff))
# define FixEndian32(col)	((((col)&0xff)<<24) | ((col&0xff00)<<8) | ((col>>8)&0xff00))

#else /* No X */

# define gxwindow gwindow
# define gxdisplay gdisplay
typedef struct gwindow *GXWindow;
typedef struct gdisplay GXDisplay;

#endif

extern int _GXDraw_WindowOrParentsDying(GXWindow gw);
extern void _GXCDraw_NewWindow(GXWindow nw);
extern void _GXDraw_Image(GWindow, GImage *, GRect *src, int32 x, int32 y);
extern void _GXDraw_TileImage(GWindow, GImage *, GRect *src, int32 x, int32 y);
extern void _GXDraw_Glyph(GWindow, GImage *, GRect *src, int32 x, int32 y);
extern void _GXDraw_ImageMagnified(GWindow, GImage *, GRect *src, int32 x, int32 y, int32 width, int32 height);
extern GImage *_GXDraw_CopyScreenToImage(GWindow, GRect *rect);

extern void _GXDraw_SetClipFunc(GXDisplay *gdisp, GGC *mine);
extern struct gcol *_GXDraw_GetScreenPixelInfo(GXDisplay *gdisp, int red, int green, int blue);
extern unsigned long _GXDraw_GetScreenPixel(GXDisplay *gdisp, Color col);
extern GImage *_GImageExtract(struct _GImage *base,GRect *src,GRect *size, double xscale, double yscale);

extern void _XSyncScreen(void);

#if !defined(__MINGW32__)
extern int GDrawKeyToXK(int keysym);
#else
extern int GDrawKeyToVK(int keysym);
#endif

# ifdef _WACOM_DRV_BROKEN
void _GXDraw_Wacom_Init(GXDisplay *gdisp);
void _GXDraw_Wacom_TestEvents(GXDisplay *gdisp);
# endif	/* Wacom fix */


#endif // FONTFORGE_CAN_USE_GDK

#endif
