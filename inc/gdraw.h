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

#ifndef FONTFORGE_GDRAW_H
#define FONTFORGE_GDRAW_H

#include "gimage.h"
#include "gresource.h"

typedef struct font_instance FontInstance, GFont;
enum gic_style { gic_overspot=2, gic_root=1, gic_hidden=0, gic_orlesser=4, gic_type=3 };
typedef struct ginput_context GIC;

typedef struct ggc {
    struct gwindow *w;
    Color fg;
    Color bg;
    GRect clip;
    unsigned int bitmap_col: 1;			/* window is mapped for bitmap */
    int16_t skip_len, dash_len;
    int16_t line_width;
    int16_t ts;
    int32_t ts_xoff, ts_yoff;
    int dash_offset;
    GFont *fi;
} GGC;

typedef struct gtextbounds {
    int16_t lbearing;		/* of first character */
				/* origin to left edge of first char's raster */
    int32_t rbearing;		/* origin to right edge of last char's raster */
    int16_t as,ds;		/* maximum ascent and maximum descent */
    				/*  (both numbers will be positive for "g" */
			        /* so total height = as+ds */
    int16_t fas, fds;		/* font ascent and descent */
			        /* total width = rbearing-lbearing */
    int32_t width;	        /* above are for the bounding rect, not the text */
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
		  et_save, et_lastsubtype };

enum sb { et_sb_top, et_sb_uppage, et_sb_up, et_sb_left=et_sb_up,
	  et_sb_down, et_sb_right=et_sb_down, et_sb_downpage,
	  et_sb_bottom,
	  et_sb_thumb, et_sb_thumbrelease, et_sb_halfup, et_sb_halfdown };

struct sbevent {
    enum sb type;
    int32_t pos;
};

typedef struct gevent {
    enum event_type type;
#define _GD_EVT_CHRLEN	10
    GWindow w;
    union {
	struct {
	    char *device;		/* for wacom devices */
	    uint32_t time;
	    uint16_t state;
	    int16_t x,y;
	    uint16_t keysym;
	    int16_t autorepeat;
	    unichar_t chars[_GD_EVT_CHRLEN];
	} chr;
	struct {
	    char *device;		/* for wacom devices */
	    uint32_t time;
	    int16_t state;
	    int16_t x,y;
	    int16_t button;
	    int16_t clicks;
	    int32_t pressure, xtilt, ytilt, separation;
	} mouse;
	struct {
	    GRect rect;
	} expose;
	struct {
	    enum visibility_state state;
	} visibility;
	struct {
	    GRect size;
	    int16_t dx, dy, dwidth, dheight;
	    unsigned int moved: 1;
	    unsigned int sized: 1;
	} resize;
	struct {
	    char *device;		/* for wacom devices */
	    uint32_t time;
	    int16_t state;
	    int16_t x,y;
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
	    int32_t x,y;
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
		    int16_t button, state;
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
			wam_nocairo=0x200000, wam_verytransient=0x400000, wam_palette=0x800000 };

typedef struct gwindow_attrs {
    enum window_attr_mask mask;
    uint32_t event_masks;			/* (1<<et_char) | (1<<et_mouseup) etc */
    int16_t border_width;
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

typedef struct gdeveventmask {
    int event_mask;
    char *device_name;
} GDevEventMask;

enum gzoom_flags { gzf_pos=1, gzf_size=2 };

typedef int (*GDrawEH)(GWindow,GEvent *);

extern Color _GDraw_res_fg, _GDraw_res_bg, _GDraw_res_warnfg;
extern int _GDraw_res_res, _GDraw_res_multiclicktime, _GDraw_res_multiclickwiggle;
extern int _GDraw_res_selnottime, _GDraw_res_twobuttonfixup, _GDraw_res_macosxcmd;
extern int _GDraw_res_synchronize;

extern unichar_t *GDrawKeysyms[];
extern GDisplay *screen_display;

extern void GDrawResourceFind(void);

extern void GDrawDestroyDisplays(void);
extern void GDrawCreateDisplays(char *displayname,char *programname);

extern int GDrawPointsToPixels(GWindow gw,int points);
extern int GDrawPixelsToPoints(GWindow gw,int pixels);

extern void GDrawSetDefaultIcon(GWindow icon);
extern GWindow GDrawCreateTopWindow(GDisplay *gdisp, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs);
extern GWindow GDrawCreateSubWindow(GWindow w, GRect *pos, int (*eh)(GWindow,GEvent *), void *user_data, GWindowAttrs *wattrs);
extern GWindow GDrawCreatePixmap(GDisplay *gdisp, GWindow similar, uint16_t width, uint16_t height);
extern GWindow GDrawCreateBitmap(GDisplay *gdisp, uint16_t width, uint16_t height, uint8_t *data);
extern GCursor GDrawCreateCursor(GWindow src,GWindow mask,Color fg,Color bg,
	int16_t x, int16_t y );
extern void GDrawDestroyWindow(GWindow w);
extern int  GDrawNativeWindowExists(GDisplay *gdisp, void *native);
extern void GDrawSetZoom(GWindow w, GRect *zoomsize, enum gzoom_flags);
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
extern void GDrawSetVisible(GWindow w, int visible);
extern int  GDrawIsVisible(GWindow w);
extern void GDrawTrueMove(GWindow w, int32_t x, int32_t y);
extern void GDrawMove(GWindow w, int32_t x, int32_t y);
extern void GDrawResize(GWindow w, int32_t width, int32_t height);
extern void GDrawMoveResize(GWindow w, int32_t x, int32_t y, int32_t width, int32_t height);
extern GWindow GDrawGetRoot(GDisplay *);
extern Color GDrawGetDefaultBackground(GDisplay *);
extern Color GDrawGetDefaultForeground(GDisplay *);
extern Color GDrawGetWarningForeground(GDisplay *);
extern GRect *GDrawGetSize(GWindow w, GRect *ret);
extern GDrawEH GDrawGetEH(GWindow w);
extern void GDrawSetEH(GWindow w,GDrawEH e_h);
extern void GDrawGetPointerPosition(GWindow w, GEvent *mouse);
extern GWindow GDrawGetPointerWindow(GWindow w);
extern void GDrawRaise(GWindow w);
extern void GDrawSetWindowTitles8(GWindow w, const char *title, const char *icontit);
extern char *GDrawGetWindowTitle8(GWindow w);
extern void GDrawSetTransientFor(GWindow transient,GWindow owner);
extern void GDrawSetCursor(GWindow w, GCursor ct);
extern GCursor GDrawGetCursor(GWindow w);
extern GWindow GDrawGetParentWindow(GWindow gw);
extern int GDrawWindowIsAncestor(GWindow ancester, GWindow descendent);
extern void GDrawSetUserData(GWindow gw, void *ud);
extern void *GDrawGetUserData(GWindow gw);
extern GDisplay *GDrawGetDisplayOfWindow(GWindow);
extern void GDrawTranslateCoordinates(GWindow from,GWindow to, GPoint *pt);
extern int32_t GDrawEventInWindow(GWindow inme,GEvent *event);
extern void GDrawBeep(GDisplay *gdisp);
extern void GDrawFlush(GDisplay *gdisp);

extern bool GDrawClipContains(const GWindow w, const GRect *other, bool rev);
extern bool GDrawClipOverlaps(const GWindow w, const GRect *other);
extern void GDrawGetClip(GWindow w, GRect *ret);
extern void GDrawPushClip(GWindow w, GRect *rct, GRect *old);
extern void GDrawPopClip(GWindow w, GRect *old);
extern void GDrawPushClipOnly(GWindow w);
extern void GDrawClipPreserve(GWindow w);
extern void GDrawSetDashedLine(GWindow w,int16_t dash_len, int16_t skip_len, int16_t off);
extern void GDrawSetStippled(GWindow w,int16_t ts, int32_t yoff,int32_t xoff);
extern void GDrawSetLineWidth(GWindow w,int16_t width);
extern int16_t GDrawGetLineWidth( GWindow w );

extern void GDrawSetBackground(GWindow w,Color col);

extern GFont *GDrawSetFont(GWindow gw, GFont *fi);
extern GFont *GDrawSetDefaultFont(GWindow gw);
extern GFont *GDrawInstanciateFont(GWindow gw, FontRequest *rq);
extern GFont *GDrawAttachFont(GWindow gw, FontRequest *rq);
extern FontRequest *GDrawDecomposeFont(GFont *fi, FontRequest *rq);
extern void GDrawWindowFontMetrics(GWindow gw,GFont *fi,int *as, int *ds, int *ld);
extern void GDrawDefaultFontMetrics(GWindow gw,int *as, int *ds, int *ld);

extern int32_t GDrawGetTextBounds(GWindow gw,const unichar_t *text, int32_t cnt, GTextBounds *size);
extern int32_t GDrawGetTextWidth(GWindow gw, const unichar_t *text, int32_t cnt);
extern int32_t GDrawDrawText(GWindow gw, int32_t x, int32_t y, const unichar_t *txt, int32_t cnt, Color col);
/* UTF8 routines */
extern int32_t GDrawGetText8Bounds(GWindow gw, const char *text, int32_t cnt, GTextBounds *size);
extern int32_t GDrawGetText8Width(GWindow gw, const char *text, int32_t cnt);
extern int32_t GDrawGetText8Height(GWindow gw, const char *text, int32_t cnt);
extern int32_t GDrawDrawText8(GWindow gw, int32_t x, int32_t y, const char *txt, int32_t cnt, Color col);

extern GIC *GDrawCreateInputContext(GWindow w,enum gic_style def_style);
extern void GDrawSetGIC(GWindow w,GIC *gic,int x, int y);
extern int GDrawKeyState(GWindow w, int keysym);

extern void GDrawDrawLine(GWindow w, int32_t x,int32_t y, int32_t xend,int32_t yend, Color col);
extern void GDrawDrawArrow(GWindow w, int32_t x,int32_t y, int32_t xend,int32_t yend, int arrows, Color col);
extern void GDrawDrawRect(GWindow w, GRect *rect, Color col);
extern void GDrawFillRect(GWindow w, GRect *rect, Color col);
extern void GDrawFillRoundRect(GWindow w, GRect *rect, int radius, Color col);
extern void GDrawDrawElipse(GWindow w, GRect *rect, Color col);
extern void GDrawFillElipse(GWindow w, GRect *rect, Color col);
extern void GDrawDrawArc(GWindow w, GRect *rect, int32_t sangle, int32_t tangle, Color col);
extern void GDrawDrawPoly(GWindow w, GPoint *pts, int16_t cnt, Color col);
extern void GDrawFillPoly(GWindow w, GPoint *pts, int16_t cnt, Color col);
extern void GDrawScroll(GWindow w, GRect *rect, int32_t hor, int32_t vert);
extern void GDrawDrawImage(GWindow w, GImage *img, GRect *src, int32_t x, int32_t y);
extern void GDrawDrawGlyph(GWindow w, GImage *img, GRect *src, int32_t x, int32_t y);
extern void GDrawDrawScaledImage(GWindow w, GImage *img, int32_t x, int32_t y);
extern void GDrawDrawImageMagnified(GWindow w, GImage *img, GRect *src, int32_t x, int32_t y,
	int32_t width, int32_t height);
extern void GDrawDrawPixmap(GWindow w, GWindow pixmap, GRect *src, int32_t x, int32_t y);

extern void GDrawGrabSelection(GWindow w,enum selnames sel);
extern void GDrawAddSelectionType(GWindow w,enum selnames sel,char *type,
	void *data,int32_t cnt,int32_t unitsize,void *(*gendata)(void *,int32_t *len),
	void (*freedata)(void *));
extern void *GDrawRequestSelection(GWindow w,enum selnames sn, char *type_name, int32_t *len);
extern int GDrawSelectionHasType(GWindow w,enum selnames sn, char *type_name);
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
extern void GDrawSkipMouseMoveEvents(GWindow w,GEvent *last);
extern void GDrawEventLoop(GDisplay *disp);
extern void GDrawPostEvent(GEvent *e);
extern void GDrawPostDragEvent(GWindow gw,GEvent *e,enum event_type);

extern GTimer *GDrawRequestTimer(GWindow w,int32_t time_from_now,int32_t frequency,
	void *userdata);
extern void GDrawCancelTimer(GTimer *timer);

extern int GDrawRequestDeviceEvents(GWindow w,int devcnt,struct gdeveventmask *de);
extern int GDrawShortcutKeyMatches(const GEvent *e, unichar_t ch);

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

extern void GDrawLayoutInit(GWindow w, char *text, int cnt, GFont *fi);
extern void GDrawLayoutDraw(GWindow w, int32_t x, int32_t y, Color fg);
extern void GDrawLayoutIndexToPos(GWindow w, int index, GRect *pos);
extern int  GDrawLayoutXYToIndex(GWindow w, int x, int y);
extern void GDrawLayoutExtents(GWindow w, GRect *size);
extern void GDrawLayoutSetWidth(GWindow w, int width);
extern int  GDrawLayoutLineCount(GWindow w);
extern int  GDrawLayoutLineStart(GWindow w,int line);

extern void GDrawFatalError(const char *fmt,...);
extern void GDrawIError(const char *fmt,...);
extern void GDrawError(const char *fmt,...);

extern int GImageGetScaledWidth(GWindow gw, GImage *img);
extern int GImageGetScaledHeight(GWindow gw, GImage *img);

#endif /* FONTFORGE_GDRAW_H */
