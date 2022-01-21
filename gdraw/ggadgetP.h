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

#ifndef FONTFORGE_GGADGET_P_H
#define FONTFORGE_GGADGET_P_H

#include "ggadget.h"
#include "gresedit.h"

struct gfuncs {
    unsigned int is_widget: 1;
    uint16_t size;
    int (*handle_expose)(GWindow pixmap,GGadget *g,GEvent *event);
    int (*handle_mouse)(GGadget *g,GEvent *event);
    int (*handle_key)(GGadget *g,GEvent *event);
    int (*handle_editcmd)(GGadget *g,enum editor_commands);
    int (*handle_focus)(GGadget *g,GEvent *event);
    int (*handle_timer)(GGadget *g,GEvent *event);
    int (*handle_sel)(GGadget *g,GEvent *event);

    void (*redraw)(GGadget *g);
    void (*move)(GGadget *g,int32_t x, int32_t y);
    void (*resize)(GGadget *g,int32_t width, int32_t height);
    void (*setvisible)(GGadget *g,int);
    void (*setenabled)(GGadget *g,int);

    GRect *(*getsize)(GGadget *g,GRect *);
    GRect *(*getinnersize)(GGadget *g,GRect *);

    void (*destroy)(GGadget *g);

    void (*set_title)(GGadget *g,const unichar_t *str);
    const unichar_t *(*_get_title)(GGadget *g);
    unichar_t *(*get_title)(GGadget *g);
    void (*set_imagetitle)(GGadget *g,GImage *,const unichar_t *str,int before);
    GImage *(*get_image)(GGadget *g);

    void (*set_font)(GGadget *g,GFont *);
    GFont *(*get_font)(GGadget *g);

    void (*clear_list)(GGadget *g);
    void (*set_list)(GGadget *g, GTextInfo **ti, int32_t copyit);
    GTextInfo **(*get_list)(GGadget *g,int32_t *len);
    GTextInfo *(*get_list_item)(GGadget *g,int32_t pos);
    void (*select_list_item)(GGadget *g,int32_t pos, int32_t sel);
    void (*select_one_list_item)(GGadget *g,int32_t pos);
    int32_t (*is_list_item_selected)(GGadget *g,int32_t pos);
    int32_t (*get_first_selection)(GGadget *g);
    void (*scroll_list_to_pos)(GGadget *g,int32_t pos);
    void (*scroll_list_to_text)(GGadget *g,const unichar_t *lab,int32_t sel);
    void (*set_list_orderer)(GGadget *g,int (*orderer)(const void *, const void *));

    void (*get_desired_size)(GGadget *g, GRect *outer, GRect *inner);
    void (*set_desired_size)(GGadget *g, GRect *outer, GRect *inner);
    int (*fills_window)(GGadget *g);
    int (*is_default)(GGadget *g);
};

enum gadget_state {gs_invisible, gs_disabled, gs_enabled, gs_active,
		   gs_focused, gs_pressedactive };

struct ggadget {
    struct gfuncs *funcs;
    struct gwindow *base;
    GRect r;
    GRect inner;
    unichar_t mnemonic;
    unichar_t shortcut;
    short short_mask;
    struct ggadget *prev;
    unsigned int takes_input: 1;
    unsigned int takes_keyboard: 1;
    unsigned int focusable: 1;
    unsigned int has_focus: 1;
    unsigned int free_box: 1;
    unsigned int was_disabled: 1;
    unsigned int vert: 1;			/* For lines & scrollbars */
    unsigned int opengroup: 1;			/* For groupboxes */
    unsigned int prevlabel: 1;			/* For groupboxes */
    unsigned int contained: 1;			/* is part of a bigger ggadget (ie. a scrollbar is part of a listbox) */
    unsigned int gg_skip_hotkey_processing: 1;
    unsigned int gg_skip_unqualified_hotkey_processing: 1;
    short cid;
    void *data;
    GBox *box;
    enum gadget_state state;
    unichar_t *popup_msg;
    GGadgetHandler handle_controlevent;
    int16_t desired_width, desired_height;
};

typedef struct ggadget GLine;
typedef struct ggadget GGroup;

typedef struct ggadget GSpacer;		/* a blank space of a given size, used in box layout */

typedef struct glabel {		/* or simple text, or groupbox */
    GGadget g;
    unsigned int fh:8;
    unsigned int as: 8;
    unsigned int image_precedes: 1;
    unsigned int is_default: 1;
    unsigned int is_cancel: 1;
    unsigned int pressed: 1;
    unsigned int within: 1;
    unsigned int labeltype: 2;	/* 0=>label/button(this), 1=>imagebutton, 2=>listbutton, 3=>colorbutton */
    unsigned int shiftonpress: 1;
    FontInstance *font;
    unichar_t *label;
    GImage *image;
    GTextInfo **ti;
    uint16_t ltot;
} GLabel, GButton;

typedef struct gimagebutton {
    GGadget g;
    unsigned int fh:8;
    unsigned int as: 8;
    unsigned int image_precedes: 1;
    unsigned int is_default: 1;
    unsigned int is_cancel: 1;
    unsigned int pressed: 1;
    unsigned int within: 1;
    unsigned int labeltype: 2;	/* 0=>label, 1=>imagebutton(this), 2=>listbutton */
    unsigned int shiftonpress: 1;
    FontInstance *font;
    unichar_t *label;
    GImage *image, *img_within, *active, *disabled;
} GImageButton;

typedef struct glistbutton {
    GGadget g;
    unsigned int fh:8;
    unsigned int as: 8;
    unsigned int image_precedes: 1;
    unsigned int is_default: 1;
    unsigned int is_cancel: 1;
    unsigned int pressed: 1;
    unsigned int within: 1;
    unsigned int labeltype: 2;	/* 0=>label, 1=>imagebutton, 2=>listbutton(this) */
    unsigned int shiftonpress: 1;
    FontInstance *font;
    unichar_t *label;
    GImage *image;
    GTextInfo **ti;
    uint16_t ltot;
    GWindow popup;
} GListButton;

typedef struct gcolorbutton {
    GGadget g;
    unsigned int fh:8;
    unsigned int as: 8;
    unsigned int image_precedes: 1;
    unsigned int is_default: 1;
    unsigned int is_cancel: 1;
    unsigned int pressed: 1;
    unsigned int within: 1;
    unsigned int labeltype: 2;	/* 0=>label/button, 1=>imagebutton, 2=>listbutton, 3=>colorbutton(this) */
    unsigned int shiftonpress: 1;
    FontInstance *font;
    unichar_t *label;
    GImage *image;
    Color col;
} GColorButton;

typedef struct gcheck {
    GGadget g;
    unsigned int fh:8;
    unsigned int as: 8;
    unsigned int image_precedes: 1;
    unsigned int pressed: 1;
    unsigned int within: 1;
    unsigned int isradio: 1;
    unsigned int ison: 1;
    FontInstance *font;
    unichar_t *label;
    GImage *image;
    GRect onoffrect, onoffinner;
    GBox *onbox, *offbox;
    GResImage *on, *off, *ondis, *offdis;
} GCheckBox;

typedef struct gradio {
    GGadget g;
    unsigned int fh:8;
    unsigned int as: 8;
    unsigned int image_precedes: 1;
    unsigned int pressed: 1;
    unsigned int within: 1;
    unsigned int isradio: 1;
    unsigned int ison: 1;
    FontInstance *font;
    unichar_t *label;
    GImage *image;
    GRect onoffrect, onoffinner;
    GBox *onbox, *offbox;
    GResImage *on, *off, *ondis, *offdis;
    struct gradio *post;
    int radiogroup;
} GRadio;

typedef struct gscrollbar {		/* and slider */
    struct ggadget g;
    int32_t sb_min, sb_max, sb_pagesize, sb_pos;
    int32_t sb_mustshow;			/* normally this is sb_pagesize, but might be the height of a single line */
		    /* if we want people to be able to scroll to see white space */
		    /* after the document */
    /*unsigned int vert: 1; */	/* Moved to GGadget, shared with line */
    unsigned int thumbpressed: 1;
    unsigned int ignorenext45: 1;
    int8_t repeatcmd;		/*  sb event to be generated on timer interrupts (ie. upline)*/
    int8_t thumbborder;		/* Size of the border of the thumbbox */
    int8_t sbborder;		/* Size of the border of the main scrollbar */
    int16_t size_offset;		/* Thumb size offset when the thumb size gets clamped. */
    int16_t thumboff;		/* Offset from where the thumb was pressed to top of thumb */
    int16_t arrowsize;		
    int16_t thumbsize;		/* Current thumb size, refigured after every call to setbounds */
    int16_t thumbpos;		/* Current thumb pos */
    GTimer *pressed;
    GBox *thumbbox;
} GScrollBar;

typedef struct glist {
    GGadget g;
    uint8_t fh;
    uint8_t as;
    uint8_t sofar_max, sofar_pos;
    uint16_t ltot, loff, lcnt;
    uint16_t xoff, xmax;
    uint16_t start, end;			/* current selection drag */
    uint16_t hmax;		/* maximum line height */
    FontInstance *font;
    GTextInfo **ti;
    struct gscrollbar *vsb;
    int (*orderer)(const void *, const void *);
    unsigned int backwards: 1;		/* reverse the order given by orderer */
    unsigned int multiple_sel: 1;	/* Allow multiple selections */
    unsigned int exactly_one: 1;	/* List must always have something selected */
    unsigned int parentpressed: 1;	/* For listbuttons, pressed in parent */
    unsigned int freeti: 1;		/* Free the ti array when we're destroyed */
    unsigned int ispopup: 1;		/* respond to Return and Escape */
    unsigned int sameheight: 1;		/* all lines are the same height */
    unsigned int always_show_sb: 1;	/* display scrollbar even if we don't need it */
    unichar_t *sofar;			/* user input */
    GTimer *enduser;
    GTimer *pressed;
    void (*popup_callback)(GGadget *g,int pos);
} GList;

typedef struct gscroll1box {
    GGadget g;
    int32_t count;
    int32_t sbsize;
    int32_t minsize, oppoatmin;
    int32_t subwidth, subheight;
    int32_t oppoatcur; // Not subwidth/height because the scroll bar can add size
    int32_t pad;
    int32_t scrollchange;
    int32_t offset;    // wrt scrolling
    GGadget **children;
    GGadget *sb;
    unsigned int vertical: 1;
    unsigned int always_show_sb: 1;
    unsigned int sized_for_sb: 1;
    unsigned int align_flow_labels: 1;
    enum gg_flags just;
    GWindow nested;
} GScroll1Box;

typedef struct gtextfield {
    GGadget g;
    unsigned int cursor_on: 1;
    unsigned int wordsel: 1;
    unsigned int linesel: 1;
    unsigned int listfield: 1;
    unsigned int drag_and_drop: 1;
    unsigned int has_dd_cursor: 1;
    unsigned int hidden_cursor: 1;
    unsigned int multi_line: 1;
    unsigned int accepts_tabs: 1;
    unsigned int accepts_returns: 1;
    unsigned int wrap: 1;
    unsigned int password: 1;
    unsigned int dontdraw: 1;	/* Used when the tf is part of a larger control, and the control determines when to draw the tf */
    unsigned int numericfield: 1;
    unsigned int incr_down: 1;	/* Direction of increments when numeric_scroll events happen */
    unsigned int completionfield: 1;
    unsigned int was_completing: 1;
    uint8_t fh;
    uint8_t as;
    uint8_t nw;			/* Width of one character (an "n") */
    int16_t xoff_left, loff_top;
    int16_t sel_start, sel_end, sel_base;
    int16_t sel_oldstart, sel_oldend, sel_oldbase;
    int16_t dd_cursor_pos;
    unichar_t *text, *oldtext;
    FontInstance *font;
    GTimer *pressed;
    GTimer *cursor;
    GCursor old_cursor;
    GScrollBar *hsb, *vsb;
    int16_t lcnt, lmax;
    int32_t *lines;		/* offsets in text to the start of the nth line */
    int16_t xmax;
    GIC *gic;
    GTimer *numeric_scroll;
    char *utf8_text;		/* For Pango */
    int32_t *lines8;		/* offsets in utf8_text */
} GTextField;

typedef struct glistfield {
    GTextField gt;
    GRect fieldrect, buttonrect;
    GTextInfo **ti;
    uint16_t ltot;
    GWindow popup;
} GListField;

typedef struct gcompletionfield {
    GListField gl;
    unichar_t **choices;
    uint16_t ctot; int16_t selected;
    GWindow choice_popup;
    GTextCompletionHandler completion;
} GCompletionField;

typedef struct gnumericfield {
    GTextField gt;
    GRect fieldrect, buttonrect;
} GNumericField;

typedef struct gmenubar {
    GGadget g;
    GMenuItem *mi;
    uint16_t *xs;			/* locations at which to draw each name (+1 to give us width of last one) */
    uint16_t mtot;
    int16_t entry_with_mouse;
    int16_t lastmi;		/* If the menubar doesn't fit across the top the make some of it be vertical. Start here */
    int ascender;
    int descender;
    struct gmenu *child;
    unsigned int pressed: 1;
    unsigned int initial_press: 1;
    unsigned int any_unmasked_shortcuts: 1;
    FontInstance *font;
    GMenuItem fake[2];		/* Used if not enough room for menu... */
} GMenuBar;

struct tabs { unichar_t *name; int16_t x, width, tw, nesting; unsigned int disabled: 1; GWindow w; };

typedef struct gtabset {
    struct ggadget g;
    struct tabs *tabs;
    int16_t *rowstarts;		/* for each row, index into tab array of its first tab, one extra entry at end with tabcnt */
    int16_t tabcnt;		/* number of tabs */
    int16_t sel;			/* active tab */
    int16_t oldsel;       /* used when swapping tabs */
    int16_t rcnt;			/* number of rows */
    int16_t active_row;		/* row which is closest to the display area */
    int16_t offset_per_row;	/* stagger tabs by this much */
    int16_t rowh;			/* height of each row */
    int16_t toff;			/* amount things are scrolled off left (x, tabs) */
    int16_t arrow_width;		/* width of arrow tab (for scrolling) */
    int16_t arrow_size;		/* size of the actual arrow itself */
    int16_t ds;
    int16_t pressed_sel;
    unsigned int scrolled: 1;	/* big tabsets either get scrolled or appear in multiple rows */
    unsigned int haslarrow: 1;
    unsigned int hasrarrow: 1;
    unsigned int pressed: 1;
    unsigned int filllines: 1;	/* If we have multiple lines then fill them so that each row takes up the entire width of the tabset */
    unsigned int fill1line: 1;
    unsigned int vertical: 1;
    unsigned int nowindow: 1;
    bool closable, movable;
    FontInstance *font;
    void (*remove_sync)(GWindow gw, int pos);
    void (*swap_sync)(GWindow gw, int pos_a, int pos_b);
    void (*nested_expose)(GWindow pixmap, GGadget *g, GEvent *event);
    int (*nested_mouse)(GGadget *g, GEvent *event);
    int16_t vert_list_width;
    int16_t as, fh, offtop;
    GGadget *vsb;
} GTabSet;

struct gdirentry;
typedef struct gfilechooser {
    struct ggadget g;
    GTextField *name;
    GList *files, *subdirs;
    GListButton *directories;
    GButton *ok, *filterb;	/* Not created by us, can be set by user to give chooser a better appearance */
    unichar_t **mimetypes;
    unichar_t *wildcard;
    unichar_t *lastname;
    GFileChooserFilterType filter;
    GFileChooserInputFilenameFuncType inputfilenamefunc;
    GButton *up, *home;
    GButton *bookmarks, *config;
    struct ghvbox *topbox;
    unichar_t **history;
    unichar_t **paths;
    unichar_t *inputfilenameprevchar;
    int hpos, hcnt, hmax;
} GFileChooser;

typedef struct ghvbox {
    GGadget g;
    int rows, cols;
    int hpad, vpad;			/* Internal padding */
    int grow_col, grow_row;		/* -1 => all */
    GGadget **children;			/* array of rows*cols */
    GGadget *label;
    int label_height;
} GHVBox;

typedef struct gflowbox {
    GGadget g;
    int count;
    int hpad, vpad, lpad;		/* Internal padding */
    int label_size;
    enum gg_flags just;
    unsigned int vertical: 1;
    GGadget **children;
    GGadget *label;
} GFlowBox;

struct col_data {
    enum me_type me_type;
    char *(*func)(GGadget *,int r,int c); /* Produces a string to display if md_str==NULL */
    GMenuItem *enum_vals;
    void (*enable_enum)(GGadget *,GMenuItem *, int r, int c);
    GTextCompletionHandler completer;
    char *title;
    int16_t width, x;			/* Relative to inner.x */
    uint8_t fixed;
    uint8_t disabled;
    uint8_t hidden;
};

typedef struct gmatrixedit {
    GGadget g;
    int rows, cols;
    int row_max;
    struct col_data *col_data;
    int hpad, vpad;			/* Internal padding */
    unsigned int has_titles: 1;
    unsigned int lr_pointer: 1;
    unsigned int wasnew: 1;		/* So we need to call newafter when finished editing */
    unsigned int big_done: 1;
    unsigned int edit_active: 1;
    unsigned int no_edit: 1;
    int pressed_col;			/* For changing column spacing */
    struct matrix_data *data;
    int16_t as, fh;
    int16_t font_as, font_fh;
    FontInstance *font;
    FontInstance *titfont;
    GGadget *tf;
    int active_col, active_row;
    int off_top, off_left;
    GGadget *vsb, *hsb;
    GGadget *del;
    GGadget *up, *down;
    GGadget **buttonlist;
    GWindow nested;
    int16_t mark_length, mark_size, mark_skip;
    char *newtext;
    void (*initrow)(GGadget *g,int row);
    int  (*candelete)(GGadget *g,int row);
    enum gme_updown (*canupdown)(GGadget *g,int row);
    void (*popupmenu)(GGadget *g,GEvent *e,int row,int col);
    int  (*handle_key)(GGadget *g,GEvent *e);
    char *(*bigedittitle)(GGadget *g,int r, int c);
    void (*finishedit)(GGadget *g,int r, int c, int wasnew);
    char *(*validatestr)(GGadget *g,int r, int c, int wasnew, char *str);
    void (*setotherbuttons)(GGadget *g, int r, int c);
    void (*reportmousemove)(GGadget *g, int r, int c);
    void (*reporttextchanged)(GGadget *g, int r, int c, GGadget *textfield);
    void (*predelete)(GGadget *g, int r);
    void (*rowmotion)(GGadget *g, int oldr, int newr);
} GMatrixEdit;

typedef struct gdrawable {
    GGadget g;
    GWindow gw;
    GDrawEH e_h;
} GDrawable;

typedef struct rowcol {
    GGadget g;
    int rows, cols;
    GFont *font;
    int as, fh;
    unsigned int hrules: 1;		/* Draw horizontal lines between each row */
    unsigned int vrules: 1;		/* Draw vertical lines between each column */
    unsigned int display_only: 1;
    unsigned int order_entries: 1;	/* normally order rows based on first column entry */
    uint8_t hpad;
    int *colx;				/* col+1 entries, last is xmax */
    GTextInfo **labels;
    GTextInfo **ti;
    GTextField *tf;
    GScrollBar *vsb, *hsb;
    int loff, xoff;
    int tfr, tfc;			/* row,col of textfield (or -1) */
    int (*orderer)(const void *, const void *);
} RowCol;


/* ColorPicker */

extern int _GScrollBar_StartTime,_GScrollBar_RepeatTime;	/* in millisecs */
extern int _GScrollBar_Width;		/* in points */
extern int _GListMarkSize;		/* in points, def width of popup mark in buttons */
extern int _GGadget_Skip;		/* in points, def hor space between gadgets */
extern int _GGadget_TextImageSkip;	/* in points, def hor space text and image */
extern GBox _GListMark_Box, _GGroup_LineBox;
extern GResFont _ggadget_default_font;

void _GWidget_AddGGadget(GWindow gw,struct ggadget *g);
void _GWidget_RemoveGadget(struct ggadget *g);
void _GWidget_SetMenuBar(GGadget *g);
void _GWidget_SetDefaultButton(GGadget *g);
void _GWidget_MakeDefaultButton(GGadget *g);
void _GWidget_SetCancelButton(GGadget *g);
void _GWidget_SetGrabGadget(GGadget *g);
void _GWidget_ClearGrabGadget(GGadget *g);
void _GWidget_SetPopupOwner(GGadget *g);
void _GWidget_ClearPopupOwner(GGadget *g);

extern void _GGadgetCopyDefaultBox(GBox *box);
extern void _GGadgetInitDefaultBox(const char *class, GBox *box);
extern void _ggadget_underlineMnemonic(GWindow gw,int32_t x,int32_t y,unichar_t *label,
	unichar_t mneumonic, Color fg,int ymax);
extern void _ggadgetFigureSize(GWindow gw, GBox *design, GRect *r, int isdef);
extern void _ggadgetSetRects(GGadget *g, GRect *outer, GRect *inner, int xjust, int yjust );
extern void _GGadgetCloseGroup(GGadget *g);
extern void _ggadget_redraw(GGadget *g);
extern int _ggadget_noop(GGadget *g, GEvent *event);
extern void _ggadget_move(GGadget *g, int32_t x, int32_t y );
extern void _ggadget_resize(GGadget *g, int32_t width, int32_t height );
extern void _ggadget_setvisible(GGadget *g,int visible);
extern void _ggadget_setenabled(GGadget *g,int enabled);
extern GRect *_ggadget_getsize(GGadget *g,GRect *rct);
extern GRect *_ggadget_getinnersize(GGadget *g,GRect *rct);
extern void _ggadget_getDesiredSize(GGadget *g, GRect *outer, GRect *inner);
extern void _ggadget_setDesiredSize(GGadget *g,GRect *outer, GRect *inner);
void _GGroup_Init(void);

extern unichar_t *_GGadgetFileToUString(char *filename,int max);

extern int GBoxDrawBorder(GWindow gw,GRect *pos,GBox *design,
	enum gadget_state state,int is_default);
extern void GBoxDrawBackground(GWindow gw,GRect *pos,GBox *design,
	enum gadget_state state,int is_default);
extern void GBoxDrawTabOutline(GWindow pixmap, GGadget *g, int x, int y,
	int width, int rowh, int active );
extern int GBoxDrawHLine(GWindow gw,GRect *pos,GBox *design);
extern int GBoxDrawVLine(GWindow gw,GRect *pos,GBox *design);
extern int GBoxBorderWidth(GWindow gw, GBox *box);
extern int GBoxExtraSpace(GGadget *g);
extern int GBoxDrawnWidth(GWindow gw, GBox *box);

extern int GGadgetInnerWithin(GGadget *g, int x, int y);

extern int GTextInfoGetWidth(GWindow base,GTextInfo *ti,FontInstance *font);
extern int GTextInfoGetMaxWidth(GWindow base,GTextInfo **ti,FontInstance *font);
extern int GTextInfoGetHeight(GWindow base,GTextInfo *ti,FontInstance *font);
extern int GTextInfoGetMaxHeight(GWindow base,GTextInfo **ti,FontInstance *font,int *allsame);
extern int GTextInfoGetAs(GWindow base,GTextInfo *ti, FontInstance *font);
extern int GTextInfoDraw(GWindow base,int x,int y,GTextInfo *ti,
	FontInstance *font,Color fg,Color sel,int ymax, int as, int ds);
extern GTextInfo *GTextInfoCopy(GTextInfo *ti);
extern GTextInfo **GTextInfoArrayFromList(GTextInfo *ti, uint16_t *cnt);
extern GTextInfo **GTextInfoArrayCopy(GTextInfo **ti);
extern int GTextInfoArrayCount(GTextInfo **ti);
extern int GTextInfoCompare(GTextInfo *ti1, GTextInfo *ti2);
extern int GMenuItemArrayMask(GMenuItem *mi);
extern int GMenuItemArrayAnyUnmasked(GMenuItem *mi);
extern void _GFlowBoxGetDesiredSize(GGadget *g, GRect *outer, GRect *inner, int squashed, int ignore_des);

extern GGadget *_GGadget_Create(GGadget *g, struct gwindow *base, GGadgetData *gd,void *data, GBox *def);
extern void _GGadget_FinalPosition(GGadget *g, struct gwindow *base, GGadgetData *gd);
extern void _ggadget_destroy(GGadget *g);

extern GWindow GListPopupCreate(GGadget *owner,void (*inform)(GGadget *,int), GTextInfo **ti);

extern int GMenuPopupCheckKey(GEvent *event);
extern int GMenuBarCheckKey(GWindow top, GGadget *g, GEvent *event);
extern void _GButton_SetDefault(GGadget *g,int32_t is_default);
extern void _GButtonInit(void);
extern void GListMarkDraw(GWindow pixmap,int x, int y, int height, enum gadget_state state );
extern const char* const* _GGadget_GetImagePath(void);
extern int _GGadget_ImageInCache(GImage *image);

extern GResInfo ggadget_ri, listmark_ri;

#endif /* FONTFORGE_GGADGET_P_H */
