/* Copyright (C) 2002-2007 by George Williams */
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

#ifndef _SFTEXTFIELDP_H
# define _SFTEXTFIELDP_H

#include "../gdraw/ggadgetP.h"

typedef struct fontdata {
    SplineFont *sf;
    enum sftf_fonttype fonttype;
    int pointsize;
    int antialias;
    BDFFont *bdf;
    struct fontdata *next;
    struct fontdata *depends_on;	/* We use much of the ftc allocated for depends_on */
					/* Can't free depends_on until after we get freeed */
    struct _GImage base;
    GImage gi;
    GClut clut;
    struct sfmaps *sfmap;
} FontData;

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
    unsigned int wrap: 1;
    unsigned int _dobitext: 1;	/* has at least one right to left character */
    unsigned int password: 1;
    unsigned int dontdraw: 1;	/* Used when the tf is part of a larger control, and the control determines when to draw the tf */
    unsigned int donthook: 1;	/* Used when the tf is part of a the gchardlg.c */
    unsigned int numericfield: 1;
    unsigned int incr_down: 1;	/* Direction of increments when numeric_scroll events happen */
    unsigned int completionfield: 1;
    unsigned int was_completing: 1;
    uint8 fh;
    uint8 as;
    uint8 nw;			/* Width of one character (an "n") */
    int16 xoff_left, loff_top;
    int16 sel_start, sel_end, sel_base;
    int16 sel_oldstart, sel_oldend, sel_oldbase;
    int16 dd_cursor_pos;
    unichar_t *text, *oldtext;	/* Input glyphs (in unicode) */
    FontInstance *font;		/* pointless */
    GTimer *pressed;
    GTimer *cursor;
    GCursor old_cursor;
    GScrollBar *hsb, *vsb;
    int16 lcnt, lmax;
    struct opentype_str ***lines;	/* pointers into the paras array */
    int16 xmax;
    GIC *gic;
    GTimer *numeric_scroll;
    struct lineheights {
	int32 y;
	int16 as, fh;
	uint16 p, linelen;
	uint32 start_pos;
    } *lineheights;
    struct fontlist {
	int start, end;		/* starting and ending characters [start,end) */
				/*  always break at newline & will omit it between fontlists */
	uint32 *feats;		/* Ends with a 0 entry */
	uint32 script, lang;
	FontData *fd;
	SplineChar **sctext;
	int scmax;
	struct opentype_str *ottext;
	struct fontlist *next;
    } *fontlist, *oldfontlist;
    struct sfmaps {
	SplineFont *sf;
	EncMap *map;
	int16 sfbit_id;
	int16 notdef_gid;
	SplineChar *fake_notdef;
	struct sfmaps *next;
    } *sfmaps;
    struct paras {
	struct opentype_str **para;	/* An array of pointers to ottext entries */
	int start_pos;
    } *paras;
    int pcnt, pmax;
    int ps, pe, ls, le;
    struct fontlist *oldstart, *oldend;
    FontData *generated;
    void *cbcontext;
    void (*changefontcallback)(void *,SplineFont *,enum sftf_fonttype,int size,int aa,
	    uint32 script, uint32 lang, uint32 *features);
    float dpi;
} SFTextArea;

extern SFTextArea *SFTFConvertToPrint(GGadget *g, int width, int height, int dpi);
extern struct sfmaps *SFMapOfSF(SFTextArea *st,SplineFont *sf);

extern struct gfuncs sftextarea_funcs;
#endif
