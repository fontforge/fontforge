/* Copyright (C) 2001 by George Williams */
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
#ifndef _TTFVIEW_H
#define _TTFVIEW_H

#include "ttffont.h"
#include "conicfont.h"
#include <ggadget.h>
#include <gwidget.h>

struct ttfargs;
struct ttfactions;

struct tableviewfuncs {
    int (*closeme)(struct tableview *);		/* 1 return => closed, 0 => cancelled */
    int (*processdata) (struct tableview *);	/* bring table->data up to date */
};

/* Superclass for all tables */
typedef struct tableview {
    Table *table;
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
    unsigned int destroyed: 1;		/* window has been destroyed */
} TableView;

typedef struct ttfview {
    TtfFile *ttf;
    GWindow gw, v;
    GGadget *vsb, *mb;
    int16 mbh;		/* Menu bar height */
    int vheight, vwidth;/* of v */
    int lheight, lpos;	/* logical height */
    GFont *font, *bold;
    int16 as, fh;
    int16 selectedfont;
    int16 sel_line;
    Table *sel_tab;
    struct ttfview *next;
    unsigned int pressed: 1;
} TtfView;

struct freetype_raster {
    int16 rows, cols;
    int16 as, lb;
    int16 bytes_per_row;
    int16 num_greys;
    uint8 *bitmap;
};

typedef struct fontview /* : tableview */ {
    Table *table;		/* Glyph table */
    GWindow gw, v;
    struct tableviewfuncs *virtuals;
    TtfFont *font;		/* for the encoding currently used */
    struct ttfview *owner;
    unsigned int destroyed: 1;		/* window has been destroyed */
/* fontview specials */
    GGadget *mb, *vsb;
    int lpos, rows, cols, boxh;
    int16 as, fh, ifh, ias;
    int16 vheight, vwidth;
    int16 mbh, sbw;
    GFont *gfont, *ifont;
    struct conicfont *cf;
    int info_glyph;
#if TT_RASTERIZE_FONTVIEW
    struct freetype_raster **rasters;
    void *freetype_face;
#endif
} FontView;

struct charshows {
    unsigned int instrpane: 1;		/* Show the character's instructions */
    unsigned int glosspane: 1;		/* Show our gloss on instructions */
    unsigned int fore: 1;		/* Show the character's splines */
    unsigned int grid: 1;		/* Show grid lines */
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    unsigned int gridspline: 1;		/* Show gridfit splines */
    unsigned int raster: 1;		/* Show a bitmap for this grid */
#endif
    uint16 ppem;			/* for the grid/gridfitting */
};

enum byte_types { bt_instr, bt_cnt, bt_byte, bt_wordhi, bt_wordlo };
struct instrinfo {
    GWindow v;
    GGadget *vsb;
    int16 sbw;
    int16 vheight, vwidth;
    int16 lheight,lpos;
    int16 as, fh;
    struct instrdata *instrdata;
    GFont *gfont;
#if TT_CONFIG_OPTION_BYTECODE_DEBUG
    struct ttfargs *args;
    struct ttfactions *acts;
    int act_cnt;
    int gsel_pos;
#endif
    int isel_pos;
    unsigned int changed: 1;
    unsigned int showaddr: 1;
    unsigned int showhex: 1;
};

typedef struct charview {
    ConicChar *cc;
    GWindow gw, v, glossv;
    struct charview *next;
    GGadget *mb, *gvsb, *vsb, *hsb;
    int xoff, yoff, gvpos;
    int16 as, fh, sas, sfh, numlen, infoh;
    int16 vheight, vwidth, gvwidth, gvheight;
    int16 mbh, sbw;
    GFont *gfont, *sfont;
    real scale;
    GPoint mouse;			/* Current mouse point */
    BasePoint info;			/* Expressed in char coordinate system */
    unsigned int destroyed: 1;		/* window has been destroyed */
    struct charshows show;
    struct instrinfo instrinfo;
#if TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    ConicPointList *gridfit;
    struct freetype_raster *raster;
    real gridwidth;
#endif
    Table *cvt;
} CharView;

typedef struct drect {
    real x, y;
    real width, height;
} DRect;

extern void DelayEvent(void (*func)(void *), void *data);
extern TtfView *ViewTtfFont(char *filename);
extern TtfView *TtfViewCreate(TtfFile *tf);
extern void FontNew(void);
extern void MenuOpen(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void TableHelp(int table_name);
extern void MenuHelp(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuPrefs(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuExit(GWindow base,struct gmenuitem *mi,GEvent *e);
int _TFVMenuSaveAs(TtfView *tfv);
int _TFVMenuSave(TtfView *tfv);
int _TFVMenuRevert(TtfView *tfv);
extern void TtfModSetFallback(void);
extern void LoadPrefs(void);
extern void SavePrefs(void);
extern void DoPrefs(void);
extern void RecentFilesRemember(char *filename);
extern unichar_t *FVOpenFont(const unichar_t *title, const unichar_t *defaultfile,
	const unichar_t *initial_filter, unichar_t **mimetypes,int mult,int newok);
void WindowMenuBuild(GWindow base,struct gmenuitem *mi,GEvent *e);
int RecentFilesAny(void);
void MenuRecentBuild(GWindow base,struct gmenuitem *mi,GEvent *e);

void _heaChangeLongMetrics(Table *_hea,int newlongcnt);
void headViewUpdateModifiedCheck(Table *tab);
void MaxPSetStack(Table *maxp,int newval);
void MaxPSetStorage(Table *maxp,int newval);
void MaxPSetFDef(Table *maxp,int newval);

void headCreateEditor(Table *tab,TtfView *tfv);
void maxpCreateEditor(Table *tab,TtfView *tfv);
void postCreateEditor(Table *tab,TtfView *tfv);
void OS2CreateEditor(Table *tab,TtfView *tfv);
void _heaCreateEditor(Table *tab,TtfView *tfv);
void instrCreateEditor(Table *tab,TtfView *tfv);
void binaryCreateEditor(Table *tab,TtfView *tfv);
void shortCreateEditor(Table *tab,TtfView *tfv);
void metricsCreateEditor(Table *tab,TtfView *tfv);
void gaspCreateEditor(Table *tab,TtfView *tfv);
void fontCreateEditor(Table *tab,TtfView *tfv);		/* glyph, loca */

int CVClose(CharView *cv);
void charCreateEditor(ConicFont *cf,int glyph);

void instr_scroll(struct instrinfo *ii,struct sbevent *sb);
void instr_mousemove(struct instrinfo *ii,int pos);
void instr_expose(struct instrinfo *ii,GWindow pixmap,GRect *rect);
void instr_typify(struct instrinfo *instrinfo);
void instrhelpsetup(void);

#if TT_RASTERIZE_FONTVIEW || TT_CONFIG_OPTION_BYTECODE_INTERPRETER
/* Interface routines to FreeType */
struct freetype_raster *FreeType_GetRaster(FontView *fv,int index);
void FreeType_ShowRaster(GWindow pixmap,int x,int y,
			struct freetype_raster *raster);
void FreeType_GridFitChar(CharView *cv);
#endif
void FreeType_FreeRaster(struct freetype_raster *);
#if TT_CONFIG_OPTION_BYTECODE_DEBUG
void CVGenerateGloss(CharView *cv);
#endif
void TtfActionsFree(struct ttfactions *acts);

#endif
