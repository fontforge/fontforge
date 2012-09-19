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
    unsigned int mykeybuild: 1;
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
    GWindow (*createPixmap)(GDisplay *, uint16 width, uint16 height);
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
};

extern GDisplay *_GXDraw_CreateDisplay(char *displayname,char *programname);
extern GDisplay *_GPSDraw_CreateDisplay(void);
extern void _GDraw_InitError(GDisplay *);
extern void _GDraw_ComposeChars(GDisplay *gdisp,GEvent *gevent);

extern void _GDraw_getimageclut(struct _GImage *base, struct gcol *clut);
extern const GCol *_GImage_GetIndexedPixel(Color col,RevCMap *rev);
extern const GCol *_GImage_GetIndexedPixelPrecise(Color col,RevCMap *rev);

extern void (*_GDraw_BuildCharHook)(GDisplay *);
extern void (*_GDraw_InsCharHook)(GDisplay *,unichar_t);
#endif
