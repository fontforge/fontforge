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
#include "ggadget.h"

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
    struct ttfview *next;
    unsigned int pressed: 1;
} TtfView;

extern void DelayEvent(void (*func)(void *), void *data);
extern TtfView *ViewTtfFont(char *filename);
extern TtfView *TtfViewCreate(TtfFile *tf);
extern void FontNew(void);
extern void MenuOpen(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuHelp(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuPrefs(GWindow base,struct gmenuitem *mi,GEvent *e);
int _TFVMenuSaveAs(TtfView *tfv);
int _TFVMenuSave(TtfView *tfv);
extern void TtfModSetFallback(void);
extern void LoadPrefs(void);
extern void DoPrefs(void);
extern unichar_t *FVOpenFont(const unichar_t *title, const unichar_t *defaultfile,
	const unichar_t *initial_filter, unichar_t **mimetypes,int mult,int newok);
void WindowMenuBuild(GWindow base,struct gmenuitem *mi,GEvent *e);
int RecentFilesAny(void);
void MenuRecentBuild(GWindow base,struct gmenuitem *mi,GEvent *e);

void maxpCreateEditor(Table *tab,TtfView *tfv);
#endif
