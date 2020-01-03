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

#ifndef FONTFORGE_GGADGET_H
#define FONTFORGE_GGADGET_H

#include "gdraw.h"
#include "intl.h"
struct giocontrol;

#ifndef MAX
#define MAX(x,y)   (((x) > (y)) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x,y)   (((x) < (y)) ? (x) : (y))
#endif

typedef struct gtextinfo {
    unichar_t *text;
    GImage *image;
    Color fg;
    Color bg;
    void *userdata;
    GFont *font;
    unsigned int disabled: 1;
    unsigned int image_precedes: 1;
    unsigned int checkable: 1;			/* Only for menus */
    unsigned int checked: 1;			/* Only for menus */
    unsigned int selected: 1;			/* Only for lists (used internally for menu(bar)s, when cursor is on the line) */
    unsigned int line: 1;			/* Only for menus */
    unsigned int text_is_1byte: 1;		/* If passed in as 1byte (ie. iso-8859-1) text, will be converted */
    unsigned int text_in_resource: 1;		/* the text field is actually an index into the string resource table */
    unsigned int changed: 1;			/* If a row/column widget changed this */
    unichar_t mnemonic;				/* Only for menus and menubars */
						/* should really be in menuitem, but that wastes space and complicates GTextInfoDraw */
    char* text_untranslated;                    /* used to simplify hotkey lookup for menus. */
    /* 
     * text_untranslated is either the GMenuItem2 shortcut or the GTextInfo prior
     * to translation occurring. The shortcut text is considered first
     * to allow the code to make the value explicit. This is useful in
     * cases where the menu text to be translated (GTextInfo.text
     * prior to calling sgettext() on it) is specially designed for
     * translation, like File|New is. Having the hotkey of "New|No
     * Shortcut" will give a text_untranslated of "New|No Shortcut".
     * See HKTextInfoToUntranslatedText() for stripping out any
     * potential underscore and the trailing "|Rest" string.
     *
     * Using a pointer like this relies on the GMenuItems used to make
     * the menus are a static structure that outlasts the
     * menu/gtextinfo itself.
     **/
} GTextInfo;

#define GTEXTINFO_EMPTY { NULL, NULL, 0x000000, 0x000000, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\0' }


typedef struct gtextinfo2 {
    unichar_t *text;
    GImage *image;
    Color fg;
    Color bg;
    void *userdata;
    GFont *font;
    unsigned int disabled: 1;
    unsigned int image_precedes: 1;
    unsigned int checkable: 1;			/* Only for menus */
    unsigned int checked: 1;			/* Only for menus */
    unsigned int selected: 1;			/* Only for lists (used internally for menu(bar)s, when cursor is on the line) */
    unsigned int line: 1;			/* Only for menus */
    unsigned int text_is_1byte: 1;		/* If passed in as 1byte (ie. iso-8859-1) text, will be converted */
    unsigned int text_in_resource: 1;		/* the text field is actually an index into the string resource table */
    unsigned int changed: 1;			/* If a row/column widget changed this */
    unsigned int sort_me_first_in_list: 1;	/* used for directories in file chooser widgets */
    unichar_t mnemonic;				/* Only for menus and menubars */
						/* should really be in menuitem, but that wastes space and complicates GTextInfoDraw */
} GTextInfo2;

#define GTEXTINFO2_EMPTY { NULL, NULL, 0x000000, 0x000000, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\0' }


typedef struct gmenuitem {
    GTextInfo ti;
    unichar_t shortcut;
    short short_mask;
    struct gmenuitem *sub;
    void (*moveto)(struct gwindow *base,struct gmenuitem *mi,GEvent *);	/* called before creating submenu */
    void (*invoke)(struct gwindow *base,struct gmenuitem *mi,GEvent *);	/* called on mouse release */
    int mid;
} GMenuItem;

#define GMENUITEM_LINE   { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }
#define GMENUITEM_EMPTY { GTEXTINFO_EMPTY, '\0', 0, NULL, NULL, NULL, 0 }


typedef struct gmenuitem2 {
    GTextInfo ti;
    char *shortcut;
    struct gmenuitem2 *sub;
    void (*moveto)(struct gwindow *base,struct gmenuitem *mi,GEvent *);	/* called before creating submenu */
    void (*invoke)(struct gwindow *base,struct gmenuitem *mi,GEvent *);	/* called on mouse release */
    int mid;
} GMenuItem2;

#define GMENUITEM2_LINE { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 1, 0, 0, 0, 1, 0, 0, 0, '\0' }, NULL, NULL, NULL, NULL, 0 }
#define GMENUITEM2_EMPTY { GTEXTINFO_EMPTY, NULL, NULL, NULL, NULL, 0 }

typedef struct tabinfo {
    unichar_t *text;
    struct ggadgetcreatedata *gcd;
    unsigned int disabled: 1;
    unsigned int selected: 1;
    unsigned int text_is_1byte: 1;		/* If passed in as 1byte (ie. iso-8859-1) text, will be converted */
    unsigned int text_in_resource: 1;		/* the text field is actually an index into the string resource table */
    unsigned char nesting;
} GTabInfo;

#define GTABINFO_EMPTY { NULL, NULL, 0, 0, 0, 0, 0 }


enum border_type { bt_none, bt_box, bt_raised, bt_lowered, bt_engraved,
	    bt_embossed, bt_double };
enum border_shape { bs_rect, bs_roundrect, bs_elipse, bs_diamond };
enum box_flags {
    box_foreground_border_inner = 1,	/* 1 point line */
    box_foreground_border_outer = 2,	/* 1 point line */
    box_active_border_inner = 4,		/* 1 point line */
    box_foreground_shadow_outer = 8,	/* 1 point line, bottom&right */
    box_do_depressed_background = 0x10,
    box_draw_default = 0x20,	/* if a default button draw a depressed rect around button */
    box_generate_colors = 0x40,	/* use border_brightest to compute other border cols */
    box_gradient_bg = 0x80
    };
typedef struct gbox {
    unsigned char border_type;	
    unsigned char border_shape;	
    unsigned char border_width;	/* In points */
    unsigned char padding;	/* In points */
    unsigned char rr_radius;	/* In points */
    unsigned char flags;
    Color border_brightest;		/* used for left upper part of elipse */
    Color border_brighter;
    Color border_darkest;		/* used for right lower part of elipse */
    Color border_darker;
    Color main_background;
    Color main_foreground;
    Color disabled_background;
    Color disabled_foreground;
    Color active_border;
    Color depressed_background;
    Color gradient_bg_end;
    Color border_inner;
    Color border_outer;
} GBox;

#define GBOX_EMPTY { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0 }


typedef struct ggadget GGadget;
typedef struct ggadget *GGadgetSet;

enum sb_type { sb_upline, sb_downline, sb_uppage, sb_downpage, sb_track, sb_trackrelease };
struct scrollbarinit { int32 sb_min, sb_max, sb_pagesize, sb_pos; };

typedef int (*GGadgetHandler)(GGadget *,GEvent *);
typedef unichar_t **(*GTextCompletionHandler)(GGadget *,int from_tab);

enum gg_flags { gg_visible=1, gg_enabled=2, gg_pos_in_pixels=4,
		gg_sb_vert=8, gg_line_vert=gg_sb_vert,
		gg_but_default=0x10, gg_but_cancel=0x20,
		gg_cb_on=0x40, gg_rad_startnew=0x80,
		gg_rad_continueold=0x100,	/* even if not previous */
		gg_list_alphabetic=0x100, gg_list_multiplesel=0x200,
		gg_list_exactlyone=0x400, gg_list_internal=0x800,
		gg_group_prevlabel=0x1000, gg_group_end=0x2000,
		gg_textarea_wrap=0x4000,
		gg_tabset_scroll=0x8000, gg_tabset_filllines=0x10000, gg_tabset_fill1line = 0x20000,
		gg_tabset_nowindow=gg_textarea_wrap,
		gg_rowcol_alphabetic=gg_list_alphabetic,
		gg_rowcol_vrules=0x40000, gg_rowcol_hrules=0x800000,
		gg_rowcol_displayonly=0x1000000,
		gg_dontcopybox=0x10000000,
		gg_pos_use0=0x20000000, gg_pos_under=0x40000000,
		gg_pos_newline = (int) 0x80000000,
		/* Reuse some flag values for different widgets */
		gg_file_pulldown=gg_sb_vert, gg_file_multiple = gg_list_multiplesel,
		gg_text_xim = gg_tabset_scroll,
		gg_tabset_vert = gg_sb_vert
};

typedef struct ggadgetdata {
    GRect pos;
    GBox *box;
    unichar_t mnemonic;
    unichar_t shortcut;
    uint8 short_mask;
    uint8 cols;			/* for rowcol */
    short cid;
    GTextInfo *label;		/* Overloaded with a GGadgetCreateData * for hvboxes (their label is a gadget) */
    union {
	GTextInfo *list;	/* for List Widgets (and ListButtons, RowCols etc) */
	GTabInfo *tabs;		/* for Tab Widgets */
	GMenuItem *menu;	/* for menus */
	GMenuItem2 *menu2;	/* for menus (alternate) */
	struct ggadgetcreatedata **boxelements;	/* An array of things to go in the box */
	struct matrixinit *matrix;
	GDrawEH drawable_e_h;	/* Drawable event handler */
	GTextCompletionHandler completion;
	struct scrollbarinit *sbinit;
	Color col;
	int radiogroup;
    } u;
    enum gg_flags flags;
    const char *popup_msg;		/* Brief help message, utf-8 encoded */
    GGadgetHandler handle_controlevent;
} GGadgetData;

#define GGADGETDATA_EMPTY { GRECT_EMPTY, NULL, '\0', '\0', 0, 0, 0, NULL, { NULL }, 0, NULL, NULL }


typedef struct ggadgetcreatedata {
    GGadget *(*creator)(struct gwindow *base, GGadgetData *gd,void *data);
    GGadgetData gd;
    void *data;
    GGadget *ret;
} GGadgetCreateData;

#define GGADGETCREATEDATA_EMPTY { NULL, GGADGETDATA_EMPTY, NULL, NULL }


#define GCD_Glue	((GGadgetCreateData *) -1)	/* Special entries */
#define GCD_ColSpan	((GGadgetCreateData *) -2)	/* for box elements */
#define GCD_RowSpan	((GGadgetCreateData *) -3)
#define GCD_HPad10	((GGadgetCreateData *) -4)

enum ghvbox_expand { gb_expandglue=-4, gb_expandgluesame=-3, gb_samesize=-2,
	gb_expandall=-1 };
enum editor_commands { ec_cut, ec_clear, ec_copy, ec_paste, ec_undo, ec_redo,
	ec_selectall, ec_search, ec_backsearch, ec_backword, ec_deleteword,
	ec_max };

    /* return values from file chooser filter functions */
enum fchooserret { fc_hide, fc_show, fc_showdisabled };

enum me_type { me_int, me_enum, me_real, me_string, me_bigstr, me_func,
	       me_funcedit,
	       me_stringchoice, me_stringchoicetrans, me_stringchoicetag,
	       me_button,
	       me_hex, me_uhex, me_addr, me_onlyfuncedit };

struct col_init {
    enum me_type me_type;
    char *(*func)(GGadget *,int r,int c);
    GTextInfo *enum_vals;
    void (*enable_enum)(GGadget *,GMenuItem *, int r, int c);
    char *title;
};

struct matrix_data {
    union {
	intpt md_ival;
	double md_real;
	char *md_str;
	void *md_addr;
    } u;
    uint8 frozen;
    uint8 user_bits;
    uint8 current;
};

struct matrixinit {
    int col_cnt;
    struct col_init *col_init;
    int initial_row_cnt;
    struct matrix_data *matrix_data;
    void (*initrow)(GGadget *g,int row);
    int  (*candelete)(GGadget *g,int row);
    void (*finishedit)(GGadget *g,int r, int c, int wasnew);
    void (*popupmenu)(GGadget *g,GEvent *e,int row,int col);
    int  (*handle_key)(GGadget *g,GEvent *e);
    char *(*bigedittitle)(GGadget *g,int r, int c);
};

#define COL_INIT_EMPTY { 0, NULL, NULL, NULL, NULL }
#define MATRIX_DATA_EMPTY { { 0 }, 0, 0, 0 }
#define MATRIXINIT_EMPTY { 0, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL }

#define GME_NoChange	0x80000000

struct gdirentry;
typedef enum fchooserret (*GFileChooserFilterType)(GGadget *g,struct gdirentry *ent,
	const unichar_t *dir);
typedef int (*GFileChooserInputFilenameFuncType)( GGadget *g,
						  const unichar_t ** currentFilename,
						  unichar_t* oldfilename );

    /* Obsolete */
#define _STR_NULL	(-1)		/* Null string resource */
#define _STR_Language	0
#define _STR_OK		1
#define _STR_Cancel	2
#define _STR_Open	3
#define _STR_Save	4
#define _STR_Filter	5
#define _STR_New	6
#define _STR_Replace	7
#define _STR_Fileexists	8
#define _STR_Fileexistspre	9
#define _STR_Fileexistspost	10
#define _STR_Createdir	11
#define _STR_Dirname	12
#define _STR_Couldntcreatedir	13
#define _STR_SelectAll	14
#define _STR_None	15
#define __STR_LastStd	15

#define _NUM_Buttonsize		0
#define _NUM_ScaleFactor	1
#define __NUM_LastStd		1

extern void GTextInfoFree(GTextInfo *ti);
extern void GTextInfoListFree(GTextInfo *ti);
extern void GTextInfoArrayFree(GTextInfo **ti);
extern GTextInfo **GTextInfoFromChars(char **array, int len);
extern const unichar_t *GStringGetResource(int index,unichar_t *mnemonic);
extern int GGadgetScale(int xpos);
extern int GIntGetResource(int index);
extern int GStringSetResourceFileV(char *filename,uint32 checksum);
extern int GStringSetResourceFile(char *filename);	/* returns 1 for success, 0 for failure */
/* fallback string arrays are null terminated. mnemonics is same length as string */
/* fallback integer arrays are terminated by 0x80000000 (negative infinity) */
extern void GStringSetFallbackArray(const unichar_t **array,const unichar_t *mn,
	const int *ires);
unichar_t *GStringFileGetResource(char *filename, int index,unichar_t *mnemonic);
extern void GResourceUseGetText(void);
extern void *GResource_font_cvt(char *val, void *def);
extern FontInstance *GResourceFindFont(char *resourcename,FontInstance *deffont);

void GGadgetDestroy(GGadget *g);
void GGadgetSetVisible(GGadget *g,int visible);
int GGadgetIsVisible(GGadget *g);
void GGadgetSetEnabled(GGadget *g,int enabled);
int GGadgetIsEnabled(GGadget *g);
GWindow GGadgetGetWindow(GGadget *g);
void *GGadgetGetUserData(GGadget *g);
void GGadgetSetUserData(GGadget *g, void *d);
void GGadgetSetPopupMsg(GGadget *g, const unichar_t *msg);
int GGadgetContains(GGadget *g, int x, int y );
int GGadgetContainsEventLocation(GGadget *g, GEvent* e );
GRect *GGadgetGetInnerSize(GGadget *g,GRect *rct);
GRect *GGadgetGetSize(GGadget *g,GRect *rct);
void GGadgetSetSize(GGadget *g,GRect *rct);
void GGadgetGetDesiredVisibleSize(GGadget *g,GRect *outer, GRect *inner);
void GGadgetGetDesiredSize(GGadget *g,GRect *outer, GRect *inner);
void GGadgetSetDesiredSize(GGadget *g,GRect *outer, GRect *inner);
int GGadgetGetCid(GGadget *g);
void GGadgetResize(GGadget *g,int32 width, int32 height );
void GGadgetMove(GGadget *g,int32 x, int32 y );
void GGadgetMoveAddToY(GGadget *g, int32 yoffset );
int32 GGadgetGetX(GGadget *g);
int32 GGadgetGetY(GGadget *g);
void  GGadgetSetY(GGadget *g, int32 y );
void GGadgetRedraw(GGadget *g);
void GGadgetsCreate(GWindow base, GGadgetCreateData *gcd);
int  GGadgetFillsWindow(GGadget *g);
int  GGadgetIsDefault(GGadget *g);

void GGadgetSetTitle(GGadget *g,const unichar_t *title);
void GGadgetSetTitle8(GGadget *g,const char *title);
void GGadgetSetTitle8WithMn(GGadget *g,const char *title);
const unichar_t *_GGadgetGetTitle(GGadget *g);	/* Do not free!!! */
unichar_t *GGadgetGetTitle(GGadget *g);		/* Free the return */
char *GGadgetGetTitle8(GGadget *g);		/* Free the return (utf8) */
void GGadgetSetFont(GGadget *g,GFont *font);
GFont *GGadgetGetFont(GGadget *g);
int GGadgetEditCmd(GGadget *g,enum editor_commands cmd);
int GGadgetActiveGadgetEditCmd(GWindow gw,enum editor_commands cmd);
void GGadgetSetHandler(GGadget *g, GGadgetHandler handler);
GGadgetHandler GGadgetGetHandler(GGadget *g);

void GTextFieldSelect(GGadget *g,int sel_start, int sel_end);
void GTextFieldShow(GGadget *g,int pos);
void GTextFieldReplace(GGadget *g,const unichar_t *txt);
bool GTextFieldIsEmpty(GGadget *g);
void GCompletionFieldSetCompletion(GGadget *g,GTextCompletionHandler completion);
void GCompletionFieldSetCompletionMode(GGadget *g,int enabled);
void GGadgetClearList(GGadget *g);
void GGadgetSetList(GGadget *g, GTextInfo **ti, int32 copyit);
GTextInfo **GGadgetGetList(GGadget *g,int32 *len);	/* Do not free!!! */
GTextInfo *GGadgetGetListItem(GGadget *g,int32 pos);
GTextInfo *GGadgetGetListItemSelected(GGadget *g);
void GGadgetSelectListItem(GGadget *g,int32 pos,int32 sel);
void GGadgetSelectOneListItem(GGadget *g,int32 pos);
int32 GGadgetIsListItemSelected(GGadget *g,int32 pos);
int32 GGadgetGetFirstListSelectedItem(GGadget *g);
void GGadgetScrollListToPos(GGadget *g,int32 pos);
void GGadgetScrollListToText(GGadget *g,const unichar_t *lab,int32 sel);
void GGadgetSetListOrderer(GGadget *g,int (*orderer)(const void *, const void *));

void GColorButtonSetColor(GGadget *g, Color col);
Color GColorButtonGetColor(GGadget *g);

void GGadgetSetChecked(GGadget *g, int ison);
int GGadgetIsChecked(GGadget *g);

int GListIndexFromY(GGadget *g,int y);
void GListSetSBAlwaysVisible(GGadget *g,int always);
void GListSetPopupCallback(GGadget *g,void (*callback)(GGadget *,int));

int GTabSetGetTabCount(GGadget *g);
int GTabSetGetSel(GGadget *g);
void GTabSetSetSel(GGadget *g,int sel);
void GTabSetSetEnabled(GGadget *g,int pos, int enabled);
GWindow GTabSetGetSubwindow(GGadget *g,int pos);
int GTabSetGetTabLines(GGadget *g);
void GTabSetSetClosable(GGadget *g, bool flag);
void GTabSetSetMovable(GGadget *g, bool flag);
void GTabSetSetNestedExpose(GGadget *g, void (*)(GWindow,GGadget *,GEvent *));
void GTabSetSetNestedMouse(GGadget *g, int (*)(GGadget *,GEvent *));
void GTabSetSetRemoveSync(GGadget *g, void (*rs)(GWindow gw, int pos));
void GTabSetSetSwapSync(GGadget *g, void (*ss)(GWindow gw, int pos_a, int pos_b));
void GTabSetChangeTabName(GGadget *g, const char *name, int pos);
void GTabSetRemetric(GGadget *g);
void GTabSetSwapTabs(GGadget *g, int pos_a, int pos_b);
void GTabSetRemoveTabByPos(GGadget *g, int pos);
void GTabSetRemoveTabByName(GGadget *g, char *name);

int32 GScrollBarGetPos(GGadget *g);
int32 GScrollBarSetPos(GGadget *g,int32 pos);
int32 GScrollBarAddToPos(GGadget *g,int32 offset);
void GScrollBarSetMustShow(GGadget *g, int32 sb_min, int32 sb_max, int32 sb_pagesize,
	int32 sb_mustshow);
void GScrollBarSetBounds(GGadget *g, int32 sb_min, int32 sb_max, int32 sb_pagesize );
void GScrollBarGetBounds(GGadget *g, int32 *sb_min, int32 *sb_max, int32 *sb_pagesize );

void GMenuBarSetItemChecked(GGadget *g, int mid, int check);
void GMenuBarSetItemEnabled(GGadget *g, int mid, int enabled);
void GMenuBarSetItemName(GGadget *g, int mid, const unichar_t *name);
void GMenuSetShortcutDomain(char *domain);
const char *GMenuGetShortcutDomain(void);
int GMenuIsCommand(GEvent *event,char *shortcut);
int GMenuMask(void);
int GMenuAnyUnmaskedShortcuts(GGadget *mb1, GGadget *mb2);


void GFileChooserPopupCheck(GGadget *g,GEvent *e);
void GFileChooserFilterIt(GGadget *g);
void GFileChooserRefreshList(GGadget *g);
int GFileChooserFilterEh(GGadget *g,GEvent *e);
void GFileChooserConnectButtons(GGadget *g,GGadget *ok, GGadget *filter);
void GFileChooserSetFilterText(GGadget *g,const unichar_t *filter);
void GFileChooserSetFilterFunc(GGadget *g,GFileChooserFilterType filter);
void GFileChooserSetInputFilenameFunc(GGadget *g,GFileChooserInputFilenameFuncType filter);
int GFileChooserDefInputFilenameFunc( GGadget *g, const unichar_t** currentFilename, unichar_t* oldfilename );
GFileChooserInputFilenameFuncType GFileChooserGetInputFilenameFunc(GGadget *g);
void GFileChooserSetDir(GGadget *g,unichar_t *dir);
struct giocontrol *GFileChooserReplaceIO(GGadget *g,struct giocontrol *gc);
unichar_t *GFileChooserGetDir(GGadget *g);
unichar_t *GFileChooserGetFilterText(GGadget *g);
GFileChooserFilterType GFileChooserGetFilterFunc(GGadget *g);
void GFileChooserSetFilename(GGadget *g,const unichar_t *defaultfile);
void GFileChooserSetMimetypes(GGadget *g,unichar_t **mimetypes);
unichar_t **GFileChooserGetMimetypes(GGadget *g);
void GFileChooserGetChildren(GGadget *g,GGadget **pulldown, GGadget **list, GGadget **tf);
int GFileChooserPosIsDir(GGadget *g, int pos);
unichar_t *GFileChooserFileNameOfPos(GGadget *g, int pos);
void GFileChooserSetShowHidden(int sh);
int GFileChooserGetShowHidden(void);
void GFileChooserSetDirectoryPlacement(int dp);
int GFileChooserGetDirectoryPlacement(void);
void GFileChooserSetBookmarks(unichar_t **b);
void GFileChooserSetPaths(GGadget *g, const char* const* path);
unichar_t **GFileChooserGetBookmarks(void);
void GFileChooserSetPrefsChangedCallback(void *data, void (*p_c)(void *));

void GHVBoxSetExpandableCol(GGadget *g,int col);
void GHVBoxSetExpandableRow(GGadget *g,int row);
void GHVBoxSetPadding(GGadget *g,int hpad, int vpad);
void GHVBoxFitWindow(GGadget *g);
void GHVBoxFitWindowCentered(GGadget *g);
void GHVBoxReflow(GGadget *g);

void GMatrixEditSet(GGadget *g,struct matrix_data *data, int rows, int copy_it);
struct matrix_data *GMatrixEditGet(GGadget *g, int *rows);
struct matrix_data *_GMatrixEditGet(GGadget *g, int *rows);
GGadget *_GMatrixEditGetActiveTextField(GGadget *g);
int GMatrixEditGetColCnt(GGadget *g);
int GMatrixEditGetActiveRow(GGadget *g);
int GMatrixEditGetActiveCol(GGadget *g);
void GMatrixEditActivateRowCol(GGadget *g, int r, int c);
void GMatrixEditDeleteRow(GGadget *g,int row);
void GMatrixEditScrollToRowCol(GGadget *g,int r, int c);
int GMatrixEditStringDlg(GGadget *g,int row,int col);
void GMatrixEditSetNewText(GGadget *g, char *text);
void GMatrixEditSetOtherButtonEnable(GGadget *g, void (*sob)(GGadget *g, int r, int c));
void GMatrixEditSetMouseMoveReporter(GGadget *g, void (*rmm)(GGadget *g, int r, int c));
void GMatrixEditSetTextChangeReporter(GGadget *g, void (*tcr)(GGadget *g, int r, int c, GGadget *text));
void GMatrixEditSetValidateStr(GGadget *g, char *(*validate)(GGadget *g, int r, int c, int wasnew, char *str));
void GMatrixEditSetBeforeDelete(GGadget *g, void (*predelete)(GGadget *g, int r));
void GMatrixEditSetRowMotionCallback(GGadget *g, void (*rowmotion)(GGadget *g, int oldr, int newr));
void GMatrixEditUp(GGadget *g);
void GMatrixEditDown(GGadget *g);
enum gme_updown { ud_up_enabled=1, ud_down_enabled=2 };
void GMatrixEditSetCanUpDown(GGadget *g, enum gme_updown (*canupdown)(GGadget *g, int r));
void GMatrixEditSetUpDownVisible(GGadget *g, int visible);
void GMatrixEditAddButtons(GGadget *g, GGadgetCreateData *gcd);
void GMatrixEditEnableColumn(GGadget *g, int col, int enabled);
void GMatrixEditShowColumn(GGadget *g, int col, int visible);
void GMatrixEditSetColumnChoices(GGadget *g, int col, GTextInfo *ti);
GMenuItem *GMatrixEditGetColumnChoices(GGadget *g, int col);
void GMatrixEditSetColumnCompletion(GGadget *g, int col, GTextCompletionHandler completion);
void GMatrixEditSetEditable(GGadget *g, int editable);

GWindow GDrawableGetWindow(GGadget *g);


extern void GGadgetPreparePopupImage(GWindow base,const unichar_t *msg,
	const void *data,
	GImage *(*get_image)(const void *data),
	void (*free_image)(const void *data,GImage *img));
extern void GGadgetPreparePopup(GWindow base,const unichar_t *msg);
extern void GGadgetPreparePopupR(GWindow base,int msg);
extern void GGadgetPreparePopup8(GWindow base, const char *msg);
extern void GGadgetEndPopup(void);
extern void GGadgetPopupExternalEvent(GEvent *e);

extern int GGadgetDispatchEvent(GGadget *g,GEvent *e);
extern void GGadgetTakesKeyboard(GGadget *g, int takes_keyboard);

/* Handles *?{}[] wildcards */
int GGadgetWildMatch(unichar_t *pattern, unichar_t *name,int ignorecase);
enum fchooserret GFileChooserDefFilter(GGadget *g,struct gdirentry *ent,
	const unichar_t *dir);

GWindow GMenuCreatePopupMenu(GWindow owner,GEvent *event, GMenuItem *mi);
GWindow GMenuCreatePopupMenuWithName(GWindow owner,GEvent *event, char* subMenuName,GMenuItem *mi);
GWindow _GMenuCreatePopupMenu(GWindow owner,GEvent *event, GMenuItem *mi,
			      void (*donecallback)(GWindow owner));
GWindow _GMenuCreatePopupMenuWithName(GWindow owner,GEvent *event, GMenuItem *mi,
				      char* subMenuName, 
				      void (*donecallback)(GWindow owner));


GGadget *GLineCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GGroupCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GSpacerCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GLabelCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GButtonCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GImageButtonCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GListButtonCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GColorButtonCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GRadioCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GCheckBoxCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GVisibilityBoxCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GScrollBarCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GListCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GTextFieldCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GPasswordCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GNumericFieldCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GTextCompletionCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GTextAreaCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GListFieldCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GSimpleListFieldCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GMenuBarCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GMenu2BarCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GTabSetCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GFileChooserCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GHBoxCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GVBoxCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GHVBoxCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GHVGroupCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GMatrixEditCreate(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *GDrawableCreate(struct gwindow *base, GGadgetData *gd,void *data);

GGadget *CreateSlider(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *CreateFileChooser(struct gwindow *base, GGadgetData *gd,void *data);
GGadget *CreateGadgets(struct gwindow *base, GGadgetCreateData *gcd);

GTextInfo **GTextInfoArrayFromList(GTextInfo *ti, uint16 *cnt);
typedef struct gresimage {
    GImage *image;
    char *filename;
} GResImage;
GResImage *GGadgetResourceFindImage(char *name, GImage *def);

void InitImageCache();
void ClearImageCache();
void GGadgetSetImageDir(char *dir);
void GGadgetSetImagePath(char *path);
GImage *GGadgetImageCache(const char *filename);
int TryGGadgetImageCache(GImage *image, const char *name);

extern unichar_t *utf82u_mncopy(const char *utf8buf,unichar_t *mn);

extern double GetCalmReal8(GWindow gw,int cid,char *namer,int *err);
extern double GetReal8(GWindow gw,int cid,char *namer,int *err);
extern int GetCalmInt8(GWindow gw,int cid,char *name,int *err);
extern int GetInt8(GWindow gw,int cid,char *namer,int *err);
extern int GetUnicodeChar8(GWindow gw,int cid,char *namer,int *err);
extern void GGadgetProtest8(char *labelr);

extern void GMenuItemParseShortCut(GMenuItem *mi,char *shortcut);
extern int GMenuItemParseMask(char *shortcut);
extern int GGadgetUndoMacEnglishOptionCombinations(GEvent *event);

/* Among other things, this routine sets global icon cache up. */
extern void GGadgetInit(void);
extern int GGadgetWithin(GGadget *g, int x, int y);
extern void GMenuItemArrayFree(GMenuItem *mi);
extern void GMenuItem2ArrayFree(GMenuItem2 *mi);
extern GMenuItem *GMenuItemArrayCopy(GMenuItem *mi, uint16 *cnt);
extern GMenuItem *GMenuItem2ArrayCopy(GMenuItem2 *mi, uint16 *cnt);

extern void GVisibilityBoxSetToMinWH(GGadget *g);

extern void GGadgetSetSkipHotkeyProcessing( GGadget *g, int v );
extern int GGadgetGetSkipHotkeyProcessing( GGadget *g );
extern void GGadgetSetSkipUnQualifiedHotkeyProcessing( GGadget *g, int v );
extern int GGadgetGetSkipUnQualifiedHotkeyProcessing( GGadget *g );

#endif /* FONTFORGE_GGADGET_H */
