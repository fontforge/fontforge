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
#ifndef _GDRAW_H
#define _GDRAW_H
#include "gimage.h"
#include "charset.h"

enum font_style { fs_none, fs_italic=1, fs_smallcaps=2, fs_condensed=4, fs_extended=8, fs_vertical=16 };
enum font_type { ft_unknown, ft_serif, ft_sans, ft_mono, ft_cursive, ft_max };
enum text_mods { tm_none, tm_upper=1, tm_lower=2, tm_initialcaps=4, tm_showsofthyphen=8 };
enum text_lines { tl_none, tl_under=1, tl_strike=2, tl_over=4, tl_dash=8 };

typedef struct {
    const unichar_t *family_name;	/* may be more than one */
    int16 point_size;			/* negative values are in pixels */
    int16 weight;
    enum font_style style;
    char *utf8_family_name;
} FontRequest;

typedef struct font_instance FontInstance, GFont;
enum gic_style { gic_overspot=2, gic_root=1, gic_hidden=0, gic_orlesser=4, gic_type=3 };
typedef struct ginput_context GIC;

enum draw_func { df_copy, df_xor };

typedef struct ggc {
    struct gwindow *w;
    int32 xor_base;
    Color fg;
    Color bg;
    GRect clip;
    enum draw_func func;
    unsigned int copy_through_sub_windows: 1;
    unsigned int bitmap_col: 1;			/* window is mapped for bitmap */
    int16 skip_len, dash_len;
    int16 line_width;
    int16 ts;
    int32 ts_xoff, ts_yoff;
    int dash_offset;
    GFont *fi;
} GGC;

typedef struct gtextbounds {
    int16 lbearing;		/* of first character */
				/* origin to left edge of first char's raster */
    int32 rbearing;		/* origin to right edge of last char's raster */
    int16 as,ds;		/* maximum ascent and maximum descent */
    				/*  (both numbers will be positive for "g" */
			        /* so total height = as+ds */
    int16 fas, fds;		/* font ascent and descent */
			        /* total width = rbearing-lbearing */
    int32 width;	        /* above are for the bounding rect, not the text */
			        /*  "width" which may be totally different */
} GTextBounds;

enum selnames { sn_primary, sn_clipboard, sn_drag_and_drop, sn_user1, sn_user2, sn_max };
typedef struct gwindow *GWindow;
typedef struct gdisplay GDisplay;
typedef struct gtimer GTimer;

enum keystate_mask { ksm_shift=1, ksm_capslock=2, ksm_control=4, ksm_meta=8,
	ksm_cmdsuse=0x8,
/* Suse X on a Mac maps command to meta. As of Mac 10.2, the command key is 0x10 */
/*  In 10.0 the command key was 0x20 */
	ksm_cmdmacosx=0x10,	/* But not the command key under suse ppc linux*/
	ksm_numlock=0x10,	/* It's numlock on my 386 system */
	ksm_super=0x40,		/* RedHat mask for the key with the windows flag on it */
	ksm_hyper=0x80,
/* Both Suse and Mac OS/X.2 now map option to 0x2000, but under 10.0 it was meta */
/* Under 10.4 it is the meta mask again */
/* Under 10.6 it is 0x2000 again. I wish they'd be consistent */
	ksm_option=0x2000,	/* sometimes */
	ksm_menumask=(ksm_control|ksm_meta|ksm_cmdmacosx|0xf0),

	ksm_button1=(1<<8), ksm_button2=(1<<9), ksm_button3=(1<<10),
	ksm_button4=(1<<11), ksm_button5=(1<<12),
	ksm_buttons=(ksm_button1|ksm_button2|ksm_button3|ksm_button4|ksm_button5)
	};
enum mnemonic_focus { mf_normal, mf_tab, mf_mnemonic, mf_shortcut };

enum event_type { et_noevent = -1, et_char, et_charup, // is 1
		  et_mousemove, et_mousedown, et_mouseup,
		  et_crossing,	/* these four are assumed to be consecutive */
		  et_focus, // is 7
		  et_expose, et_visibility, et_resize, et_timer,
		  et_close/*request by user*/, et_create,
		  et_map, et_destroy/*window being freed*/,
		  et_selclear,
		  et_drag, et_dragout, et_drop,
		  et_lastnativeevent=et_drop,
		  et_controlevent, et_user };

enum visibility_state { vs_unobscured, vs_partially, vs_obscured };

enum et_subtype { et_buttonpress, et_buttonactivate, et_radiochanged,
		  et_listselected, et_listdoubleclick,
		  et_scrollbarchange,
		  et_textchanged, et_textfocuschanged,
		  et_lastsubtype };

enum sb { et_sb_top, et_sb_uppage, et_sb_up, et_sb_left=et_sb_up,
	  et_sb_down, et_sb_right=et_sb_down, et_sb_downpage,
	  et_sb_bottom,
	  et_sb_thumb, et_sb_thumbrelease };

struct sbevent {
    enum sb type;
    int32 pos;
};

typedef struct gevent {
    enum event_type type;
#define _GD_EVT_CHRLEN	10
    GWindow w;
    union {
	struct {
	    char *device;		/* for wacom devices */
	    uint32 time;
	    uint16 state;
	    int16 x,y;
	    uint16 keysym;
	    int16 autorepeat;
	    unichar_t chars[_GD_EVT_CHRLEN];
	} chr;
	struct {
	    char *device;		/* for wacom devices */
	    uint32 time;
	    int16 state;
	    int16 x,y;
	    int16 button;
	    int16 clicks;
	    int32 pressure, xtilt, ytilt, separation;
	} mouse;
	struct {
	    GRect rect;
	} expose;
	struct {
	    enum visibility_state state;
	} visibility;
	struct {
	    GRect size;
	    int16 dx, dy, dwidth, dheight;
	    unsigned int moved: 1;
	    unsigned int sized: 1;
	} resize;
	struct {
	    char *device;		/* for wacom devices */
	    uint32 time;
	    int16 state;
	    int16 x,y;
	    unsigned int entered: 1;
	} crossing;
	struct {
	    unsigned int gained_focus: 1;
	    unsigned int mnemonic_focus: 2;
	} focus;
	struct {
	    unsigned int is_visible: 1;
	} map;
	struct {
	    enum selnames sel;
	} selclear;
	struct {
	    int32 x,y;
	} drag_drop;
	struct {
	    GTimer *timer;
	    void *userdata;
	} timer;
	struct {
	    enum et_subtype subtype;
	    struct ggadget *g;
	    union {
		struct sbevent sb;
		struct {
		    int gained_focus;
		} tf_focus;
		struct {
		    int from_pulldown;	/* -1 normally, else index into pulldown list */
		} tf_changed;
		struct {
		    int clicks;
		    int16 button, state;
		} button;
		struct {
		    int from_mouse, changed_index;
		} list;
	    } u;
	} control;
	struct {
	    long subtype;
	    void *userdata;
	} user;
    } u;
    void *native_window;
} GEvent;

typedef enum cursor_types { ct_default, ct_pointer, ct_backpointer, ct_hand,
	ct_question, ct_cross, ct_4way, ct_text, ct_watch, ct_draganddrop,
	ct_invisible, 
	ct_user, ct_user2 /* and so on */ } GCursor;

enum window_attr_mask { wam_events=0x2, wam_bordwidth=0x4,
			wam_bordcol=0x8, wam_backcol=0x10, wam_cursor=0x20, wam_wtitle=0x40,
			wam_ititle=0x80, wam_icon=0x100, wam_nodecor=0x200,
			wam_positioned=0x400, wam_centered=0x800, wam_undercursor=0x1000,
			wam_noresize=0x2000, wam_restrict=0x4000, wam_redirect=0x8000,
			wam_isdlg=0x10000, wam_notrestricted=0x20000,
			wam_transient=0x40000,
			wam_utf8_wtitle=0x80000, wam_utf8_ititle=0x100000,
			wam_nocairo=0x200000, wam_verytransient=0x400000 };

typedef struct gwindow_attrs {
    enum window_attr_mask mask;
    uint32 event_masks;			/* (1<<et_char) | (1<<et_mouseup) etc */
    int16 border_width;
    Color border_color;			/* Color_UNKNOWN if unspecified */
    Color background_color;
    GCursor cursor;
	/* Remainder is only for top level windows */
    const unichar_t *window_title;
    const unichar_t *icon_title;
    struct gwindow *icon;		/* A bitmap pixmap, or NULL */
    unsigned int nodecoration: 1;	/* no wm decoration */
    unsigned int positioned: 1;		/* position information is important */
    unsigned int centered: 2;		/* center the window on the screen. pos.width&pos.height are used */
    unsigned int undercursor: 1;	/* center the window under the cursor. */
    unsigned int noresize: 1;		/* set min and max sizes to current size */
    unsigned int restrict_input_to_me: 1;/* for dialogs, no input outside of dlg */
    unsigned int redirect_chars_to_me: 1;/* ditto, we get any input outside of us */
    unsigned int is_dlg: 1;		/* 1 for dlg, 0 for main window */
    unsigned int not_restricted: 1;	/* gets events if if a restricted (modal) dlg is up */
    GWindow redirect_from;		/* only redirect input from this window and its children */
    GWindow transient;			/* the Transient_FOR hint */
    const char *utf8_window_title;
    const char *utf8_icon_title;
} GWindowAttrs;

#define GWINDOWATTRS_EMPTY { 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL }


enum printer_attr_mask { pam_pagesize=1, pam_margins=2, pam_scale=4,
			 pam_res=8, pam_copies=0x10, pam_thumbnails=0x20, pam_printername=0x40,
			 pam_filename=0x80, pam_args=0x100, pam_color=0x200, pam_transparent=0x400,
			 pam_lpr=0x800, pam_queue=0x1000, pam_eps=0x2000, pam_landscape=0x4000,
			 pam_title=0x8000 };

enum printer_units { pu_inches, pu_points, pu_mm };

typedef struct gprinter_attrs {
    enum printer_attr_mask mask;
    float width, height;		/* paper size */
    float lmargin, rmargin, tmargin, bmargin;
    float scale;			/* 1.0 implies no scaling */
    enum printer_units units;
    int32 res;				/* printer resolution */
    int16 num_copies;
    int16 thumbnails;			/* linear count of number of thumbnail*/
					/* pages per edge of real page */
    unsigned int do_color: 1;
    unsigned int do_transparent: 1;	/* try to get transparent images to work*/
    unsigned int use_lpr: 1;
    unsigned int donot_queue: 1;	/* ie. print to file */
    unsigned int landscape: 1;
    unsigned int eps: 1;		/* generate an eps file, not a full doc */
    char *printer_name;			/* only if things are queued */
    char *file_name;			/* only if things aren't queued */
    char *extra_lpr_args;
    unichar_t *title;
    uint16 start_page, end_page;	/* Ignored by printer routines, for programmer */
} GPrinterAttrs;

typedef struct gdeveventmask {
    int event_mask;
    char *device_name;
} GDevEventMask;

enum gzoom_flags { gzf_pos=1, gzf_size=2 };
    /* bit flags for the hasCairo query */
enum gcairo_flags { gc_buildpath=1,	/* Has build path commands (postscript, cairo) */
		    gc_alpha=2,		/* Supports alpha channels & translucent colors (cairo, pdf) */
		    gc_xor=4,		/* Cairo can't do the traditional XOR drawing that X11 does */
		    gc_all = gc_buildpath|gc_alpha
		    };

typedef int (*GDrawEH)(GWindow,GEvent *);

extern unichar_t *GDrawKeysyms[];
extern GDisplay *screen_display, *printer_display;

extern void GDrawDestroyDisplays();
extern void GDrawCreateDisplays(char *displayname,char *programname);
extern void *GDrawNativeDisplay(GDisplay *);
extern void GDrawTerm(GDisplay *disp);

extern int GDrawGetRes(GWindow gw);
extern int GDrawPointsToPixels(GWindow gw,int points);
extern int GDrawPixelsToPoints(GWindow gw,int pixels);

extern void GDrawSetDefaultIcon(GWindow icon);
extern GWindow GDrawCreateTopWindow(GDisplay *gdisp, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs);
extern GWindow GDrawCreateSubWindow(GWindow w, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs);
extern GWindow GDrawCreatePixmap(GDisplay *gdisp, uint16 width, uint16 height);
extern GWindow GDrawCreateBitmap(GDisplay *gdisp, uint16 width, uint16 height, uint8 *data);
extern GCursor GDrawCreateCursor(GWindow src,GWindow mask,Color fg,Color bg,
	int16 x, int16 y );
extern void GDrawDestroyWindow(GWindow w);
extern void GDrawDestroyCursor(GDisplay *gdisp, GCursor ct);
extern int  GDrawNativeWindowExists(GDisplay *gdisp, void *native);
extern void GDrawSetZoom(GWindow w, GRect *zoomsize, enum gzoom_flags);
extern void GDrawSetWindowBorder(GWindow w, int width, Color color);
extern void GDrawSetWindowBackground(GWindow w, Color color);

/**
 * Set the window type to the given name.
 *
 * You should not free the 'name' string, it is assumed to exist for
 * the lifetime of the fontforge run, for example, as a constant
 * string. No copy of name is performed.
 */
extern void  GDrawSetWindowTypeName(GWindow w, char* name);
/**
 * Get the window type string that was set with GDrawSetWindowTypeName()
 * or NULL if no such string was set.
 *
 * No memory allocations are performed. You should not free the string
 * that is returned.
 */
extern char* GDrawGetWindowTypeName(GWindow w);
extern int  GDrawSetDither(GDisplay *gdisp, int dither);
extern void GDrawReparentWindow(GWindow child,GWindow newparent, int x,int y);
extern void GDrawSetVisible(GWindow w, int visible);
extern int  GDrawIsVisible(GWindow w);
extern void GDrawTrueMove(GWindow w, int32 x, int32 y);
extern void GDrawMove(GWindow w, int32 x, int32 y);
extern void GDrawResize(GWindow w, int32 width, int32 height);
extern void GDrawMoveResize(GWindow w, int32 x, int32 y, int32 width, int32 height);
extern GWindow GDrawGetRoot(GDisplay *);
extern Color GDrawGetDefaultBackground(GDisplay *);
extern Color GDrawGetDefaultForeground(GDisplay *);
extern GRect *GDrawGetSize(GWindow w, GRect *ret);
extern GDrawEH GDrawGetEH(GWindow w);
extern void GDrawSetEH(GWindow w,GDrawEH e_h);
extern void GDrawGetPointerPosition(GWindow w, GEvent *mouse);
extern GWindow GDrawGetPointerWindow(GWindow w);
extern void GDrawRaise(GWindow w);
extern void GDrawRaiseAbove(GWindow w,GWindow below);
extern int  GDrawIsAbove(GWindow w,GWindow other);
extern void GDrawLower(GWindow w);
extern void GDrawSetWindowTitles(GWindow w, const unichar_t *title, const unichar_t *icontit);
extern void GDrawSetWindowTitles8(GWindow w, const char *title, const char *icontit);
extern unichar_t *GDrawGetWindowTitle(GWindow w);
extern char *GDrawGetWindowTitle8(GWindow w);
extern void GDrawSetTransientFor(GWindow transient,GWindow owner);
extern void GDrawSetCursor(GWindow w, GCursor ct);
extern GCursor GDrawGetCursor(GWindow w);
extern GWindow GDrawGetRedirectWindow(GDisplay *gd);
extern GWindow GDrawGetParentWindow(GWindow gw);
extern int GDrawWindowIsAncestor(GWindow ancester, GWindow descendent);
extern void GDrawSetUserData(GWindow gw, void *ud);
extern void *GDrawGetUserData(GWindow gw);
extern GDisplay *GDrawGetDisplayOfWindow(GWindow);
extern void GDrawTranslateCoordinates(GWindow from,GWindow to, GPoint *pt);
extern int32 GDrawEventInWindow(GWindow inme,GEvent *event);
extern void GDrawBeep(GDisplay *gdisp);
extern void GDrawFlush(GDisplay *gdisp);

extern void GDrawGetClip(GWindow w, GRect *ret);
extern void GDrawSetClip(GWindow w, GRect *rct);
extern void GDrawPushClip(GWindow w, GRect *rct, GRect *old);
extern void GDrawPopClip(GWindow w, GRect *old);
extern void GDrawPushClipOnly(GWindow w);
extern void GDrawClipPreserve(GWindow w);
extern GGC *GDrawGetWindowGGC(GWindow w);
extern void GDrawSetXORBase(GWindow w,Color col);
extern void GDrawSetXORMode(GWindow w);
extern void GDrawSetCopyMode(GWindow w);
extern void GDrawSetCopyThroughSubWindows(GWindow w,int16 through);
extern void GDrawSetDashedLine(GWindow w,int16 dash_len, int16 skip_len, int16 off);
extern void GDrawSetStippled(GWindow w,int16 ts, int32 yoff,int32 xoff);
extern void GDrawSetLineWidth(GWindow w,int16 width);
extern int16 GDrawGetLineWidth( GWindow w );

extern void GDrawSetForeground(GWindow w,Color col);
extern void GDrawSetBackground(GWindow w,Color col);

extern GFont *GDrawSetFont(GWindow gw, GFont *fi);
extern GFont *GDrawInstanciateFont(GWindow gw, FontRequest *rq);
extern GFont *GDrawAttachFont(GWindow gw, FontRequest *rq);
extern FontRequest *GDrawDecomposeFont(GFont *fi, FontRequest *rq);
extern void GDrawWindowFontMetrics(GWindow gw,GFont *fi,int *as, int *ds, int *ld);

extern int32 GDrawGetTextBounds(GWindow gw,const unichar_t *text, int32 cnt, GTextBounds *size);
extern int32 GDrawGetTextWidth(GWindow gw, const unichar_t *text, int32 cnt);
extern int32 GDrawDrawText(GWindow gw, int32 x, int32 y, const unichar_t *txt, int32 cnt, Color col);
/* UTF8 routines */
extern int32 GDrawGetText8Bounds(GWindow gw, const char *text, int32 cnt, GTextBounds *size);
extern int32 GDrawGetText8Width(GWindow gw, const char *text, int32 cnt);
extern int32 GDrawGetText8Height(GWindow gw, const char *text, int32 cnt);
extern int32 GDrawDrawText8(GWindow gw, int32 x, int32 y, const char *txt, int32 cnt, Color col);

extern GIC *GDrawCreateInputContext(GWindow w,enum gic_style def_style);
extern void GDrawSetGIC(GWindow w,GIC *gic,int x, int y);

extern void GDrawClear(GWindow w, GRect *rect);
extern void GDrawDrawLine(GWindow w, int32 x,int32 y, int32 xend,int32 yend, Color col);
extern void GDrawDrawArrow(GWindow w, int32 x,int32 y, int32 xend,int32 yend, int arrows, Color col);
extern void GDrawDrawRect(GWindow w, GRect *rect, Color col);
extern void GDrawFillRect(GWindow w, GRect *rect, Color col);
extern void GDrawFillRoundRect(GWindow w, GRect *rect, int radius, Color col);
extern void GDrawDrawElipse(GWindow w, GRect *rect, Color col);
extern void GDrawFillElipse(GWindow w, GRect *rect, Color col);
extern void GDrawDrawArc(GWindow w, GRect *rect, int32 sangle, int32 tangle, Color col);
extern void GDrawDrawPoly(GWindow w, GPoint *pts, int16 cnt, Color col);
extern void GDrawFillPoly(GWindow w, GPoint *pts, int16 cnt, Color col);
extern void GDrawScroll(GWindow w, GRect *rect, int32 hor, int32 vert);
extern void GDrawDrawImage(GWindow w, GImage *img, GRect *src, int32 x, int32 y);
extern void GDrawDrawGlyph(GWindow w, GImage *img, GRect *src, int32 x, int32 y);
extern void GDrawDrawScaledImage(GWindow w, GImage *img, int32 x, int32 y);
extern void GDrawDrawImageMagnified(GWindow w, GImage *img, GRect *src, int32 x, int32 y,
	int32 width, int32 height);
extern void GDrawTileImage(GWindow w, GImage *img, GRect *src, int32 x, int32 y);
extern void GDrawDrawPixmap(GWindow w, GWindow pixmap, GRect *src, int32 x, int32 y);
extern void GDrawTilePixmap(GWindow w, GWindow pixmap, GRect *src, int32 x, int32 y);
extern GImage *GDrawCopyScreenToImage(GWindow w, GRect *rect);

extern void GDrawGrabSelection(GWindow w,enum selnames sel);
extern void GDrawAddSelectionType(GWindow w,enum selnames sel,char *type,
	void *data,int32 cnt,int32 unitsize,void *(*gendata)(void *,int32 *len),
	void (*freedata)(void *));
extern void *GDrawRequestSelection(GWindow w,enum selnames sn, char *typename, int32 *len);
extern int GDrawSelectionHasType(GWindow w,enum selnames sn, char *typename);
extern void GDrawBindSelection(GDisplay *disp,enum selnames sel, char *atomname);
extern int GDrawSelectionOwned(GDisplay *disp,enum selnames sel);

extern void GDrawPointerUngrab(GDisplay *disp);
extern void GDrawPointerGrab(GWindow w);
extern int GDrawEnableExposeRequests(GWindow w,int enabled);
extern void GDrawRequestExpose(GWindow w, GRect *rect, int doclear);
extern void GDrawSync(GDisplay *gdisp);
extern void GDrawForceUpdate(GWindow w);
extern void GDrawProcessOneEvent(GDisplay *disp);
extern void GDrawProcessPendingEvents(GDisplay *disp);
extern void GDrawProcessWindowEvents(GWindow w);
extern void GDrawSkipMouseMoveEvents(GWindow w,GEvent *last);
extern void GDrawEventLoop(GDisplay *disp);
extern void GDrawPostEvent(GEvent *e);
extern void GDrawPostDragEvent(GWindow gw,GEvent *e,enum event_type);

extern GTimer *GDrawRequestTimer(GWindow w,int32 time_from_now,int32 frequency,
	void *userdata);
extern void GDrawCancelTimer(GTimer *timer);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Windowless timers used for background activities
//

/**
 * Callback which will be called at a nominated frequency with a given
 * userdata pointer.
 */
typedef void (* BackgroundTimerFunc )(void*);

/**
 * Internal bookkeeping for windowless timers. They are currently
 * windowed timers on the inside but we spare the user from creating
 * the window, keeping track of it, and having to deal with an event
 * handling function which tries to get back a nominated userdata
 * pointer from all that jazz.
 */
typedef struct BackgroundTimerstruct
{
    // takes userdata as arg1
    BackgroundTimerFunc func;

    // the userdata pointer that should be passed to func
    void *userdata;

    // the internal hidden window used to actually get the timer events
    GWindow w;

    // the GDraw timer associated with the above window
    GTimer* timer;

    // how often to fire the timer
    int32 BackgroundTimerMS;
} BackgroundTimer_t;

/**
 * Create a new windowless timer which will be fired every
 * BackgroundTimerMS milliseconds and call func with the supplied
 * userdata.
 */
BackgroundTimer_t*
BackgroundTimer_new( int32 BackgroundTimerMS, 
		     BackgroundTimerFunc func,
		     void *userdata );

/**
 * Remove a windowless background timer freeing any resources
 * associated with it.
 */
void BackgroundTimer_remove( BackgroundTimer_t* t );

/**
 * Make sure the timer fires at it's desired time from now(). For
 * example, if a timer will fire every 2 seconds and is about to fire
 * in a few ms from now, calling touch() will make it fire 2 seconds
 * from now instead.
 *
 * This way if you have a timer which is to handle background issues
 * if something doesn't happen in a scheduled amount of time, you can
 * touch() the timer to make sure it fires again after the full
 * elapsed time instead of having it fire too soon.
 */
void BackgroundTimer_touch( BackgroundTimer_t* t );

////////////////////////////////////////////////////////////////////////////////


extern void GDrawSyncThread(GDisplay *gd, void (*func)(void *), void *data);

extern GWindow GPrinterStartJob(GDisplay *gdisp,void *user_data,GPrinterAttrs *attrs);
extern void GPrinterNextPage(GWindow w);
extern int  GPrinterEndJob(GWindow w,int cancel);

extern void GDrawSetBuildCharHooks(void (*hook)(GDisplay *), void (*inshook)(GDisplay *,unichar_t));

extern int GDrawRequestDeviceEvents(GWindow w,int devcnt,struct gdeveventmask *de);

extern enum gcairo_flags GDrawHasCairo(GWindow w);
extern void GDrawQueueDrawing(GWindow w,void (*)(GWindow,void *),void *);
extern void GDrawPathStartNew(GWindow w);
extern void GDrawPathStartSubNew(GWindow w);
extern int GDrawFillRuleSetWinding(GWindow w);
extern void GDrawPathClose(GWindow w);
extern void GDrawPathMoveTo(GWindow w,double x, double y);
extern void GDrawPathLineTo(GWindow w,double x, double y);
extern void GDrawPathCurveTo(GWindow w,
		    double cx1, double cy1,
		    double cx2, double cy2,
		    double x, double y);
extern void GDrawPathStroke(GWindow w,Color col);
extern void GDrawPathFill(GWindow w,Color col);
extern void GDrawPathFillAndStroke(GWindow w,Color fillcol, Color strokecol);
extern void GDrawEnableCairo(int on);

extern void GDrawLayoutInit(GWindow w, char *text, int cnt, GFont *fi);
extern void GDrawLayoutDraw(GWindow w, int32 x, int32 y, Color fg);
extern void GDrawLayoutIndexToPos(GWindow w, int index, GRect *pos);
extern int  GDrawLayoutXYToIndex(GWindow w, int x, int y);
extern void GDrawLayoutExtents(GWindow w, GRect *size);
extern void GDrawLayoutSetWidth(GWindow w, int width);
extern int  GDrawLayoutLineCount(GWindow w);
extern int  GDrawLayoutLineStart(GWindow w,int line);

extern void GDrawFatalError(const char *fmt,...);
extern void GDrawIError(const char *fmt,...);
extern void GDrawError(const char *fmt,...);
extern int GDrawKeyState(int keysym);

extern int GImageGetScaledWidth(GWindow gw, GImage *img);
extern int GImageGetScaledHeight(GWindow gw, GImage *img);


/* extern void setZeroMQReadFD( GDisplay *disp, */
/* 			     int zeromq_fd, void* zeromq_datas, */
/* 			     void (*zeromq_fd_callback)(int zeromq_fd, void* datas )); */

extern void GDrawAddReadFD( GDisplay *disp,
			    int fd, void* udata,
			    void (*callback)(int fd, void* udata ));
extern void GDrawRemoveReadFD( GDisplay *disp,
			       int fd, void* udata );

/**
 * The Mac OSX build doesn't use the same core event loop as the
 * Linux/X build. So inside the timer we can use this to double check
 * if any fds that we should monitor for input have changed and if so
 * service their messages.
 */
extern void MacServiceReadFDs(void);

#endif
