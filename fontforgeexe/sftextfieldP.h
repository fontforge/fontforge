/* Copyright (C) 2002-2012 by George Williams */
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

#ifndef FONTFORGE_SFTEXTFIELDP_H
#define FONTFORGE_SFTEXTFIELDP_H

#include "../gdraw/ggadgetP.h"
#include "sflayoutP.h"

typedef struct sftextarea {
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
    unsigned int tf_wrap: 1;
    unsigned int _dobitext: 1;	/* has at least one right to left character */
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
    unichar_t *pointless_text, *pointless_oldtext;
    FontInstance *font;		/* pointless */
    GTimer *pressed;
    GTimer *cursor;
    GCursor old_cursor;
    GScrollBar *hsb, *vsb;
    GIC *gic;
    GTimer *numeric_scroll;
    struct layoutinfo li;
    void *cbcontext;
    void (*changefontcallback)(void *,SplineFont *,enum sftf_fonttype,int size,int aa,
	    uint32_t script, uint32_t lang, uint32_t *features);
} SFTextArea;

extern void SFTFRefreshFonts(GGadget *g);
extern void SFTextAreaShow(GGadget *g,int pos);
extern void SFTextAreaSelect(GGadget *g,int start, int end);
extern void SFTextAreaReplace(GGadget *g,const unichar_t *txt);
extern int SFTFSetFontData(GGadget *g, int start, int end, SplineFont *sf,
	enum sftf_fonttype, int size, int antialias);
extern int SFTFSetFont(GGadget *g, int start, int end, SplineFont *sf);
extern int SFTFSetFontType(GGadget *g, int start, int end, enum sftf_fonttype);
extern int SFTFSetSize(GGadget *g, int start, int end, int size);
extern int SFTFSetAntiAlias(GGadget *g, int start, int end, int antialias);
extern int SFTFSetScriptLang(GGadget *g, int start, int end, uint32_t script, uint32_t lang);
extern int SFTFSetFeatures(GGadget *g, int start, int end, uint32_t *features);
extern void SFTFRegisterCallback(GGadget *g, void *cbcontext,
	void (*changefontcallback)(void *,SplineFont *,enum sftf_fonttype,int size,int aa,
		uint32_t script, uint32_t lang, uint32_t *features));
extern void SFTFProvokeCallback(GGadget *g);
extern void  SFTFSetDPI(GGadget *g, float dpi);
extern float SFTFGetDPI(GGadget *g);
extern void SFTFInitLangSys(GGadget *g, int end, uint32_t script, uint32_t lang);
extern GGadget *SFTextAreaCreate(struct gwindow *base, GGadgetData *gd,void *data);
extern void SFTFPopupMenu(SFTextArea *st, GEvent *event);

extern struct gfuncs sftextarea_funcs;

#endif /* FONTFORGE_SFTEXTFIELDP_H */
