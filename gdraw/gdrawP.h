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
#ifndef _GDRAWP_H
#define _GDRAWP_H

#include <basics.h>

#include "gdraw.h"

typedef struct gcol {
    int16 red, green, blue;
    uint32 pixel;
} GCol;

struct revcol /* : GCol */ {
    int16 red, green, blue;
    uint32 index;
    uint8 dist;
    struct revcol *next;
};

struct revitem {
    struct revcol *cols[2];	/* cols[0] => colours in this subcube, cols[1] => those near */
    int16 cnt;
    struct revcmap *sub;
};

struct revcmap {
    int16 range;		/* red_max-red_min+1, total number of colors */
				/*  in the cube along any linear dimension */
    int16 side_cnt;		/* Number of sub-cubes along each linear side side of the cube */
				/* ie. we decimate by a factor of side_cnt, there */
			        /*  will be side_cnt levels of red, and side_cnt^3*/
			        /*  subcubes */
    int16 side_shift;		/* if side_cnt==2^n then this is n */
    int16 div_mul, div_shift, div_add;
				/* tricks for dividing by range/side_cnt */
			        /* We can do (small) integer division by */
			        /*  multiplying by an integer reciprical and */
			        /*  left shifting */
    unsigned int is_grey: 1;
    Color mask;			/* masks off the high order bits that this revclut handles, leaving us with those bits of interest to the subclut */
    struct revitem *cube;
    GCol *greys;		/* 256 entries, if set */
};

typedef struct {		/* normal 16 bit characters are two bytes */
    unsigned char byte1;
    unsigned char byte2;
} GChar2b;

struct gchr_transform {
    uint32 oldstate;
    uint32 newstate;
    unichar_t resch;
};

struct gchr_lookup {
    int cnt;
    struct gchr_transform *transtab;
};

struct gchr_accents {
    unichar_t accent;
    uint32 mask;
};

enum text_funcs { tf_width, tf_drawit, tf_rect, tf_stopat, tf_stopbefore, tf_stopafter };
struct tf_arg { GTextBounds size; int width, maxwidth; unichar_t *last; char *utf8_last; int first; int dont_replace; };

struct gwindow {
    GGC *ggc;
    GDisplay *display;
    int (*eh)(GWindow,GEvent *);
    GRect pos;
    GWindow parent;
    void *user_data;
    struct gwidgetdata *widget_data;
    void *native_window;
    unsigned int is_visible: 1;
    unsigned int is_pixmap: 1;
    unsigned int is_toplevel: 1;
    unsigned int visible_request: 1;
    unsigned int is_dying: 1;
    unsigned int is_popup: 1;
    unsigned int disable_expose_requests: 1;
    unsigned int usecairo: 1;		/* use a cairo context -- if meaningful */
    /**
     * This is an optional window type string that is used by the hotkey subsystem.
     * It is a pointer to a constant string which is never to be freeed or NULL.
     * This will be, for example, "CharView" for the glyph editing window.
     */
    char* window_type_name;
};

struct ginput_context {
    GWindow w;
    enum gic_style style;
    void *ic;
    struct ginput_context *next;
};

struct gtimer {
    long time_sec;				/* longs not int32s to match timeval */
    long time_usec;
    int32 repeat_time;				/* 0 => one shot */
    GWindow owner;
    void *userdata;
    struct gtimer *next;
    unsigned int active: 1;
};

typedef struct fd_callback_struct
{
    int    fd;
    void*  udata;
    void (*callback)(int fd, void* udata );
    
} fd_callback_t;

enum { gdisplay_fd_callbacks_size = 64 };

struct gdisplay {
    struct displayfuncs *funcs;
    void *semaphore;				/* To lock the display against multiple threads */
    struct font_state *fontstate;
    int16 res;
    int16 scale_screen_by;			/* When converting screen pixels to printer pixels */
    GWindow groot;
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
    /* display specific data */
};
#define PointToPixel(points,res)		(((points)*(res)+36)/72)
#define PointTenthsToPixel(pointtenths,res)	((((pointtenths)*(res)+36)/72)/10)
#define PixelToPoint(pixels,res)		(((pixels)*72+(res)/2)/(res))
#define PixelToPointTenths(pixels,res)		(((pixels)*720+(res)/2)/(res))

struct font_data;

struct displayfuncs {
    void (*init)(GDisplay *);
    void (*term)(GDisplay *);
    void *(*nativeDisplay)(GDisplay *);

    void (*setDefaultIcon)(GWindow);

    GWindow (*createTopWindow)(GDisplay *, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *);
    GWindow (*createSubWindow)(GWindow, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *);
    GWindow (*createPixmap)(GDisplay *, GWindow similar, uint16 width, uint16 height);
    GWindow (*createBitmap)(GDisplay *, uint16 width, uint16 height, uint8 *data);
    GCursor (*createCursor)(GWindow src, GWindow mask, Color fg, Color bg, int16 x, int16 y);
    void (*destroyWindow)(GWindow);
    void (*destroyCursor)(GDisplay *,GCursor);
    int (*nativeWindowExists)(GDisplay *,void *native_window);
    void (*setZoom)(GWindow,GRect *size,enum gzoom_flags flags);
    void (*setWindowBorder)(GWindow,int width,Color);
    void (*setWindowBackground)(GWindow,Color);
    int (*setDither)(GDisplay *,int);

    void (*reparentWindow)(GWindow,GWindow,int,int);
    void (*setVisible)(GWindow,int);
    void (*move)(GWindow,int32,int32);
    void (*trueMove)(GWindow,int32,int32);
    void (*resize)(GWindow,int32,int32);
    void (*moveResize)(GWindow,int32,int32,int32,int32);
    void (*raise)(GWindow);
    void (*raiseAbove)(GWindow,GWindow);
    int  (*isAbove)(GWindow,GWindow);
    void (*lower)(GWindow);
    void (*setWindowTitles)(GWindow, const unichar_t *title, const unichar_t *icontitle);
    void (*setWindowTitles8)(GWindow, const char *title, const char *icontitle);
    unichar_t *(*getWindowTitle)(GWindow);
    char *(*getWindowTitle8)(GWindow);
    void (*setTransientFor)(GWindow, GWindow);
    void (*getPointerPos)(GWindow,GEvent *);
    GWindow (*getPointerWindow)(GWindow);
    void (*setCursor)(GWindow, GCursor);
    GCursor (*getCursor)(GWindow);
    GWindow (*getRedirectWindow)(GDisplay *gd);
    void (*translateCoordinates)(GWindow from, GWindow to, GPoint *pt);

    void (*beep)(GDisplay *);
    void (*flush)(GDisplay *);

    void (*pushClip)(GWindow, GRect *rct, GRect *old);
    void (*popClip)(GWindow, GRect *old);

    void (*setDifferenceMode)(GWindow);

    void (*clear)(GWindow,GRect *);
    void (*drawLine)(GWindow, int32 x,int32 y, int32 xend,int32 yend, Color col);
    void (*drawArrow)(GWindow, int32 x,int32 y, int32 xend,int32 yend, int16 arrows, Color col); /* arrows&1 => arrow at start, &2 => at end */
    void (*drawRect)(GWindow, GRect *rect, Color col);
    void (*fillRect)(GWindow, GRect *rect, Color col);
    void (*fillRoundRect)(GWindow, GRect *rect, int radius, Color col);
    void (*drawElipse)(GWindow, GRect *rect, Color col);
    void (*fillElipse)(GWindow, GRect *rect, Color col);
    void (*drawArc)(GWindow, GRect *rect, int32 sangle, int32 eangle, Color col);
    void (*drawPoly)(GWindow, GPoint *pts, int16 cnt, Color col);
    void (*fillPoly)(GWindow, GPoint *pts, int16 cnt, Color col);
    void (*scroll)(GWindow, GRect *rect, int32 hor, int32 vert);

    void (*drawImage)(GWindow, GImage *, GRect *src, int32 x, int32 y);
    void (*tileImage)(GWindow, GImage *, GRect *src, int32 x, int32 y);
    void (*drawGlyph)(GWindow, GImage *, GRect *src, int32 x, int32 y);
    void (*drawImageMag)(GWindow, GImage *, GRect *src, int32 x, int32 y, int32 width, int32 height);
    GImage *(*copyScreenToImage)(GWindow, GRect *rect);
    void (*drawPixmap)(GWindow, GWindow, GRect *src, int32 x, int32 y);
    void (*tilePixmap)(GWindow, GWindow, GRect *src, int32 x, int32 y);

    GIC *(*createInputContext)(GWindow, enum gic_style);
    void (*setGIC)(GWindow, GIC *, int x, int y);
    int (*keyState)(GWindow w, int keysym);

    void (*grabSelection)(GWindow w,enum selnames sel);
    void (*addSelectionType)(GWindow w,enum selnames sel,char *type,
	void *data,int32 cnt,int32 unitsize,void *(*gendata)(void *,int32 *len),
	void (*freedata)(void *));
    void *(*requestSelection)(GWindow w,enum selnames sn, char *typename, int32 *len);
    int (*selectionHasType)(GWindow w,enum selnames sn, char *typename);
    void (*bindSelection)(GDisplay *disp,enum selnames sn, char *atomname);
    int (*selectionHasOwner)(GDisplay *disp,enum selnames sn);

    void (*pointerUngrab)(GDisplay *);
    void (*pointerGrab)(GWindow);
    void (*requestExpose)(GWindow,GRect *,int);
    void (*forceUpdate)(GWindow);
    void (*sync)(GDisplay *);
    void (*skipMouseMoveEvents)(GWindow, GEvent *);
    void (*processPendingEvents)(GDisplay *);
    void (*processWindowEvents)(GWindow);
    void (*processOneEvent)(GDisplay *);
    void (*eventLoop)(GDisplay *);
    void (*postEvent)(GEvent *e);
    void (*postDragEvent)(GWindow w,GEvent *mouse,enum event_type et);
    int  (*requestDeviceEvents)(GWindow w,int devcnt,struct gdeveventmask *de);

    GTimer *(*requestTimer)(GWindow w,int32 time_from_now,int32 frequency, void *userdata);
    void (*cancelTimer)(GTimer *timer);

    void (*syncThread)(GDisplay *gd, void (*func)(void *), void *data);

    GWindow (*startJob)(GDisplay *gdisp,void *user_data,GPrinterAttrs *attrs);
    void (*nextPage)(GWindow w);
    int (*endJob)(GWindow w,int cancel);

    void (*getFontMetrics)(GWindow,GFont *,int *,int *,int *);

    enum gcairo_flags (*hasCairo)(GWindow w);

    void (*startNewPath)(GWindow w);
    void (*closePath)(GWindow w);
    void (*moveto)(GWindow w,double x, double y);
    void (*lineto)(GWindow w,double x, double y);
    void (*curveto)(GWindow w, double cx1,double cy1, double cx2,double cy2, double x, double y);
    void (*stroke)(GWindow w, Color col);
    void (*fill)(GWindow w, Color col);
    void (*fillAndStroke)(GWindow w, Color fillcol,Color strokecol);

    void (*layoutInit)(GWindow w, char *text, int cnt, GFont *fi);
    void (*layoutDraw)(GWindow w, int32 x, int32 y, Color fg);
    void (*layoutIndexToPos)(GWindow w, int index, GRect *pos);
    int  (*layoutXYToIndex)(GWindow w, int x, int y);
    void (*layoutExtents)(GWindow w, GRect *size);
    void (*layoutSetWidth)(GWindow w, int width);
    int  (*layoutLineCount)(GWindow w);
    int  (*layoutLineStart)(GWindow w,int line);
    void (*startNewSubPath)(GWindow w);
    int  (*fillRuleSetWinding)(GWindow w);

    int (*doText8)(GWindow w, int32 x, int32 y, const char *text, int32 cnt, Color col, enum text_funcs drawit, struct tf_arg *arg);

    void (*PushClipOnly)(GWindow w);
    void (*ClipPreserve)(GWindow w);
    
};

extern int16 div_tables[257][2]; // in div_tables.c

extern void GDrawIErrorRun(const char *fmt,...);
extern void GDrawIError(const char *fmt,...);

extern void _GXDraw_DestroyDisplay(GDisplay * gdisp);
extern GDisplay *_GXDraw_CreateDisplay(char *displayname,char *programname);
extern void _GGDKDraw_DestroyDisplay(GDisplay *disp);
extern GDisplay *_GGDKDraw_CreateDisplay(char *displayname, char *programname);
extern void _GPSDraw_DestroyDisplay(GDisplay *gdisp);
extern GDisplay *_GPSDraw_CreateDisplay(void);
extern void _GDraw_InitError(GDisplay *);
extern void _GDraw_ComposeChars(GDisplay *gdisp,GEvent *gevent);

extern void _GDraw_getimageclut(struct _GImage *base, struct gcol *clut);
extern const GCol *_GImage_GetIndexedPixel(Color col,RevCMap *rev);
extern const GCol *_GImage_GetIndexedPixelPrecise(Color col,RevCMap *rev);

extern void (*_GDraw_BuildCharHook)(GDisplay *);
extern void (*_GDraw_InsCharHook)(GDisplay *,unichar_t);
#endif
