/* Copyright (C) 2000-2006 by George Williams */
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
#include "pfaeditui.h"
#include <ustring.h>
#include <chardata.h>
#include <utype.h>
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
extern int _GScrollBar_Width;
#include <gkeysym.h>
#include <math.h>

static int last_aspect=0;

struct sortablenames {
    int strid;
    int lang;
    uint8 basedon;		/* Only for certain strids when lang is 0x409 (English US) */
    uint8 cantremove;		/* Names based on PS values will always be output */
    uint16 thislocale;
    char *str;			/* utf8 */
};

struct gfi_data {
    SplineFont *sf;
    GWindow gw, tn_v;
    GFont *tn_font;
    GGadget *tn_smalledit;
    int tn_fh, tn_as;
    int tn_width, tn_height;
    int tn_offleft, tn_offtop, tn_langstart, tn_stridstart, tn_strstart, tn_hend;
    int tn_active, tn_smallactive, tn_done;
    int private_aspect, ttfv_aspect, tn_aspect, tx_aspect;
    int old_sel, old_aspect, old_lang, old_strid;
    int ttf_set, names_set, tex_set;
    struct psdict *private;
    int tn_cnt, tn_max;
    int langlocalecode;	/* MS code for the current locale */
    struct sortablenames *ttfnames;
    unsigned int family_untitled: 1;
    unsigned int human_untitled: 1;
    unsigned int done: 1;
    unsigned int mpdone: 1;
    struct anchor_shows { CharView *cv; SplineChar *sc; int restart; } anchor_shows[2];
    struct texdata texdata;
    struct contextchaindlg *ccd;
    struct statemachinedlg *smd;
/* For GDEF Mark Attachment Class -- used in lookup flags */
/* As usual, class 0 is unused */
    int mark_class_cnt;
    char **mark_classes;		/* glyph name list */
    char **mark_class_names;		/* used within ff */
    struct markclassdlg *mcd;
};

GTextInfo emsizes[] = {
    { (unichar_t *) "1000", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "1024", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "2048", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "4096", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};

GTextInfo interpretations[] = {
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "None", ignore "Interpretation|" */
/* GT: In french this could be "Aucun" or "Aucune" depending on the gender */
    { (unichar_t *) N_("Interpretation|None"), NULL, 0, 0, (void *) ui_none, NULL, 0, 0, 0, 0, 0, 0, 1},
/*  { (unichar_t *) N_("Adobe Public Use Defs."), NULL, 0, 0, (void *) ui_adobe, NULL, 0, 0, 0, 0, 0, 0, 1}, */
/*  { (unichar_t *) N_("Greek"), NULL, 0, 0, (void *) ui_greek, NULL, 0, 0, 0, 0, 0, 0, 1}, */
    { (unichar_t *) N_("Japanese"), NULL, 0, 0, (void *) ui_japanese, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Traditional Chinese"), NULL, 0, 0, (void *) ui_trad_chinese, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Simplified Chinese"), NULL, 0, 0, (void *) ui_simp_chinese, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Korean"), NULL, 0, 0, (void *) ui_korean, NULL, 0, 0, 0, 0, 0, 0, 1},
/*  { (unichar_t *) N_("AMS Public Use"), NULL, 0, 0, (void *) ui_ams, NULL, 0, 0, 0, 0, 0, 0, 1}, */
    { NULL }};
GTextInfo macstyles[] = {
    { (unichar_t *) N_("Bold"), NULL, 0, 0, (void *) sf_bold, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Italic"), NULL, 0, 0, (void *) sf_italic, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Condense"), NULL, 0, 0, (void *) sf_condense, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Expand"), NULL, 0, 0, (void *) sf_extend, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Underline"), NULL, 0, 0, (void *) sf_underline, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Outline"), NULL, 0, 0, (void *) sf_outline, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Shadow"), NULL, 0, 0, (void *) sf_shadow, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo widthclass[] = {
    { (unichar_t *) N_("Ultra-Condensed (50%)"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Extra-Condensed (62.5%)"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Condensed (75%)"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Semi-Condensed (87.5%)"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Medium (100%)"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Semi-Expanded (112.5%)"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Expanded (125%)"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Extra-Expanded (150%)"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ultra-Expanded (200%)"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo weightclass[] = {
    { (unichar_t *) N_("100 Thin"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("200 Extra-Light"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("300 Light"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("400 Book"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("500 Medium"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("600 Demi-Bold"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("700 Bold"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("800 Heavy"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("900 Black"), NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo fstype[] = {
    { (unichar_t *) N_("Never Embed/No Editing"), NULL, 0, 0, (void *) 0x02, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Printable Document"), NULL, 0, 0, (void *) 0x04, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Editable Document"), NULL, 0, 0, (void *) 0x08, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Installable Font"), NULL, 0, 0, (void *) 0x00, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo pfmfamily[] = {
    { (unichar_t *) N_("Serif"), NULL, 0, 0, (void *) 0x11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif"), NULL, 0, 0, (void *) 0x21, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Monospace"), NULL, 0, 0, (void *) 0x31, NULL, 0, 0, 0, 0, 0, 0, 1},
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Script", ignore "cursive|" */
/* GT: English uses "script" to me a general writing style (latin, greek, kanji) */
/* GT: and the cursive handwriting style. Here we mean cursive handwriting. */
    { (unichar_t *) N_("cursive|Script"), NULL, 0, 0, (void *) 0x41, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Decorative"), NULL, 0, 0, (void *) 0x51, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo ibmfamily[] = {
    { (unichar_t *) N_("No Classification"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Old Style Serifs"), NULL, 0, 0, (void *) 0x100, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Rounded Legibility"), NULL, 0, 0, (void *) 0x101, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Geralde"), NULL, 0, 0, (void *) 0x102, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Venetian"), NULL, 0, 0, (void *) 0x103, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Modified Venetian"), NULL, 0, 0, (void *) 0x104, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Dutch Modern"), NULL, 0, 0, (void *) 0x105, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Dutch Trad"), NULL, 0, 0, (void *) 0x106, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Contemporary"), NULL, 0, 0, (void *) 0x107, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Calligraphic"), NULL, 0, 0, (void *) 0x108, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("OSS Miscellaneous"), NULL, 0, 0, (void *) 0x10f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Transitional Serifs"), NULL, 0, 0, (void *) 0x200, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("TS Direct Line"), NULL, 0, 0, (void *) 0x201, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("TS Script"), NULL, 0, 0, (void *) 0x202, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("TS Miscellaneous"), NULL, 0, 0, (void *) 0x20f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Modern Serifs"), NULL, 0, 0, (void *) 0x300, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MS Italian"), NULL, 0, 0, (void *) 0x301, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MS Script"), NULL, 0, 0, (void *) 0x302, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("MS Miscellaneous"), NULL, 0, 0, (void *) 0x30f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Clarendon Serifs"), NULL, 0, 0, (void *) 0x400, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Clarendon"), NULL, 0, 0, (void *) 0x401, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Modern"), NULL, 0, 0, (void *) 0x402, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Traditional"), NULL, 0, 0, (void *) 0x403, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Newspaper"), NULL, 0, 0, (void *) 0x404, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Stub Serif"), NULL, 0, 0, (void *) 0x405, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Monotone"), NULL, 0, 0, (void *) 0x406, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Typewriter"), NULL, 0, 0, (void *) 0x407, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CS Miscellaneous"), NULL, 0, 0, (void *) 0x40f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slab Serifs"), NULL, 0, 0, (void *) 0x500, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Monotone"), NULL, 0, 0, (void *) 0x501, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Humanist"), NULL, 0, 0, (void *) 0x502, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Geometric"), NULL, 0, 0, (void *) 0x503, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Swiss"), NULL, 0, 0, (void *) 0x504, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Typewriter"), NULL, 0, 0, (void *) 0x505, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Miscellaneous"), NULL, 0, 0, (void *) 0x50f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Freeform Serifs"), NULL, 0, 0, (void *) 0x700, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("FS Modern"), NULL, 0, 0, (void *) 0x701, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("FS Miscellaneous"), NULL, 0, 0, (void *) 0x70f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sans-Serif"), NULL, 0, 0, (void *) 0x800, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS IBM NeoGrotesque Gothic"), NULL, 0, 0, (void *) 0x801, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Humanist"), NULL, 0, 0, (void *) 0x802, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Low-x Round Geometric"), NULL, 0, 0, (void *) 0x803, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS High-x Round Geometric"), NULL, 0, 0, (void *) 0x804, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS NeoGrotesque Gothic"), NULL, 0, 0, (void *) 0x805, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Modified Grotesque Gothic"), NULL, 0, 0, (void *) 0x806, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Typewriter Gothic"), NULL, 0, 0, (void *) 0x809, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Matrix"), NULL, 0, 0, (void *) 0x80a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("SS Miscellaneous"), NULL, 0, 0, (void *) 0x80f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ornamentals"), NULL, 0, 0, (void *) 0x900, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("O Engraver"), NULL, 0, 0, (void *) 0x901, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("O Black Letter"), NULL, 0, 0, (void *) 0x902, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("O Decorative"), NULL, 0, 0, (void *) 0x903, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("O Three Dimensional"), NULL, 0, 0, (void *) 0x904, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("O Miscellaneous"), NULL, 0, 0, (void *) 0x90f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Scripts"), NULL, 0, 0, (void *) 0xa00, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Uncial"), NULL, 0, 0, (void *) 0xa01, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Brush Joined"), NULL, 0, 0, (void *) 0xa02, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Formal Joined"), NULL, 0, 0, (void *) 0xa03, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Monotone Joined"), NULL, 0, 0, (void *) 0xa04, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Calligraphic"), NULL, 0, 0, (void *) 0xa05, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Brush Unjoined"), NULL, 0, 0, (void *) 0xa06, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Formal Unjoined"), NULL, 0, 0, (void *) 0xa07, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Monotone Unjoined"), NULL, 0, 0, (void *) 0xa08, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("S Miscellaneous"), NULL, 0, 0, (void *) 0xa0f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Symbolic"), NULL, 0, 0, (void *) 0xc00, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sy Mixed Serif"), NULL, 0, 0, (void *) 0xc03, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sy Old Style Serif"), NULL, 0, 0, (void *) 0xc06, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sy Neo-grotesque Sans Serif"), NULL, 0, 0, (void *) 0xc07, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sy Miscellaneous"), NULL, 0, 0, (void *) 0xc0f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panfamily[] = {
    { (unichar_t *) N_("Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Text & Display"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Script", ignore "cursive|" */
/* GT: English uses "script" to me a general writing style (latin, greek, kanji) */
/* GT: and the cursive handwriting style. Here we mean cursive handwriting. */
    { (unichar_t *) N_("cursive|Script"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Decorative"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Pictorial"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "6", NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "7", NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panserifs[] = {
    { (unichar_t *) N_("Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Cove"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Obtuse Cove"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Square Cove"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Obtuse Square Cove"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Square"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Thin"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Bone"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Exaggerated"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Triangle"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal Sans"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Obtuse Sans"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Perp Sans"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Flared"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Rounded"), NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panweight[] = {
    { (unichar_t *) N_("Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Very Light"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Light"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Thin"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Book"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Medium"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Demi"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Bold"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Heavy"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Black"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Nord"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panprop[] = {
    { (unichar_t *) N_("Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Old Style"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Modern"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Even Width"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Expanded"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Condensed"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Very Expanded"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Very Condensed"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Monospaced"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo pancontrast[] = {
    { (unichar_t *) N_("Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("None"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Very Low"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Low"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Medium Low"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Medium"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Medium High"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("High"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Very High"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panstrokevar[] = {
    { (unichar_t *) N_("Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gradual/Diagonal"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gradual/Transitional"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gradual/Vertical"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gradual/Horizontal"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Rapid/Vertical"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Rapid/Horizontal"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Instant/Vertical"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panarmstyle[] = {
    { (unichar_t *) N_("Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Straight Arms/Horizontal"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Straight Arms/Wedge"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Straight Arms/Vertical"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Straight Arms/Single Serif"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Straight Arms/Double Serif"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Non-Straight Arms/Horizontal"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Non-Straight Arms/Wedge"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Non-Straight Arms/Vertical"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Non-Straight Arms/Single Serif"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Non-Straight Arms/Double Serif"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panletterform[] = {
    { (unichar_t *) N_("Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Contact"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Weighted"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Boxed"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Flattened"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Rounded"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Off-Center"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Normal/Square"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Contact"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Weighted"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Boxed"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Rounded"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Off-Center"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oblique/Square"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panmidline[] = {
    { (unichar_t *) N_("Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Standard/Trimmed"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Standard/Pointed"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Standard/Serifed"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("High/Trimmed"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("High/Pointed"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("High/Serifed"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Constant/Trimmed"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Constant/Pointed"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Constant/Serifed"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Low/Trimmed"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Low/Pointed"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Low/Serifed"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panxheight[] = {
    { (unichar_t *) N_("Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Constant/Small"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Constant/Standard"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Constant/Large"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ducking/Small"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ducking/Standard"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ducking/Large"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static struct ms_2_locales { char *loc_name; int local_id; } ms_2_locals[] = {
    { "af", 0x436 },
    { "sq_AL", 0x41c },
    { "am", 0x45e },
    { "ar_SA", 0x401 },
    { "ar_IQ", 0x801 },
    { "ar_EG", 0xc01 },
    { "ar_LY", 0x1001 },
    { "ar_DZ", 0x1401 },
    { "ar_MA", 0x1801 },
    { "ar_TN", 0x1C01 },
    { "ar_OM", 0x2001 },
    { "ar_YE", 0x2401 },
    { "ar_SY", 0x2801 },
    { "ar_JO", 0x2c01 },
    { "ar_LB", 0x3001 },
    { "ar_KW", 0x3401 },
    { "ar_AE", 0x3801 },
    { "ar_BH", 0x3c01 },
    { "ar_QA", 0x4001 },
    { "hy", 0x42b },
    { "as", 0x44d },
    { "az", 0x42c },
    { "az", 0x82c },
    { "eu", 0x42d },
    { "be_BY", 0x423 },
    { "bn_IN", 0x445 },
    { "bn_BD", 0x845 },
    { "bg_BG", 0x402 },
    { "my", 0x455 },
    { "ca", 0x403 },
    { "km", 0x453 },
    { "zh_TW", 0x404 },		/* Trad */
    { "zh_CN", 0x804 },		/* Simp */
    { "zh_HK", 0xc04 },		/* Trad */
    { "zh_SG", 0x1004 },	/* Simp */
    { "zh_MO", 0x1404 },	/* Trad */
    { "hr", 0x41a },
    { "hr_BA", 0x101a },
    { "cs_CZ", 0x405 },
    { "da_DK", 0x406 },
    { "div", 0x465 },
    { "nl_NL", 0x413 },
    { "nl_BE", 0x813 },
    { "en_UK", 0x809 },
    { "en_US", 0x409 },
    { "en_CA", 0x1009 },
    { "en_AU", 0xc09 },
    { "en_NZ", 0x1409 },
    { "en_IE", 0x1809 },
    { "en_ZA", 0x1c09 },
    { "en_JM", 0x2009 },
    { "en", 0x2409 },
    { "en_BZ", 0x2809 },
    { "en_TT", 0x2c09 },
    { "en_ZW", 0x3009 },
    { "en_PH", 0x3409 },
    { "en_ID", 0x3809 },
    { "en_HK", 0x3c09 },
    { "en_IN", 0x4009 },
    { "en_MY", 0x4409 },
    { "et_EE", 0x425 },
    { "fo", 0x438 },
/* No language code for filipino */
    { "fa", 0x429 },
    { "fi_FI", 0x40b },
    { "fr_FR", 0x40c },
    { "fr_BE", 0x80c },
    { "fr_CA", 0xc0c },
    { "fr_CH", 0x100c },
    { "fr_LU", 0x140c },
    { "fr_MC", 0x180c },
    { "fr", 0x1c0c },		/* West Indes */
    { "fr_RE", 0x200c },
    { "fr_CD", 0x240c },
    { "fr_SN", 0x280c },
    { "fr_CM", 0x2c0c },
    { "fr_CI", 0x300c },
    { "fr_ML", 0x340c },
    { "fr_MA", 0x380c },
    { "fr_HT", 0x3c0c },
    { "fr_DZ", 0xe40c },	/* North African is most likely to be Algeria, possibly Tunisia */
    { "fy", 0x462 },
    { "gl", 0x456 },
    { "ka", 0x437 },
    { "de_DE", 0x407 },
    { "de_CH", 0x807 },
    { "de_AT", 0xc07 },
    { "de_LU", 0x1007 },
    { "de_LI", 0x1407 },
    { "el_GR", 0x408 },
    { "ga", 0x83c },
    { "gd", 0x43c },
    { "gn", 0x474 },
    { "gu", 0x447 },
    { "ha", 0x468 },
    { "he_IL", 0x40d },
    { "iw", 0x40d },		/* Obsolete name for Hebrew */
    { "hi", 0x439 },
    { "hu_HU", 0x40e },
    { "is_IS", 0x40f },
    { "id", 0x421 },
    { "in", 0x421 },		/* Obsolete name for Indonesean */
    { "iu", 0x45d },
    { "it_IT", 0x410 },
    { "it_CH", 0x810 },
    { "ja_JP", 0x411 },
    { "kn", 0x44b },
    { "ks_IN", 0x860 },
    { "kk", 0x43f },
    { "ky", 0x440 },
    { "km", 0x453 },
    { "kok", 0x457 },
    { "ko", 0x412 },
    { "ko", 0x812 },	/*Johab */
    { "lo", 0x454 },
    { "la", 0x476 },
    { "lv_LV", 0x426 },
    { "lt_LT", 0x427 },
    { "lt", 0x827 },	/* Classic */
    { "mk", 0x42f },
    { "ms", 0x43e },
    { "ms", 0x83e },
    { "ml", 0x44c },
    { "mt", 0x43a },
    { "mr", 0x44e },
    { "mn", 0x450 },
    { "ne_NP", 0x461 },
    { "ne_IN", 0x861 },
    { "no_NO", 0x414 },	/* Bokmal */
    { "no_NO", 0x814 },	/* Nynorsk */
    { "or", 0x448 },
    { "om", 0x472 },
    { "ps", 0x463 },
    { "pl_PL", 0x415 },
    { "pt_PT", 0x416 },
    { "pt_BR", 0x816 },
    { "pa_IN", 0x446 },
    { "pa_PK", 0x846 },
    { "qu_BO", 0x46b },
    { "qu_EC", 0x86b },
    { "qu_PE", 0xc6b },
    { "rm", 0x417 },
    { "ro_RO", 0x418 },
    { "ro_MD", 0x818 },
    { "ru_RU", 0x419 },
    { "ru_MD", 0x819 },
    { "smi", 0x43b },
    { "sa", 0x43b },
/* No language code for Sepedi */
    { "sr", 0xc1a },	/* Cyrillic */
    { "sr", 0x81a },	/* Latin */
    { "sd_IN", 0x459 },
    { "sd_PK", 0x859 },
    { "si", 0x45b },
    { "sk_SK", 0x41b },
    { "sl_SI", 0x424 },
    { "wen", 0x42e },
    { "es_ES", 0x40a },	/* traditional spanish */
    { "es_MX", 0x80a },
    { "es_ES", 0xc0a },	/* Modern spanish */
    { "es_GT", 0x100a },
    { "es_CR", 0x140a },
    { "es_PA", 0x180a },
    { "es_DO", 0x1c0a },
    { "es_VE", 0x200a },
    { "es_CO", 0x240a },
    { "es_PE", 0x280a },
    { "es_AR", 0x2c0a },
    { "es_EC", 0x300a },
    { "es_CL", 0x340a },
    { "es_UY", 0x380a },
    { "es_PY", 0x3c0a },
    { "es_BO", 0x400a },
    { "es_SV", 0x440a },
    { "es_HN", 0x480a },
    { "es_NI", 0x4c0a },
    { "es_PR", 0x500a },
    { "es_US", 0x540a },
    { "sutu", 0x430 },
    { "sw_KE", 0x441 },
    { "sv_SE", 0x41d },
    { "sv_FI", 0x81d },
    { "tl", 0x464 },
    { "tg", 0x464 },
    { "ta", 0x449 },
    { "tt", 0x444 },
    { "te", 0x44a },
    { "th", 0x41e },
    { "bo_CN", 0x451 },
    { "bo_BT", 0x451 },
    { "ti_ET", 0x473 },
    { "ti_ER", 0x873 },
    { "ts", 0x431 },
    { "tn", 0x432 },
    { "tr_TR", 0x41f },
    { "tk", 0x442 },
    { "uk_UA", 0x422 },
    { "ug", 0x480 },
    { "ur_PK", 0x420 },
    { "ur_IN", 0x820 },
    { "uz", 0x443 },	/* Latin */
    { "uz", 0x843 },	/* Cyrillic */
    { "ven", 0x433 },
    { "vi", 0x42a },
    { "cy", 0x452 },
    { "xh", 0x434 },
    { "yi", 0x43d },
    { "ji", 0x43d },	/* Obsolete Yiddish */
    { "yo", 0x46a },
    { "zu", 0x435 },
    { NULL }};
static GTextInfo mslanguages[] = {
    { (unichar_t *) N_("Afrikaans"), NULL, 0, 0, (void *) 0x436, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Albanian"), NULL, 0, 0, (void *) 0x41c, NULL, 0, 0, 0, 0, 0, 0, 1},
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Malayalam", ignore "Lang|" */
    { (unichar_t *) N_("Lang|Amharic"), NULL, 0, 0, (void *) 0x45e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Saudi Arabia"), NULL, 0, 0, (void *) 0x401, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Iraq"), NULL, 0, 0, (void *) 0x801, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Egypt"), NULL, 0, 0, (void *) 0xc01, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Libya"), NULL, 0, 0, (void *) 0x1001, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Algeria"), NULL, 0, 0, (void *) 0x1401, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Morocco"), NULL, 0, 0, (void *) 0x1801, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Tunisia"), NULL, 0, 0, (void *) 0x1C01, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Oman"), NULL, 0, 0, (void *) 0x2001, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Yemen"), NULL, 0, 0, (void *) 0x2401, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Syria"), NULL, 0, 0, (void *) 0x2801, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Jordan"), NULL, 0, 0, (void *) 0x2c01, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Lebanon"), NULL, 0, 0, (void *) 0x3001, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Kuwait"), NULL, 0, 0, (void *) 0x3401, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic U.A.E."), NULL, 0, 0, (void *) 0x3801, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Bahrain"), NULL, 0, 0, (void *) 0x3c01, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Arabic Qatar"), NULL, 0, 0, (void *) 0x4001, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Armenian"), NULL, 0, 0, (void *) 0x42b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Assamese"), NULL, 0, 0, (void *) 0x44d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Azeri (Latin)"), NULL, 0, 0, (void *) 0x42c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Azeri (Cyrillic)"), NULL, 0, 0, (void *) 0x82c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Basque"), NULL, 0, 0, (void *) 0x42d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Byelorussian"), NULL, 0, 0, (void *) 0x423, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Bengali"), NULL, 0, 0, (void *) 0x445, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Bengali Bangladesh"), NULL, 0, 0, (void *) 0x845, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Bulgarian"), NULL, 0, 0, (void *) 0x402, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Burmese"), NULL, 0, 0, (void *) 0x455, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Catalan"), NULL, 0, 0, (void *) 0x403, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Cambodian"), NULL, 0, 0, (void *) 0x453, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Cherokee"), NULL, 0, 0, (void *) 0x45c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Chinese (Taiwan)"), NULL, 0, 0, (void *) 0x404, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Chinese (PRC)"), NULL, 0, 0, (void *) 0x804, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Chinese (Hong Kong)"), NULL, 0, 0, (void *) 0xc04, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Chinese (Singapore)"), NULL, 0, 0, (void *) 0x1004, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Chinese (Macau)"), NULL, 0, 0, (void *) 0x1404, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Croatian"), NULL, 0, 0, (void *) 0x41a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Croatian Bosnia/Herzegovina"), NULL, 0, 0, (void *) 0x101a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Czech"), NULL, 0, 0, (void *) 0x405, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Danish"), NULL, 0, 0, (void *) 0x406, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Divehi"), NULL, 0, 0, (void *) 0x465, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Dutch"), NULL, 0, 0, (void *) 0x413, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Flemish (Belgian Dutch)"), NULL, 0, 0, (void *) 0x813, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Edo"), NULL, 0, 0, (void *) 0x466, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (British)"), NULL, 0, 0, (void *) 0x809, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (US)"), NULL, 0, 0, (void *) 0x409, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Canada)"), NULL, 0, 0, (void *) 0x1009, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Australian)"), NULL, 0, 0, (void *) 0xc09, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (New Zeeland)"), NULL, 0, 0, (void *) 0x1409, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Irish)"), NULL, 0, 0, (void *) 0x1809, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (South Africa)"), NULL, 0, 0, (void *) 0x1c09, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Jamaica)"), NULL, 0, 0, (void *) 0x2009, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Caribbean)"), NULL, 0, 0, (void *) 0x2409, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Belize)"), NULL, 0, 0, (void *) 0x2809, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Trinidad)"), NULL, 0, 0, (void *) 0x2c09, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Zimbabwe)"), NULL, 0, 0, (void *) 0x3009, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Philippines)"), NULL, 0, 0, (void *) 0x3409, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Indonesia)"), NULL, 0, 0, (void *) 0x3809, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Hong Kong)"), NULL, 0, 0, (void *) 0x3c09, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (India)"), NULL, 0, 0, (void *) 0x4009, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("English (Malaysia)"), NULL, 0, 0, (void *) 0x4409, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Estonian"), NULL, 0, 0, (void *) 0x425, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Faeroese"), NULL, 0, 0, (void *) 0x438, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Farsi"), NULL, 0, 0, (void *) 0x429, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Filipino"), NULL, 0, 0, (void *) 0x464, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Finnish"), NULL, 0, 0, (void *) 0x40b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French French"), NULL, 0, 0, (void *) 0x40c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Belgium"), NULL, 0, 0, (void *) 0x80c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Canadian"), NULL, 0, 0, (void *) 0xc0c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Swiss"), NULL, 0, 0, (void *) 0x100c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Luxembourg"), NULL, 0, 0, (void *) 0x140c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Monaco"), NULL, 0, 0, (void *) 0x180c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French West Indies"), NULL, 0, 0, (void *) 0x1c0c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("French Réunion"), NULL, 0, 0, (void *) 0x200c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French D.R. Congo"), NULL, 0, 0, (void *) 0x240c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Senegal"), NULL, 0, 0, (void *) 0x280c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Camaroon"), NULL, 0, 0, (void *) 0x2c0c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("French Côte d'Ivoire"), NULL, 0, 0, (void *) 0x300c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Mali"), NULL, 0, 0, (void *) 0x340c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Morocco"), NULL, 0, 0, (void *) 0x380c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French Haiti"), NULL, 0, 0, (void *) 0x3c0c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("French North Africa"), NULL, 0, 0, (void *) 0xe40c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Frisian"), NULL, 0, 0, (void *) 0x462, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Fulfulde"), NULL, 0, 0, (void *) 0x467, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gaelic (Scottish)"), NULL, 0, 0, (void *) 0x43c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Gaelic (Irish)"), NULL, 0, 0, (void *) 0x83c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Galician"), NULL, 0, 0, (void *) 0x467, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Georgian"), NULL, 0, 0, (void *) 0x437, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("German German"), NULL, 0, 0, (void *) 0x407, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("German Swiss"), NULL, 0, 0, (void *) 0x807, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("German Austrian"), NULL, 0, 0, (void *) 0xc07, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("German Luxembourg"), NULL, 0, 0, (void *) 0x1007, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("German Liechtenstein"), NULL, 0, 0, (void *) 0x1407, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Greek"), NULL, 0, 0, (void *) 0x408, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Guarani"), NULL, 0, 0, (void *) 0x474, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Gujarati"), NULL, 0, 0, (void *) 0x447, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Hausa"), NULL, 0, 0, (void *) 0x468, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Hawaiian"), NULL, 0, 0, (void *) 0x475, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Hebrew"), NULL, 0, 0, (void *) 0x40d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Hindi"), NULL, 0, 0, (void *) 0x439, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Hungarian"), NULL, 0, 0, (void *) 0x40e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ibibio"), NULL, 0, 0, (void *) 0x469, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Icelandic"), NULL, 0, 0, (void *) 0x40f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Igbo"), NULL, 0, 0, (void *) 0x470, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Indonesian"), NULL, 0, 0, (void *) 0x421, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Inuktitut"), NULL, 0, 0, (void *) 0x45d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Italian"), NULL, 0, 0, (void *) 0x410, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Italian Swiss"), NULL, 0, 0, (void *) 0x810, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Japanese"), NULL, 0, 0, (void *) 0x411, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Kannada"), NULL, 0, 0, (void *) 0x44b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Kanuri"), NULL, 0, 0, (void *) 0x471, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Kashmiri (India)"), NULL, 0, 0, (void *) 0x860, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Kazakh"), NULL, 0, 0, (void *) 0x43f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Khmer"), NULL, 0, 0, (void *) 0x453, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Kirghiz"), NULL, 0, 0, (void *) 0x440, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Konkani"), NULL, 0, 0, (void *) 0x457, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Korean"), NULL, 0, 0, (void *) 0x412, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Korean (Johab)"), NULL, 0, 0, (void *) 0x812, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lao"), NULL, 0, 0, (void *) 0x454, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Latvian"), NULL, 0, 0, (void *) 0x426, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Latin"), NULL, 0, 0, (void *) 0x476, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lithuanian"), NULL, 0, 0, (void *) 0x427, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lithuanian (Classic)"), NULL, 0, 0, (void *) 0x827, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Macedonian"), NULL, 0, 0, (void *) 0x42f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Malay"), NULL, 0, 0, (void *) 0x43e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Malay (Brunei)"), NULL, 0, 0, (void *) 0x83e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Malayalam"), NULL, 0, 0, (void *) 0x44c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Maltese"), NULL, 0, 0, (void *) 0x43a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Manipuri"), NULL, 0, 0, (void *) 0x458, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Maori"), NULL, 0, 0, (void *) 0x481, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Marathi"), NULL, 0, 0, (void *) 0x44e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Mongolian (Cyrillic)"), NULL, 0, 0, (void *) 0x450, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Mongolian (Mongolian)"), NULL, 0, 0, (void *) 0x850, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Nepali"), NULL, 0, 0, (void *) 0x461, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Nepali (India)"), NULL, 0, 0, (void *) 0x861, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Norwegian (Bokmal)"), NULL, 0, 0, (void *) 0x414, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Norwegian (Nynorsk)"), NULL, 0, 0, (void *) 0x814, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Oriya"), NULL, 0, 0, (void *) 0x448, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Oromo"), NULL, 0, 0, (void *) 0x472, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Papiamentu"), NULL, 0, 0, (void *) 0x479, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Pashto"), NULL, 0, 0, (void *) 0x463, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Polish"), NULL, 0, 0, (void *) 0x415, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Portugese (Portugal)"), NULL, 0, 0, (void *) 0x416, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Portuguese (Brasil)"), NULL, 0, 0, (void *) 0x816, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Punjabi (India"), NULL, 0, 0, (void *) 0x446, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Punjabi (Pakistan"), NULL, 0, 0, (void *) 0x846, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Quecha (Bolivia)"), NULL, 0, 0, (void *) 0x46b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Quecha (Ecuador)"), NULL, 0, 0, (void *) 0x86b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Quecha (Peru)"), NULL, 0, 0, (void *) 0xc6b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Rhaeto-Romanic"), NULL, 0, 0, (void *) 0x417, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Romanian"), NULL, 0, 0, (void *) 0x418, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Romanian (Moldova)"), NULL, 0, 0, (void *) 0x818, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Russian"), NULL, 0, 0, (void *) 0x419, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Russian (Moldova)"), NULL, 0, 0, (void *) 0x819, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sami (Lappish)"), NULL, 0, 0, (void *) 0x43b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sanskrit"), NULL, 0, 0, (void *) 0x43b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sepedi"), NULL, 0, 0, (void *) 0x46c, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Serbian (Cyrillic)"), NULL, 0, 0, (void *) 0xc1a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Serbian (Latin)"), NULL, 0, 0, (void *) 0x81a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sindhi India"), NULL, 0, 0, (void *) 0x459, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sindhi Pakistan"), NULL, 0, 0, (void *) 0x859, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Sinhalese"), NULL, 0, 0, (void *) 0x45b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slovak"), NULL, 0, 0, (void *) 0x41b, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Slovenian"), NULL, 0, 0, (void *) 0x424, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sorbian"), NULL, 0, 0, (void *) 0x42e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Traditional)"), NULL, 0, 0, (void *) 0x40a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish Mexico"), NULL, 0, 0, (void *) 0x80a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Modern)"), NULL, 0, 0, (void *) 0xc0a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Guatemala)"), NULL, 0, 0, (void *) 0x100a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Costa Rica)"), NULL, 0, 0, (void *) 0x140a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Panama)"), NULL, 0, 0, (void *) 0x180a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Dominican Republic)"), NULL, 0, 0, (void *) 0x1c0a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Venezuela)"), NULL, 0, 0, (void *) 0x200a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Colombia)"), NULL, 0, 0, (void *) 0x240a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Peru)"), NULL, 0, 0, (void *) 0x280a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Argentina)"), NULL, 0, 0, (void *) 0x2c0a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Ecuador)"), NULL, 0, 0, (void *) 0x300a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Chile)"), NULL, 0, 0, (void *) 0x340a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Uruguay)"), NULL, 0, 0, (void *) 0x380a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Paraguay)"), NULL, 0, 0, (void *) 0x3c0a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Bolivia)"), NULL, 0, 0, (void *) 0x400a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (El Salvador)"), NULL, 0, 0, (void *) 0x440a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Honduras)"), NULL, 0, 0, (void *) 0x480a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Nicaragua)"), NULL, 0, 0, (void *) 0x4c0a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Puerto Rico)"), NULL, 0, 0, (void *) 0x500a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (United States)"), NULL, 0, 0, (void *) 0x540a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Spanish (Latin America)"), NULL, 0, 0, (void *) 0xe40a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sutu"), NULL, 0, 0, (void *) 0x430, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Swahili (Kenyan)"), NULL, 0, 0, (void *) 0x441, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Swedish (Sweden)"), NULL, 0, 0, (void *) 0x41d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Swedish (Finland)"), NULL, 0, 0, (void *) 0x81d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Syriac"), NULL, 0, 0, (void *) 0x45a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Tagalog"), NULL, 0, 0, (void *) 0x464, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tajik"), NULL, 0, 0, (void *) 0x428, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tamazight (Arabic)"), NULL, 0, 0, (void *) 0x45f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tamazight (Latin)"), NULL, 0, 0, (void *) 0x85f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Tamil"), NULL, 0, 0, (void *) 0x449, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tatar (Tatarstan)"), NULL, 0, 0, (void *) 0x444, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Telugu"), NULL, 0, 0, (void *) 0x44a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Thai"), NULL, 0, 0, (void *) 0x41e, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tibetan (PRC)"), NULL, 0, 0, (void *) 0x451, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tibetan Bhutan"), NULL, 0, 0, (void *) 0x851, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tigrinya Ethiopia"), NULL, 0, 0, (void *) 0x473, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tigrinyan Eritrea"), NULL, 0, 0, (void *) 0x873, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tsonga"), NULL, 0, 0, (void *) 0x431, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Tswana"), NULL, 0, 0, (void *) 0x432, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Turkish"), NULL, 0, 0, (void *) 0x41f, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Turkmen"), NULL, 0, 0, (void *) 0x442, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Uighur"), NULL, 0, 0, (void *) 0x480, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Ukrainian"), NULL, 0, 0, (void *) 0x422, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Urdu (Pakistan)"), NULL, 0, 0, (void *) 0x420, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Urdu (India)"), NULL, 0, 0, (void *) 0x820, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Uzbek (Latin)"), NULL, 0, 0, (void *) 0x443, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Uzbek (Cyrillic)"), NULL, 0, 0, (void *) 0x843, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Venda"), NULL, 0, 0, (void *) 0x433, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Vietnamese"), NULL, 0, 0, (void *) 0x42a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Welsh"), NULL, 0, 0, (void *) 0x452, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Xhosa"), NULL, 0, 0, (void *) 0x434, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Lang|Yi"), NULL, 0, 0, (void *) 0x478, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Yiddish"), NULL, 0, 0, (void *) 0x43d, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Yoruba"), NULL, 0, 0, (void *) 0x46a, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Zulu"), NULL, 0, 0, (void *) 0x435, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo ttfnameids[] = {
/* Put styles (docs call it subfamily) first because it is most likely to change */
    { (unichar_t *) N_("Styles (SubFamily)"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 1, 0, 1},
    { (unichar_t *) N_("Copyright"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Family"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Fullname"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("UniqueID"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Version"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1},
/* Don't bother with PostscriptName, we set that elsewhere */
    { (unichar_t *) N_("Trademark"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Manufacturer"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Designer"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Descriptor"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Vendor URL"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Designer URL"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("License"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("License URL"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1},
/* slot 15 is reserved */
    { (unichar_t *) N_("Preferred Family"), NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Preferred Styles"), NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Compatible Full"), NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Sample Text"), NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("CID findfont Name"), NULL, 0, 0, (void *) 20, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

struct langstyle { int lang; const char *str; };
static const char regulareng[] = "Regular";
static const char demiboldeng[] = "DemiBold";
static const char demiboldeng3[] = "Demi";
static const char demiboldeng5[] = "SemiBold";
static const char boldeng[] = "Bold";
static const char thineng[] = "Thin";
static const char lighteng[] = "Light";
static const char extralighteng[] = "ExtraLight";
static const char extralighteng2[] = "VeryLight";
static const char mediumeng[] = "Medium";
static const char bookeng[] = "Book";
static const char heavyeng[] = "Heavy";
static const char blackeng[] = "Black";
static const char italiceng[] = "Italic";
static const char obliqueeng[] = "Oblique";
static const char condensedeng[] = "Condensed";
static const char expandedeng[] = "Expanded";
static const char outlineeng[] = "Outline";

static const char regularfren[] = "Normal";
static const char boldfren[] = "Gras";
static const char demiboldfren[] = "DemiGras";
static const char demiboldfren2[] = "Demi";
static const char mediumfren[] = "Normal";
static const char lightfren[] = "Maigre";
static const char blackfren[] = "ExtraGras";
static const char italicfren[] = "Italique";
static const char obliquefren[] = "Oblique";
static const char condensedfren[] = "Etroite";
static const char expandedfren[] = "Elargi";
static const char outlinefren[] = "Contour";

static const char regulargerm[] = "Standard";
static const char demiboldgerm[] = "Halbfett";
static const char demiboldgerm2[] = "Schmallfett";
static const char boldgerm[] = "Fett";
static const char boldgerm2[] = "Dick";
static const char blackgerm[] = "Schwarz";
static const char lightgerm[] = "Mager";
static const char mediumgerm[] = "Mittel";
static const char bookgerm[] = "Buchschrift";
static const char italicgerm[] = "Kursiv";
static const char obliquegerm[] = "Schräg";
static const char condensedgerm[] = "Schmal";
static const char expandedgerm[] = "Breit";
static const char outlinegerm[] = "Konturert";

static const char regularspan[] = "Normal";
static const char boldspan[] = "Negrita";
static const char lightspan[] = "Fina";
static const char blackspan[] = "Supernegra";
static const char italicspan[] = "Cursiva";
static const char condensedspan[] = "Condensada";
static const char expandedspan[] = "Amplida";

static const char regulardutch[] = "Regelmatig";
static const char bolddutch[] = "Vet";
static const char lightdutch[] = "Licht";
static const char blackdutch[] = "Extra vet";
static const char italicdutch[] = "Cursief";
static const char condenseddutch[] = "Smal";
static const char expandeddutch[] = "Breed";

static const char regularswed[] = "Mager";
static const char boldswed[] = "Fet";
static const char lightswed[] = "Extrafin";
static const char blackswed[] = "Extrafet";
static const char italicswed[] = "Kursiv";
static const char condensedswed[] = "Smal";
static const char expandedswed[] = "Bred";

static const char regulardanish[] = "Normal";
static const char bolddanish[] = "Fed";
static const char demibolddanish[] = "Halvfed";
static const char lightdanish[] = "Fin";
static const char mediumdanish[] = "Medium";
static const char blackdanish[] = "Extrafed";
static const char italicdanish[] = "Kursiv";
static const char condenseddanish[] = "Smal";
static const char expandeddanish[] = "Bred";
static const char outlinedanish[] = "Kontour";

static const char regularnor[] = "Vanlig";
static const char boldnor[] = "Halvfet";
static const char lightnor[] = "Mager";
static const char blacknor[] = "Fet";
static const char italicnor[] = "Kursiv";
static const char condensednor[] = "Smal";
static const char expandednor[] = "Sperret";

static const char regularital[] = "Normale";
static const char demiboldital[] = "Nerretto";
static const char boldital[] = "Nero";
static const char thinital[] = "Fine";
static const char lightital[] = "Chiaro";
static const char mediumital[] = "Medio";
static const char bookital[] = "Libro";
static const char heavyital[] = "Nerissimo";
static const char blackital[] = "ExtraNero";
static const char italicital[] = "Cursivo";
static const char obliqueital[] = "Obliquo";
static const char condensedital[] = "Condensato";
static const char expandedital[] = "Allargato";

static const char regularru[] = "Обычный";
static const char demiboldru[] = "Полужирный";
static const char boldru[] = "Обычный";
static const char heavyru[] = "Сверхжирный";
static const char blackru[] = "Чёрный";
static const char thinru[] = "Тонкий";
static const char lightru[] = "Светлый";
static const char italicru[] = "Курсив";
static const char obliqueru[] = "Наклон";
static const char condensedru[] = "Узкий";
static const char expandedru[] = "Широкий";

static const char regularhu[] = "Normál";
static const char demiboldhu[] = "Negyedkövér";
static const char demiboldhu2[] = "Félkövér";
static const char boldhu[] = "Félkövér";
static const char boldhu2[] = "Háromnegyedkövér";
static const char thinhu[] = "Sovány";
static const char lighthu[] = "Világos";
static const char mediumhu[] = "Közepes";
static const char bookhu[] = "Halvány";
static const char bookhu2[] = "Könyv";
static const char heavyhu[] = "Kövér";
static const char heavyhu2[] = "Extrakövér";
static const char blackhu[] = "Fekete";
static const char blackhu2[] = "Sötét";
static const char italichu[] = "Dőlt";
static const char obliquehu[] = "Döntött";
static const char obliquehu2[] = "Ferde";
static const char condensedhu[] = "Keskeny";
static const char expandedhu[] = "Széles";
static const char outlinehu[] = "Kontúros";

static struct langstyle regs[] = { {0x409, regulareng}, { 0x40c, regularfren }, { 0x410, regularital }, { 0x407, regulargerm }, { 0x40a, regularspan }, { 0x419, regularru }, { 0x40e, regularhu },
	{ 0x413, regulardutch}, { 0x41d, regularswed }, { 0x414, regularnor }, { 0x406, regulardanish}, { 0 }};
static struct langstyle meds[] = { {0x409, mediumeng}, { 0x410, mediumital }, { 0x40c, mediumfren }, { 0x407, mediumgerm }, { 0x40e, mediumhu }, { 0x406, mediumdanish}, { 0 }};
static struct langstyle books[] = { {0x409, bookeng}, { 0x410, bookital }, { 0x407, bookgerm }, { 0x40e, bookhu }, { 0x40e, bookhu2 }, { 0 }};
static struct langstyle bolds[] = { {0x409, boldeng}, { 0x410, boldital }, { 0x40c, boldfren }, { 0x407, boldgerm }, { 0x407, boldgerm2 }, { 0x40a, boldspan}, { 0x419, boldru }, { 0x40e, boldhu }, { 0x40e, boldhu2 }, 
	{ 0x413, bolddutch}, { 0x41d, boldswed }, { 0x414, boldnor }, { 0x406, bolddanish}, { 0 }};
static struct langstyle italics[] = { {0x409, italiceng}, { 0x410, italicital }, { 0x40c, italicfren }, { 0x407, italicgerm }, { 0x40a, italicspan}, { 0x419, italicru }, { 0x40e, italichu },
	{ 0x413, italicdutch}, { 0x41d, italicswed }, { 0x414, italicnor },  { 0x406, italicdanish}, { 0 }};
static struct langstyle obliques[] = { {0x409, obliqueeng}, { 0x410, obliqueital }, { 0x40c, obliquefren }, { 0x407, obliquegerm }, { 0x419, obliqueru }, { 0x40e, obliquehu }, { 0x40e, obliquehu2 }, { 0 }};
static struct langstyle demibolds[] = { {0x409, demiboldeng}, {0x409, demiboldeng3}, {0x409, demiboldeng5},
	{ 0x410, demiboldital }, { 0x40c, demiboldfren }, { 0x40c, demiboldfren2 }, { 0x407, demiboldgerm }, { 0x407, demiboldgerm2 },
	{ 0x419, demiboldru }, { 0x40e, demiboldhu }, { 0x40e, demiboldhu2 }, { 0x406, demibolddanish}, { 0 }};
static struct langstyle heavys[] = { {0x409, heavyeng}, { 0x410, heavyital }, { 0x419, heavyru }, { 0x40e, heavyhu }, { 0x40e, heavyhu2 }, { 0 }};
static struct langstyle blacks[] = { {0x409, blackeng}, { 0x410, blackital }, { 0x40c, blackfren }, { 0x407, blackgerm }, { 0x419, blackru }, { 0x40e, blackhu }, { 0x40e, blackhu2 }, { 0x40a, blackspan }, 
	{ 0x413, blackdutch}, { 0x41d, blackswed }, { 0x414, blacknor }, { 0x406, blackdanish}, { 0 }};
static struct langstyle thins[] = { {0x409, thineng}, { 0x410, thinital }, { 0x419, thinru }, { 0x40e, thinhu }, { 0 }};
static struct langstyle extralights[] = { {0x409, extralighteng}, {0x409, extralighteng2}, {0}};
static struct langstyle lights[] = { {0x409, lighteng}, {0x410, lightital}, {0x40c, lightfren}, {0x407, lightgerm}, { 0x419, lightru }, { 0x40e, lighthu }, { 0x40a, lightspan }, 
	{ 0x413, lightdutch}, { 0x41d, lightswed }, { 0x414, lightnor }, { 0x406, lightdanish}, { 0 }};
static struct langstyle condenseds[] = { {0x409, condensedeng}, {0x410, condensedital}, {0x40c, condensedfren}, {0x407, condensedgerm}, { 0x419, condensedru }, { 0x40e, condensedhu }, { 0x40a, condensedspan }, 
	{ 0x413, condenseddutch}, { 0x41d, condensedswed }, { 0x414, condensednor }, { 0x406, condenseddanish}, { 0 }};
static struct langstyle expandeds[] = { {0x409, expandedeng}, {0x410, expandedital}, {0x40c, expandedfren}, {0x407, expandedgerm}, { 0x419, expandedru }, { 0x40e, expandedhu }, { 0x40a, expandedspan }, 
	{ 0x413, expandeddutch}, { 0x41d, expandedswed }, { 0x414, expandednor }, { 0x406, expandeddanish}, { 0 }};
static struct langstyle outlines[] = { {0x409, outlineeng}, {0x40c, outlinefren}, {0x407, outlinegerm}, {0x40e, outlinehu}, { 0x406, outlinedanish}, { 0 }};
static struct langstyle *stylelist[] = {regs, meds, books, demibolds, bolds, heavys, blacks,
	extralights, lights, thins, italics, obliques, condenseds, expandeds, outlines, NULL };

#define CID_Features	101		/* Mac stuff */
#define CID_FeatureDel	103
#define CID_FeatureEdit	105

#define CID_Family	1002
#define CID_Weight	1003
#define CID_ItalicAngle	1004
#define CID_UPos	1005
#define CID_UWidth	1006
#define CID_Ascent	1007
#define CID_Descent	1008
#define CID_Notice	1010
#define CID_Version	1011
#define CID_UniqueID	1012
#define CID_HasVerticalMetrics	1013
#define CID_VOriginLab	1014
#define CID_VOrigin	1015
#define CID_Fontname	1016
#define CID_Em		1017
#define CID_Scale	1018
#define CID_IsOrder2	1019
#define CID_IsMultiLayer	1020
#define CID_Interpretation	1021
#define CID_IsStrokedFont	1022
#define CID_StrokeWidth		1023
#define CID_Namelist	1024
#define CID_XUID	1113
#define CID_Human	1114

#define CID_PrivateEntries	2001
#define	CID_PrivateValues	2002
#define	CID_Add			2003
#define CID_Guess		2004
#define CID_Remove		2005
#define CID_Hist		2006

#define CID_TTFTabs		3000
#define CID_WeightClass		3001
#define CID_WidthClass		3002
#define CID_PFMFamily		3003
#define CID_FSType		3004
#define CID_NoSubsetting	3005
#define CID_OnlyBitmaps		3006
#define CID_LineGap		3007
#define CID_VLineGap		3008
#define CID_VLineGapLab		3009
#define CID_WinAscent		3010
#define CID_WinAscentLab	3011
#define CID_WinAscentIsOff	3012
#define CID_WinDescent		3013
#define CID_WinDescentLab	3014
#define CID_WinDescentIsOff	3015
#define CID_TypoAscent		3016
#define CID_TypoAscentLab	3017
#define CID_TypoAscentIsOff	3018
#define CID_TypoDescent		3019
#define CID_TypoDescentLab	3020
#define CID_TypoDescentIsOff	3021
#define CID_TypoLineGap		3022
#define CID_HHeadAscent		3023
#define CID_HHeadAscentLab	3024
#define CID_HHeadAscentIsOff	3025
#define CID_HHeadDescent	3026
#define CID_HHeadDescentLab	3027
#define CID_HHeadDescentIsOff	3028
#define CID_Vendor		3029
#define CID_IBMFamily		3030

#define CID_SubSuperDefault	3100
#define CID_SubXSize		3101
#define CID_SubYSize		3102
#define CID_SubXOffset		3103
#define CID_SubYOffset		3104
#define CID_SuperXSize		3105
#define CID_SuperYSize		3106
#define CID_SuperXOffset	3107
#define CID_SuperYOffset	3108
#define CID_StrikeoutSize	3109
#define CID_StrikeoutPos	3110

#define CID_PanFamily		4001
#define CID_PanSerifs		4002
#define CID_PanWeight		4003
#define CID_PanProp		4004
#define CID_PanContrast		4005
#define CID_PanStrokeVar	4006
#define CID_PanArmStyle		4007
#define CID_PanLetterform	4008
#define CID_PanMidLine		4009
#define CID_PanXHeight		4010
#define CID_PanDefault		4011
#define CID_PanFamilyLab	4021
#define CID_PanSerifsLab	4022
#define CID_PanWeightLab	4023
#define CID_PanPropLab		4024
#define CID_PanContrastLab	4025
#define CID_PanStrokeVarLab	4026
#define CID_PanArmStyleLab	4027
#define CID_PanLetterformLab	4028
#define CID_PanMidLineLab	4029
#define CID_PanXHeightLab	4030

#define CID_TNLangSort		5001
#define CID_TNStringSort	5002
#define CID_TNVScroll		5003
#define CID_TNHScroll		5004
#define CID_TNEntryField	5005

#define CID_Language		5006	/* Used by AskForLangNames */

#define CID_Comment		6001

#define CID_AnchorClasses	7001
#define CID_AnchorNew		7002
#define CID_AnchorDel		7003
#define CID_AnchorRename	7004
#define CID_ShowMark		7005
#define CID_ShowBase		7006

#define CID_MarkClasses		7101
#define CID_MarkNew		7102
#define CID_MarkEdit		7103

#define CID_TeXText		8001
#define CID_TeXMath		8002
#define CID_TeXMathExt		8003
#define CID_MoreParams		8005
#define CID_TeXExtraSpLabel	8006
#define CID_TeX			8007	/* through 8014 */

#define CID_DesignSize		8301
#define CID_DesignBottom	8302
#define CID_DesignTop		8303
#define CID_StyleID		8304
#define CID_StyleName		8305
#define CID_StyleNameNew	8306
#define CID_StyleNameDel	8307
#define CID_StyleNameRename	8308

#define CID_Contextual		9000
#define CID_ContextClasses	9001	/* Variants at +n*100 */
#define CID_ContextNew		9002
#define CID_ContextDel		9003
#define CID_ContextEdit		9004
#define CID_ContextEditData	9005

#define CID_StateMachine	9010
#define CID_SMList		9011	/* Variants at +n*100 */
#define CID_SMNew		9012
#define CID_SMDel		9013
#define CID_SMEdit		9014

#define CID_Tabs		10001
#define CID_OK			10002
#define CID_Cancel		10003
#define CID_MainGroup		10004

#define CID_MacAutomatic	16000
#define CID_MacStyles		16001
#define CID_MacFOND		16002

const char *TTFNameIds(int id) {
    int i;

    FontInfoInit();
    for ( i=0; ttfnameids[i].text!=NULL; ++i )
	if ( ttfnameids[i].userdata == (void *) (intpt) id )
return( (char *) ttfnameids[i].text );

    if ( id==6 )
return( "Postscript" );

return( _("Unknown") );
}

const char *MSLangString(int language) {
    int i;

    FontInfoInit();
    for ( i=0; mslanguages[i].text!=NULL; ++i )
	if ( mslanguages[i].userdata == (void *) (intpt) language )
return( (char *) mslanguages[i].text );

    language &= 0xff;
    for ( i=0; mslanguages[i].text!=NULL; ++i )
	if ( ((intpt) mslanguages[i].userdata & 0xff) == language )
return( (char *) mslanguages[i].text );

return( _("Unknown") );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

struct psdict *PSDictCopy(struct psdict *dict) {
    struct psdict *ret;
    int i;

    if ( dict==NULL )
return( NULL );

    ret = gcalloc(1,sizeof(struct psdict));
    ret->cnt = dict->cnt; ret->next = dict->next;
    ret->keys = gcalloc(ret->cnt,sizeof(char *));
    ret->values = gcalloc(ret->cnt,sizeof(char *));
    for ( i=0; i<dict->next; ++i ) {
	ret->keys[i] = copy(dict->keys[i]);
	ret->values[i] = copy(dict->values[i]);
    }

return( ret );
}

int PSDictFindEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( -1 );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
return( i );

return( -1 );
}

char *PSDictHasEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( NULL );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
return( dict->values[i] );

return( NULL );
}

int PSDictRemoveEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( false );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
    break;
    if ( i==dict->next )
return( false );
    free( dict->keys[i]);
    free( dict->values[i] );
    --dict->next;
    while ( i<dict->next ) {
	dict->keys[i] = dict->keys[i+1];
	dict->values[i] = dict->values[i+1];
	++i;
    }

return( true );
}

int PSDictChangeEntry(struct psdict *dict, char *key, char *newval) {
    int i;

    if ( dict==NULL )
return( -1 );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
    break;
    if ( i==dict->next ) {
	if ( dict->next>=dict->cnt ) {
	    dict->cnt += 10;
	    dict->keys = grealloc(dict->keys,dict->cnt*sizeof(char *));
	    dict->values = grealloc(dict->values,dict->cnt*sizeof(char *));
	}
	dict->keys[dict->next] = copy(key);
	dict->values[dict->next] = NULL;
	++dict->next;
    }
    free(dict->values[i]);
    dict->values[i] = copy(newval);
return( i );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
/* These are Postscript names, and as such should not be translated */
enum { pt_number, pt_boolean, pt_array, pt_code };
static struct { const char *name; short type, arr_size, present; } KnownPrivates[] = {
    { "BlueValues", pt_array, 14 },
    { "OtherBlues", pt_array, 10 },
    { "BlueFuzz", pt_number },
    { "FamilyBlues", pt_array, 14 },
    { "FamilyOtherBlues", pt_array, 10 },
    { "BlueScale", pt_number },
    { "BlueShift", pt_number },
    { "StdHW", pt_array, 1 },
    { "StdVW", pt_array, 1 },
    { "StemSnapH", pt_array, 12 },
    { "StemSnapV", pt_array, 12 },
    { "ForceBold", pt_boolean },
    { "LanguageGroup", pt_number },
    { "RndStemUp", pt_number },
    { "lenIV", pt_number },
    { "ExpansionFactor", pt_number },
    { "Erode", pt_code },
/* I am deliberately not including Subrs and OtherSubrs */
/* The first could not be entered (because it's a set of binary strings) */
/* And the second has special meaning to us and must be handled with care */
    { NULL }
};

struct ask_data {
    int ret;
    int done;
};

static int Ask_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    struct ask_data *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	d->done = true;
    }
return( true );
}

static int Ask_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    struct ask_data *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	d->done = d->ret = true;
    }
return( true );
}

static int ask_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct ask_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static char *AskKey(SplineFont *sf) {
    int i,j, cnt=0;
    GTextInfo *ti;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    struct ask_data d;
    char *ret;
    int ptwidth;

    if ( sf->private==NULL )
	for ( i=0; KnownPrivates[i].name!=NULL; ++i ) {
	    KnownPrivates[i].present = 0;
	    ++cnt;
	}
    else {
	for ( i=0; KnownPrivates[i].name!=NULL; ++i ) {
	    for ( j=0; j<sf->private->next; ++j )
		if ( strcmp(KnownPrivates[i].name,sf->private->keys[j])==0 )
	    break;
	    if ( !(KnownPrivates[i].present = (j<sf->private->next)) )
		++cnt;
	}
    }
    if ( cnt==0 )
	ti = NULL;
    else {
	ti = gcalloc(cnt+1,sizeof(GTextInfo));
	for ( i=cnt=0; KnownPrivates[i].name!=NULL; ++i )
	    if ( !KnownPrivates[i].present )
		ti[cnt++].text = uc_copy(KnownPrivates[i].name);
    }

    memset(&d,'\0',sizeof(d));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Private Key");
    pos.x = pos.y = 0;
    ptwidth = 2*GIntGetResource(_NUM_Buttonsize)+GGadgetScale(60);
    pos.width =GDrawPointsToPixels(NULL,ptwidth);
    pos.height = GDrawPointsToPixels(NULL,90);
    gw = GDrawCreateTopWindow(NULL,&pos,ask_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _("Key (in Private dictionary)");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 6;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 10; gcd[1].gd.pos.y = 18; gcd[1].gd.pos.width = ptwidth-20;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    gcd[1].creator = GTextFieldCreate;
    if ( ti!=NULL ) {
	gcd[1].gd.u.list = ti;
	gcd[1].creator = GListFieldCreate;
    }

    gcd[2].gd.pos.x = 20-3; gcd[2].gd.pos.y = 90-35-3;
    gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[2].text = (unichar_t *) _("_OK");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = Ask_OK;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = -20; gcd[3].gd.pos.y = 90-35;
    gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _("_Cancel");
    label[3].text_is_1byte = true;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.handle_controlevent = Ask_Cancel;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = 2; gcd[4].gd.pos.y = 2;
    gcd[4].gd.pos.width = pos.width-4; gcd[4].gd.pos.height = pos.height-2;
    gcd[4].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[4].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    ret = NULL;
    if ( d.ret )
	ret = cu_copy(_GGadgetGetTitle(gcd[1].ret));
    GTextInfoListFree(ti);
    GDrawDestroyWindow(gw);
return( ret );
}

static GTextInfo *PI_ListSet(SplineFont *sf) {
    GTextInfo *ti = gcalloc((sf->private==NULL?0:sf->private->next)+1,sizeof( GTextInfo ));
    int i=0;

    if ( sf->private!=NULL ) {
	for ( i=0; i<sf->private->next; ++i ) {
	    ti[i].text = uc_copy(sf->private->keys[i]);
	}
    }
    if ( i!=0 )
	ti[0].selected = true;
return( ti );
}

static GTextInfo **PI_ListArray(struct psdict *private) {
    GTextInfo **ti = gcalloc((private==NULL?0:private->next)+1,sizeof( GTextInfo *));
    int i=0;

    if ( private!=NULL ) {
	for ( i=0; i<private->next; ++i ) {
	    ti[i] = gcalloc(1,sizeof(GTextInfo));
	    ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	    ti[i]->text = uc_copy(private->keys[i]);
	}
    }
    ti[i] = gcalloc(1,sizeof(GTextInfo));
    if ( i!=0 )
	ti[0]->selected = true;
return( ti );
}

static void PIPrivateCheck(struct gfi_data *d) {
    if ( d->private==NULL ) {
	if ( d->sf->private==NULL ) {
	    d->private = gcalloc(1,sizeof(struct psdict));
	    d->private->cnt = 10;
	    d->private->keys = gcalloc(10,sizeof(char *));
	    d->private->values = gcalloc(10,sizeof(char *));
	} else
	    d->private = PSDictCopy(d->sf->private);
    }
}

static int PIFinishFormer(struct gfi_data *d) {
    unichar_t *end;
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif

    if ( d->old_sel < 0 )
return( true );
    if ( d->private==NULL && d->sf->private!=NULL ) {
	const unichar_t *val = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_PrivateValues));
	if ( uc_strcmp(val,d->sf->private->values[d->old_sel])==0 )
return( true );			/* Didn't change */
	PIPrivateCheck(d);
    }
    if ( d->private!=NULL && d->old_sel>=0 && d->old_sel!=d->private->next ) {
	const unichar_t *val = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_PrivateValues));
	const unichar_t *pt = val;
	int i;

	/* does the type appear reasonable? */
	while ( isspace(*pt)) ++pt;
	for ( i=0; KnownPrivates[i].name!=NULL; ++i )
	    if ( strcmp(KnownPrivates[i].name,d->private->keys[d->old_sel])==0 )
	break;
	if ( KnownPrivates[i].name!=NULL ) {
	    if ( KnownPrivates[i].type==pt_array ) {
		if ( *pt!='[' && gwwv_ask(_("Bad type"),(const char **) buts,0,1,_("Expected array.\nProceed anyway?"))==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_boolean ) {
		if ( uc_strcmp(pt,"true")!=0 && uc_strcmp(pt,"false")!=0 &&
			gwwv_ask(_("Bad type"),(const char **) buts,0,1,_("Expected boolean.\nProceed anyway?"))==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_code ) {
		if ( *pt!='{' && gwwv_ask(_("Bad type"),(const char **) buts,0,1,_("Expected code.\nProceed anyway?"))==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_number ) {
		u_strtod(pt,&end);
		while ( isspace(*end)) ++end;
		if ( *end!='\0' && gwwv_ask(_("Bad type"),(const char **) buts,0,1,_("Expected number.\nProceed anyway?"))==1 )
return( false );
	    }
	}

	/* Ok then set it */
	free(d->private->values[d->old_sel]);
	d->private->values[d->old_sel] = cu_copy(val);
	d->old_sel = -1;
    }
return( true );
}

static void ProcessListSel(struct gfi_data *d) {
    GGadget *list = GWidgetGetControl(d->gw,CID_PrivateEntries);
    int sel = GGadgetGetFirstListSelectedItem(list);
    unichar_t *temp;
    static const unichar_t nullstr[] = { 0 };
    SplineFont *sf = d->sf;
    struct psdict *private;

    if ( d->old_sel==sel )
return;

    if ( !PIFinishFormer(d)) {
	/*GGadgetSelectListItem(list,sel,false);*/
	GGadgetSelectListItem(list,d->old_sel,true);
return;
    }
    private = d->private ? d->private : sf->private;
    if ( sel==-1 ) {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Remove),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Hist),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PrivateValues),false);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),nullstr);
    } else {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Remove),true);
	if ( strcmp(private->keys[sel],"BlueValues")==0 ||
		strcmp(private->keys[sel],"OtherBlues")==0 ||
		strcmp(private->keys[sel],"StdHW")==0 ||
		strcmp(private->keys[sel],"StemSnapH")==0 ||
		strcmp(private->keys[sel],"StdVW")==0 ||
		strcmp(private->keys[sel],"StemSnapV")==0 ) {
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),true);
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Hist),true);
	} else {
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),false);
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Hist),false);
	}
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PrivateValues),true);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( private->values[sel]));
	free( temp );
	GTextFieldShow(GWidgetGetControl(d->gw,CID_PrivateValues),0);
    }
    d->old_sel = sel;
}

static int PI_Add(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int i;
    char *newkey;
    GTextInfo **ti;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	if ( !PIFinishFormer(d))
return(true);
	newkey = AskKey(d->sf);
	if ( newkey==NULL )
return( true );
	PIPrivateCheck(d);
	if (( i = PSDictFindEntry(d->private,newkey))==-1 )
	    i = PSDictChangeEntry(d->private,newkey,"");
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	ti = PI_ListArray(d->private);
	if ( i>0 ) {
	    ti[0]->selected = false;
	    ti[i]->selected = true;
	}
	GGadgetSetList(list,ti,false);
	d->old_sel = -1;
	ProcessListSel(d);
	free(newkey);
    }
return( true );
}

static void arraystring(char *buffer,real *array,int cnt) {
    int i, ei;

    for ( ei=cnt; ei>1 && array[ei-1]==0; --ei );
    *buffer++ = '[';
    for ( i=0; i<ei; ++i ) {
	sprintf(buffer, "%d ", (int) array[i]);
	buffer += strlen(buffer);
    }
    if ( buffer[-1] ==' ' ) --buffer;
    *buffer++ = ']'; *buffer='\0';
}

static void SnapSet(struct psdict *private,real stemsnap[12], real snapcnt[12],
	char *name1, char *name2 ) {
    int i, mi;
    char buffer[211];

    mi = -1;
    for ( i=0; stemsnap[i]!=0 && i<12; ++i )
	if ( mi==-1 ) mi = i;
	else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
    if ( mi==-1 )
return;
    sprintf( buffer, "[%d]", (int) stemsnap[mi]);
    PSDictChangeEntry(private,name1,buffer);
    arraystring(buffer,stemsnap,12);
    PSDictChangeEntry(private,name2,buffer);
}

static int PI_Guess(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int sel;
    SplineFont *sf;
    real bluevalues[14], otherblues[10];
    real snapcnt[12];
    real stemsnap[12];
    char buffer[211];
    unichar_t *temp;
    struct psdict *private;
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	sf = d->sf;
	private = d->private ? d->private : sf->private;
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	sel = GGadgetGetFirstListSelectedItem(list);
	if ( strcmp(private->keys[sel],"BlueValues")==0 ||
		strcmp(private->keys[sel],"OtherBlues")==0 ) {
	    if ( gwwv_ask(_("Guess"),(const char **) buts,0,1,_("This will change both BlueValues and OtherBlues.\nDo you want to continue?"))==1 )
return( true );
	    PIPrivateCheck(d);
	    private = d->private;
	    FindBlues(sf,bluevalues,otherblues);
	    arraystring(buffer,bluevalues,14);
	    PSDictChangeEntry(private,"BlueValues",buffer);
	    arraystring(buffer,otherblues,10);
	    PSDictChangeEntry(private,"OtherBlues",buffer);
	} else if ( strcmp(private->keys[sel],"StdHW")==0 ||
		strcmp(private->keys[sel],"StemSnapH")==0 ) {
	    if ( gwwv_ask(_("Guess"),(const char **) buts,0,1,_("This will change both StdHW and StemSnapH.\nDo you want to continue?"))==1 )
return( true );
	    FindHStems(sf,stemsnap,snapcnt);
	    PIPrivateCheck(d);
	    SnapSet(d->private,stemsnap,snapcnt,"StdHW","StemSnapH");
	} else if ( strcmp(private->keys[sel],"StdVW")==0 ||
		strcmp(private->keys[sel],"StemSnapV")==0 ) {
	    if ( gwwv_ask(_("Guess"),(const char **) buts,0,1,_("This will change both StdVW and StemSnapV.\nDo you want to continue?"))==1 )
return( true );
	    FindVStems(sf,stemsnap,snapcnt);
	    PIPrivateCheck(d);
	    SnapSet(d->private,stemsnap,snapcnt,"StdVW","StemSnapV");
	}
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( d->private->values[sel]));
	free( temp );
    }
return( true );
}

static int PI_Hist(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int sel;
    SplineFont *sf;
    struct psdict *private;
    enum hist_type h;
    unichar_t *temp;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	sf = d->sf;
	PIPrivateCheck(d);
	private = d->private ? d->private : sf->private;
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	sel = GGadgetGetFirstListSelectedItem(list);
	if ( strcmp(private->keys[sel],"BlueValues")==0 ||
		strcmp(private->keys[sel],"OtherBlues")==0 )
	    h = hist_blues;
	else if ( strcmp(private->keys[sel],"StdHW")==0 ||
		strcmp(private->keys[sel],"StemSnapH")==0 )
	    h = hist_hstem;
	else if ( strcmp(private->keys[sel],"StdVW")==0 ||
		strcmp(private->keys[sel],"StemSnapV")==0 )
	    h = hist_vstem;
	else
return( true );		/* can't happen */
	SFHistogram(sf,private,NULL,NULL,h);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( d->private->values[sel]));
	free( temp );
    }
return( true );
}

static int PI_Delete(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int sel;
    SplineFont *sf;
    GTextInfo **ti;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	PIPrivateCheck(d);
	sf = d->sf;
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	sel = GGadgetGetFirstListSelectedItem(list);
	PSDictRemoveEntry(d->private, d->private->keys[sel]);
	sf->changed = true;
	ti = PI_ListArray(d->private);
	--sel;
	if ( d->private!=NULL && sel>=d->private->next )
	    sel = d->private->next-1;
	if ( sel>0 ) {
	    ti[0]->selected = false;
	    ti[sel]->selected = true;
	}
	GGadgetSetList(list,ti,false);
	d->old_sel = -2;
	ProcessListSel(d);
    }
return( true );
}

static int PI_ListSel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	ProcessListSel(GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

/* Use URW 4 letter abbreviations */
static char *knownweights[] = { "Demi", "Bold", "Regu", "Medi", "Book", "Thin",
	"Ligh", "Heav", "Blac", "Ultr", "Nord", "Norm", "Gras", "Stan", "Halb",
	"Fett", "Mage", "Mitt", "Buch", NULL };
static char *realweights[] = { "Demi", "Bold", "Regular", "Medium", "Book", "Thin",
	"Light", "Heavy", "Black", "Ultra", "Nord", "Normal", "Gras", "Standard", "Halbfett",
	"Fett", "Mager", "Mittel", "Buchschrift", NULL};
static char *moreweights[] = { "ExtraLight", "VeryLight", NULL };
static char **noticeweights[] = { moreweights, realweights, knownweights, NULL };

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int GFI_NameChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GWindow gw = GGadgetGetWindow(g);
	struct gfi_data *gfi = GDrawGetUserData(gw);
	const unichar_t *uname = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Fontname));
	unichar_t ubuf[50];
	int i,j;
	for ( j=0; noticeweights[j]!=NULL; ++j ) {
	    for ( i=0; noticeweights[j][i]!=NULL; ++i ) {
		if ( uc_strstrmatch(uname,noticeweights[j][i])!=NULL ) {
		    uc_strcpy(ubuf, noticeweights[j]==knownweights ?
			    realweights[i] : noticeweights[j][i]);
		    GGadgetSetTitle(GWidgetGetControl(gw,CID_Weight),ubuf);
	    break;
		}
	    }
	    if ( noticeweights[j][i]!=NULL )
	break;
	}
	if ( gfi->human_untitled )
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_Human),uname);
	if ( gfi->family_untitled ) {
	    const unichar_t *ept = uname+u_strlen(uname); unichar_t *temp;
	    for ( i=0; knownweights[i]!=NULL; ++i ) {
		if (( temp = uc_strstrmatch(uname,knownweights[i]))!=NULL && temp<ept && temp!=uname )
		    ept = temp;
	    }
	    if (( temp = uc_strstrmatch(uname,"ital"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = uc_strstrmatch(uname,"obli"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = uc_strstrmatch(uname,"kurs"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = uc_strstrmatch(uname,"slanted"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = u_strchr(uname,'-'))!=NULL && temp!=uname )
		ept = temp;
	    temp = u_copyn(uname,ept-uname);
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_Family),temp);
	}
    }
return( true );
}

static int GFI_FamilyChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	gfi->family_untitled = false;
    }
return( true );
}

static int GFI_HumanChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	gfi->human_untitled = false;
    }
return( true );
}

static int GFI_VMetricsCheck(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	const unichar_t *vo = _GGadgetGetTitle(GWidgetGetControl(gw,CID_VOrigin));
	int checked = GGadgetIsChecked(g);
	if ( checked && *vo=='\0' ) {
	    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	    char space[10]; unichar_t uspace[10];
	    sprintf( space, "%d", d->sf->ascent );
	    uc_strcpy(uspace,space);
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_VOrigin),uspace);
	}
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_VOrigin),checked);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_VOriginLab),checked);
	GGadgetSetEnabled(GWidgetGetControl(GDrawGetParentWindow(gw),CID_VLineGap),checked);
	GGadgetSetEnabled(GWidgetGetControl(GDrawGetParentWindow(gw),CID_VLineGapLab),checked);
    }
return( true );
}

static int GFI_EmChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	char buf[20]; unichar_t ubuf[20];
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(g); unichar_t *end;
	int val = u_strtol(ret,&end,10), ascent, descent;
	if ( *end )
return( true );
	switch ( GGadgetGetCid(g)) {
	  case CID_Em:
	    ascent = rint( ((double) val)*d->sf->ascent/(d->sf->ascent+d->sf->descent) );
	    descent = val - ascent;
	  break;
	  case CID_Ascent:
	    ascent = val;
	    ret = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
	    descent = u_strtol(ret,&end,10);
	    if ( *end )
return( true );
	  break;
	  case CID_Descent:
	    descent = val;
	    ret = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Ascent));
	    ascent = u_strtol(ret,&end,10);
	    if ( *end )
return( true );
	  break;
	}
	sprintf( buf, "%d", ascent ); if ( ascent==0 ) buf[0]='\0'; uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Ascent),ubuf);
	sprintf( buf, "%d", descent ); if ( descent==0 ) buf[0]='\0'; uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Descent),ubuf);
	sprintf( buf, "%d", ascent+descent ); if ( ascent+descent==0 ) buf[0]='\0'; uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Em),ubuf);
    }
return( true );
}

static int GFI_GuessItalic(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	real val = SFGuessItalicAngle(d->sf);
	char buf[30]; unichar_t ubuf[30];
	sprintf( buf, "%.1f", val);
	uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_ItalicAngle),ubuf);
    }
return( true );
}

static void GFI_CleanupContext(struct gfi_data *d) {
    FPST *fpst;
    ASM *sm;
    int i, j;

    /* Free any context which were created but got [Cancelled] */
    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_ContextClasses+i*100);
	int32 len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    fpst = (FPST *) (old[j]->userdata);
	    if ( fpst!=NULL ) fpst->ticked = false;
	}
    }
    for ( fpst = d->sf->possub; fpst!=NULL; fpst=fpst->next )
	fpst->ticked = true;
    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_ContextClasses+i*100);
	int32 len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    fpst = (FPST *) (old[j]->userdata);
	    if ( fpst!=NULL && !fpst->ticked ) {
		fpst->next = NULL;
		FPSTFree(fpst);
	    }
	}
    }

    /* Do the same thing for apple state machines */
    for ( i=0; i<3; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+i*100);
	int32 len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    sm = (ASM *) (old[j]->userdata);
	    if ( sm!=NULL ) sm->ticked = false;
	}
    }
    for ( sm = d->sf->sm; sm!=NULL; sm=sm->next )
	sm->ticked = true;
    for ( i=0; i<3; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+i*100);
	int32 len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    sm = (ASM *) (old[j]->userdata);
	    if ( sm!=NULL && !sm->ticked ) {
		sm->next = NULL;
		ASMFree(sm);
	    }
	}
    }
}

static void GFI_ProcessContexts(struct gfi_data *d) {
    FPST *fpst, *next, *p, *last;
    ASM *sm, *nextsm, *psm, *lastsm;
    int i, j;

    /* Free any contexts which have been deleted */
    for ( fpst = d->sf->possub; fpst!=NULL; fpst=fpst->next )
	fpst->ticked = false;
    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_ContextClasses+i*100);
	int32 len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    fpst = (FPST *) (old[j]->userdata);
	    if ( fpst!=NULL ) fpst->ticked = true;
	}
    }
    p = NULL;
    for ( fpst = d->sf->possub; fpst!=NULL; fpst=next ) {
	next = fpst->next;
	if ( fpst->ticked )
	    p = fpst;
	else {
	    if ( p==NULL )
		d->sf->possub = next;
	    else
		p->next = next;
	    fpst->next = NULL;
	    FPSTFree(fpst);
	}
    }

    /* Now build up a new list containing all fpst's */
    last = NULL;
    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_ContextClasses+i*100);
	int32 len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    fpst = (FPST *) (old[j]->userdata);
	    if ( fpst!=NULL ) {
		if ( last==NULL )
		    d->sf->possub = fpst;
		else
		    last->next = fpst;
		last = fpst;
	    }
	    old[j]->userdata = NULL;
	}
    }
    if ( last==NULL )
	d->sf->possub = NULL;
    else
	last->next = NULL;

    /* And for apple state machines... */
    /* Free any state machines which have been deleted */
    for ( sm = d->sf->sm; sm!=NULL; sm=sm->next )
	sm->ticked = false;
    for ( i=0; i<3; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+i*100);
	int32 len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    sm = (ASM *) (old[j]->userdata);
	    if ( sm!=NULL ) sm->ticked = true;
	}
    }
    psm = NULL;
    for ( sm = d->sf->sm; sm!=NULL; sm=nextsm ) {
	nextsm = sm->next;
	if ( sm->ticked )
	    psm = sm;
	else {
	    if ( p==NULL )
		d->sf->sm = nextsm;
	    else
		psm->next = nextsm;
	    sm->next = NULL;
	    ASMFree(sm);
	}
    }

    /* Now build up a new list containing all active state machines */
    lastsm = NULL;
    for ( i=0; i<4; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+i*100);
	int32 len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    sm = (ASM *) (old[j]->userdata);
	    if ( sm!=NULL ) {
		if ( lastsm==NULL )
		    d->sf->sm = sm;
		else
		    lastsm->next = sm;
		lastsm = sm;
	    }
	    old[j]->userdata = NULL;
	}
    }
    if ( lastsm==NULL )
	d->sf->sm = NULL;
    else
	lastsm->next = NULL;
}

static void MCD_Close(struct markclassdlg *mcd);

static void SortableNamesFree(struct sortablenames *names,int cnt) {
    int i;

    if ( names==NULL )
return;

    for ( i=0; i<cnt; ++i )
	free(names[i].str);
    free(names);
}

static void GFI_Close(struct gfi_data *d) {
    FontView *fvs;
    SplineFont *sf = d->sf;

    if ( d->ccd )
	CCD_Close(d->ccd);
    if ( d->smd )
	SMD_Close(d->smd);
    if ( d->mcd )
	MCD_Close(d->mcd );

    GFI_CleanupContext(d);
    PSDictFree(d->private);
    SortableNamesFree(d->ttfnames,d->tn_cnt);

    GDrawDestroyWindow(d->gw);
    for ( fvs = d->sf->fv; fvs!=NULL; fvs = fvs->nextsame ) {
	GDrawRequestExpose(sf->fv->v,NULL,false);
	if ( fvs->fontinfo == d )
	    fvs->fontinfo = NULL;
    }
    d->done = true;
    /* d will be freed by destroy event */;
}

static void MarkClassFree(int cnt,char **classes,char **names) {
    int i;

    for ( i=1; i<cnt; ++i ) {
	free( classes[i] );
	free( names[i] );
    }
    free( classes );
    free( names );
}

static void GFI_CancelClose(struct gfi_data *d) {
    MacFeatListFree(GGadgetGetUserData((GWidgetGetControl(
	    d->gw,CID_Features))));
    MarkClassFree(d->mark_class_cnt,d->mark_classes,d->mark_class_names);
    GFI_Close(d);
}

static GTextInfo *MarkClassesList(SplineFont *sf) {
    int cnt;
    GTextInfo *ti;

    if ( sf->mark_class_cnt==0 )
return( NULL );

    ti = gcalloc(sf->mark_class_cnt+1,sizeof(GTextInfo));
    for ( cnt=1; cnt<sf->mark_class_cnt; ++cnt ) {
	ti[cnt-1].text = uc_copy(sf->mark_class_names[cnt]);
	ti[cnt-1].fg = ti[cnt-1].bg = COLOR_DEFAULT;
    }
return( ti );
}

#define CID_MCD_Name		1001
#define CID_MCD_Set		1002
#define CID_MCD_Select		1003
#define CID_MCD_GlyphList	1004

#define MCD_Width	250
#define MCD_Height	210

typedef struct markclassdlg {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int which;
} MarkClassDlg;

static void MCD_Close(MarkClassDlg *mcd) {
    mcd->d->mcd = NULL;
    GDrawDestroyWindow(mcd->gw);
    free(mcd);
}

static void MCD_DoCancel(MarkClassDlg *mcd) {
    MCD_Close(mcd);
}

static int MCD_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MarkClassDlg *mcd = GDrawGetUserData(GGadgetGetWindow(g));
	MCD_DoCancel(mcd);
    }
return( true );
}

static int MCD_InvalidClassList(const char *ret,char **classes, char **names,
	int nclass, int which) {
    const char *pt, *end;
    char *tpt, *tend;
    int i;

    for ( pt = ret; *pt; pt = end ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	end = strchr(pt,' ');
	if ( end==NULL ) end = pt+strlen(pt);
	for ( i=1; classes[i]!=NULL; ++i ) {
	    if ( which==i )
	continue;
	    for ( tpt=classes[i]; *tpt; tpt = tend ) {
		while ( *tpt==' ' ) ++tpt;
		tend = strchr(tpt,' ');
		if ( tend==NULL ) tend = tpt+strlen(tpt);
		if ( tend-tpt==end-pt && strncmp(pt,tpt,end-pt)==0 ) {
		    char *dupname = copyn(pt,end-pt);
		    gwwv_post_error(_("Bad Class"),_("No glyphs from another class may appear here, but %.30s appears here and in class %.30s"), dupname, names[i]);
		    free(dupname);
return( true );
		}
	    }
	}
    }
return( false );
}

static int MCD_OK(GGadget *g, GEvent *e) {
    char *newname;
    char *glyphs;
    struct gfi_data *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MarkClassDlg *mcd = GDrawGetUserData(GGadgetGetWindow(g));
	newname = GGadgetGetTitle8(GWidgetGetControl(mcd->gw,CID_MCD_Name));
	glyphs = GGadgetGetTitle8(GWidgetGetControl(mcd->gw,CID_MCD_GlyphList));
	d = mcd->d;

	if ( !CCD_NameListCheck(d->sf,glyphs,true,_("Bad Class")) ||
		MCD_InvalidClassList(glyphs,d->mark_classes,d->mark_class_names,
			d->mark_class_cnt,mcd->which )) {
	    free(newname); free(glyphs);
return( true );
	}

	if ( mcd->which==-1 ) {		/* New */
	    
	    if ( d->mark_class_cnt==0 ) {
		d->mark_class_cnt = 2;		/* Class 0 is magic */
		d->mark_classes = gcalloc(2,sizeof(char *));
		d->mark_class_names = gcalloc(2,sizeof(unichar_t *));
	    } else {
		++d->mark_class_cnt;
		d->mark_classes = grealloc(d->mark_classes,d->mark_class_cnt*sizeof(char*));
		d->mark_class_names = grealloc(d->mark_class_names,d->mark_class_cnt*sizeof(unichar_t*));
	    }
	    d->mark_classes[d->mark_class_cnt-1] = copy(glyphs);
	    d->mark_class_names[d->mark_class_cnt-1] = copy(newname);
	    GListAppendLine8(mcd->list,newname,false);
	} else {
	    free(d->mark_classes[mcd->which]); d->mark_classes[mcd->which] = copy(glyphs);
	    free(d->mark_class_names[mcd->which]); d->mark_class_names[mcd->which] = copy(newname);
	    GListChangeLine8(mcd->list,mcd->which,newname);
	}
	MCD_Close(mcd);
	free(newname); free(glyphs);
    }
return( true );
}

static int MCD_ToSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MarkClassDlg *mcd = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(mcd->gw,CID_MCD_GlyphList));
	SplineFont *sf = mcd->d->sf;
	FontView *fv = sf->fv;
	const unichar_t *end;
	int pos, found=-1;
	char *nm;

	GDrawSetVisible(fv->gw,true);
	GDrawRaise(fv->gw);
	memset(fv->selected,0,fv->map->enccount);
	while ( *ret ) {
	    end = u_strchr(ret,' ');
	    if ( end==NULL ) end = ret+u_strlen(ret);
	    nm = cu_copybetween(ret,end);
	    for ( ret = end; isspace(*ret); ++ret);
	    if (( pos = SFFindSlot(sf,fv->map,-1,nm))!=-1 ) {
		if ( found==-1 ) found = pos;
		if ( pos!=-1 )
		    fv->selected[pos] = true;
	    }
	    free(nm);
	}

	if ( found!=-1 )
	    FVScrollToChar(fv,found);
	GDrawRequestExpose(fv->v,NULL,false);
    }
return( true );
}

static int MCD_FromSelection(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	MarkClassDlg *mcd = GDrawGetUserData(GGadgetGetWindow(g));
	SplineFont *sf = mcd->d->sf;
	FontView *fv = sf->fv;
	unichar_t *vals, *pt;
	int i, len, max, gid;
	SplineChar *sc, dummy;
    
	for ( i=len=max=0; i<fv->map->enccount; ++i ) if ( fv->selected[i]) {
	    gid = fv->map->map[i];
	    if ( gid!=-1 && sf->glyphs[gid]!=NULL )
		sc = sf->glyphs[gid];
	    else
		sc = SCBuildDummy(&dummy,sf,fv->map,i);
	    len += strlen(sc->name)+1;
	    if ( fv->selected[i]>max ) max = fv->selected[i];
	}
	pt = vals = galloc((len+1)*sizeof(unichar_t));
	*pt = '\0';
	/* in a class the order of selection is irrelevant */
	for ( i=0; i<fv->map->enccount; ++i ) {
	    gid = fv->map->map[i];
	    if ( gid!=-1 && sf->glyphs[gid]!=NULL )
		sc = sf->glyphs[gid];
	    else
		sc = SCBuildDummy(&dummy,sf,fv->map,i);
	    uc_strcpy(pt,sc->name);
	    pt += u_strlen(pt);
	    *pt++ = ' ';
	}
	if ( pt>vals ) pt[-1]='\0';
    
	GGadgetSetTitle(GWidgetGetControl(mcd->gw,CID_MCD_GlyphList),vals);
	free(vals);
    }
return( true );
}

void DropChars2Text(GWindow gw, GGadget *glyphs,GEvent *event) {
    char *cnames;
    const unichar_t *old;
    unichar_t *new;
    int32 len;

    if ( !GDrawSelectionHasType(gw,sn_drag_and_drop,"STRING"))
return;
    cnames = GDrawRequestSelection(gw,sn_drag_and_drop,"STRING",&len);
    if ( cnames==NULL )
return;

    old = _GGadgetGetTitle(glyphs);
    if ( old==NULL || *old=='\0' ) {
	new = uc_copy(cnames);
    } else {
	new = galloc(strlen(cnames)+u_strlen(old)+5);
	u_strcpy(new,old);
	if ( new[u_strlen(new)-1]!=' ' )
	    uc_strcat(new," ");
	uc_strcat(new,cnames);
    }
    GGadgetSetTitle(glyphs,new);
    free( cnames );
    free( new );
}

static void MCD_Drop(MarkClassDlg *mcd,GEvent *event) {
    DropChars2Text(mcd->gw,GWidgetGetControl(mcd->gw,CID_MCD_GlyphList),event);
}

static int mcd_e_h(GWindow gw, GEvent *event) {
    MarkClassDlg *mcd = GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_close:
	MCD_DoCancel(mcd);
      break;
      case et_char:
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("fontinfo.html#MarkClass");
return( true );
	}
return( false );
      break;
      case et_drop:
	MCD_Drop(mcd,event);
      break;
    }
return( true );
}

static void CreateMarkClassDlg(struct gfi_data *d, GGadget *list, int which) {
    MarkClassDlg *mcd;
    GWindow gw;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[10];
    GTextInfo label[10];
    int k;
    unichar_t *freeme = NULL;

    if ( d->mcd!=NULL ) {
	GDrawSetVisible(d->mcd->gw,true);
	GDrawRaise(d->mcd->gw);
return;
    }

    memset(&wattrs,0,sizeof(wattrs));
    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));

    mcd = gcalloc(1,sizeof(MarkClassDlg));
    mcd->d = d; mcd->list = list; mcd->which = which;

    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = false;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Mark Classes");
    wattrs.is_dlg = false;
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,MCD_Width));
    pos.height = GDrawPointsToPixels(NULL,MCD_Height);
    mcd->gw = gw = GDrawCreateTopWindow(NULL,&pos,mcd_e_h,mcd,&wattrs);

    k = 0;

    label[k].text = (unichar_t *) _("Class Name:");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 10;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    if ( which!=-1 ) {
	gcd[k].gd.label = &label[k];
	label[k].text = (unichar_t *) d->mark_class_names[which];
	label[k].text_is_1byte = true;
    }
    gcd[k].gd.pos.x = 70; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_MCD_Name;
    gcd[k++].creator = GTextFieldCreate;

    label[k].text = (unichar_t *) _("Set");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+28;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set this glyph list to be the glyphs selected in the fontview");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = MCD_FromSelection;
    gcd[k].gd.cid = CID_MCD_Set;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("Select");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 70; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.popup_msg = (unichar_t *) _("Set the fontview's selection to be the glyphs named here");
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[k].gd.handle_controlevent = MCD_ToSelection;
    gcd[k].gd.cid = CID_MCD_Select;
    gcd[k++].creator = GButtonCreate;

    if ( which!=-1 ) {
	gcd[k].gd.label = &label[k];
	label[k].text = freeme = uc_copy(d->mark_classes[which]);
    }
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.pos.width = MCD_Width-20; gcd[k].gd.pos.height = 8*13+4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;	/* Just ASCII text for glyph names, no need for xim */
    gcd[k].gd.cid = CID_MCD_GlyphList;
    gcd[k++].creator = GTextAreaCreate;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = MCD_Height-30-3;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_default;
    gcd[k].gd.handle_controlevent = MCD_OK;
    gcd[k++].creator = GButtonCreate;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = -30+3; gcd[k].gd.pos.y = MCD_Height-30;
    gcd[k].gd.pos.width = -1;
    gcd[k].gd.flags = gg_visible|gg_enabled | gg_but_cancel;
    gcd[k].gd.handle_controlevent = MCD_Cancel;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
    gcd[k].gd.pos.width = pos.width-4;
    gcd[k].gd.pos.height = pos.height-4;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
    gcd[k++].creator = GGroupCreate;

    GGadgetsCreate(mcd->gw,gcd);
    GDrawSetVisible(mcd->gw,true);

    free(freeme);
}

static int GFI_MarkNew(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	CreateMarkClassDlg(d, GWidgetGetControl(GGadgetGetWindow(g),CID_MarkClasses), -1);
    }
return( true );
}

static int GFI_MarkEdit(GGadget *g, GEvent *e) {
    int i;
    GGadget *list;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_MarkClasses);
	if ( (i = GGadgetGetFirstListSelectedItem(list))==-1 && i+1 < d->mark_class_cnt )
return( true );
	CreateMarkClassDlg(d, list, i+1);
    }
return( true );
}

static int GFI_MarkSelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MarkEdit),sel!=-1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	e->u.control.subtype = et_buttonactivate;
	GFI_MarkEdit(g,e);
    }
return( true );
}

GTextInfo *AnchorClassesList(SplineFont *sf) {
    AnchorClass *an;
    int cnt;
    GTextInfo *ti;

    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next );
    ti = gcalloc(cnt+1,sizeof(GTextInfo));
    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next ) {
	ti[cnt].text = ClassName(an->name,an->feature_tag,an->flags,
		an->script_lang_index, an->merge_with, an->type,false,sf);
	ti[cnt].fg = ti[cnt].bg = COLOR_DEFAULT;
	ti[cnt].userdata = an;
    }
return( ti );
}

GTextInfo **AnchorClassesLList(SplineFont *sf) {
    AnchorClass *an;
    int cnt;
    GTextInfo **ti;

    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next );
    ti = gcalloc(cnt+1,sizeof(GTextInfo*));
    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next ) {
	ti[cnt] = gcalloc(1,sizeof(GTextInfo));
	ti[cnt]->text = ClassName(an->name,an->feature_tag,an->flags,
		an->script_lang_index,an->merge_with, an->type,false,sf);
	ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
	ti[cnt]->userdata = an;
    }
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

GTextInfo **AnchorClassesSimpleLList(SplineFont *sf) {
    AnchorClass *an;
    int cnt;
    GTextInfo **ti;

    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next );
    ti = gcalloc(cnt+1,sizeof(GTextInfo*));
    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next ) {
	ti[cnt] = gcalloc(1,sizeof(GTextInfo));
	ti[cnt]->text = uc_copy(an->name);
	ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
	ti[cnt]->userdata = an;
    }
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

static void GFI_AnchorShow(GGadget *g, int index) {
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    int i, start;
    GGadget *list = GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorClasses);
    int sel = GGadgetGetFirstListSelectedItem(list);
    int32 len;
    GTextInfo **old = GGadgetGetList(list,&len);
    AnchorClass *ac;
    SplineChar *sc;
    AnchorPoint *ap, *ap2;
    CharView *cvs;

    if ( (ac = old[sel]->userdata)==NULL )
return;

    if ( d->anchor_shows[index].restart || d->anchor_shows[index].sc==NULL )
	start = 0;
    else
	start = d->anchor_shows[index].sc->orig_pos+1;
    for ( i=start; i<d->sf->glyphcnt; ++i ) if ( d->sf->glyphs[i]!=NULL ) {
	sc = d->sf->glyphs[i];
	for ( ap = sc->anchor; ap!=NULL ; ap=ap->next ) {
	    if ( ap->anchor==ac &&
		    ((index==0 && (ap->type==at_mark || ap->type==at_centry)) ||
		     (index==1 && (ap->type==at_basechar || ap->type==at_baselig ||
				     ap->type==at_basemark || ap->type==at_cexit))))
	break;
	}
	if ( ap!=NULL )
    break;
    }
    if ( i==d->sf->glyphcnt ) {
	if ( start==0 ) {
	    GGadgetSetEnabled(g,false);
	    gwwv_post_error(_("No More"),index==0?_("There are no marks associated with this anchor class"):_("There are no base glyphs associated with this anchor class"));
	} else {
	    GGadgetSetTitle8(g,index==0?_("Show First Mark"):_("Show First Base"));
	    gwwv_post_error(_("No More"),index==0?_("There are no more marks associated with this anchor class"):_("There are no more base glyphs associated with this anchor class"));
	}
    } else {
	cvs = NULL;
	if ( d->anchor_shows[index].sc!=NULL ) {
	    for ( cvs = d->anchor_shows[index].sc->views;
		    cvs!=NULL && cvs!=d->anchor_shows[index].cv; cvs=cvs->next );
	}
	for ( ap2 = sc->anchor; ap2!=NULL ; ap2=ap2->next )
	    ap2->selected = false;
	ap->selected = true;
	d->anchor_shows[index].sc = sc;
	d->anchor_shows[index].restart = false;
	if ( cvs!=NULL )
	    CVChangeSC(cvs,sc);
	else
	    d->anchor_shows[index].cv = CharViewCreate(sc,sc->parent->fv,-1);
	if ( start==0 )
	    GGadgetSetTitle8(g,index==0?_("Show Next Mark"):_("Show Next Base"));
    }
}

static int GFI_AnchorShowMark(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	GFI_AnchorShow(g,0);
return( true );
}

static int GFI_AnchorShowBase(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	GFI_AnchorShow(g,1);
return( true );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

int AnchorClassesNextMerge(AnchorClass *ac) {
    int max=0;

    while ( ac!=NULL ) {
	if ( ac->merge_with>max ) max = ac->merge_with;
	ac = ac->next;
    }
return( max + 1 );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void AnchorClassNameDecompose(AnchorClass *ac,const unichar_t *line,
	SplineFont *sf) {
    unichar_t *end;

    free(ac->name);
    ac->feature_tag = (line[0]<<24) | (line[1]<<16) | (line[2]<<8) | line[3];
    line += 5;
    if (( line[0]=='r' || line[0]==' ' ) &&
	    ( line[1]=='b' || line[1]==' ' ) &&
	    ( line[2]=='l' || line[2]==' ' ) &&
	    ( line[3]=='m' || line[3]==' ' ) &&
	    ( line[4]==' ' || line[4]=='(' )) {
	ac->flags = 0;
	if ( line[0]=='r' ) ac->flags |= pst_r2l;
	if ( line[1]=='b' ) ac->flags |= pst_ignorebaseglyphs;
	if ( line[2]=='l' ) ac->flags |= pst_ignoreligatures;
	if ( line[3]=='m' ) ac->flags |= pst_ignorecombiningmarks;
	line += 4;
	if ( *line=='(' ) {
	    const unichar_t *end;
	    int i;
	    for ( end = ++line; *end && *end!=')'; ++end );
	    for ( i=sf->mark_class_cnt-1; i>0; --i )
		if ( strlen(sf->mark_class_names[i])==end-line &&
			uc_strncmp(line,sf->mark_class_names[i],
				end-line)==0 )
	    break;
	    ac->flags |= (i<<8);
	    line = end;
	}
	++line;
    }
    ac->script_lang_index = u_strtol(line,&end,10);
    ac->type = u_strtol(end,&end,10);
    ac->merge_with = u_strtol(end,&end,10);
    while ( *end==' ' ) ++end;
    ac->name = u2utf8_copy(end);
}

static void GFI_GetAnchors(struct gfi_data *d) {
    GGadget *list = GWidgetGetControl(d->gw,CID_AnchorClasses);
    int32 len;
    GTextInfo **old = GGadgetGetList(list,&len);
    AnchorClass *klast=NULL, *test;
    int i;
    SplineFont *sf = d->sf;

    for ( i=0; i<len; ++i ) {
	test = chunkalloc(sizeof(AnchorClass));
	AnchorClassNameDecompose(test,old[i]->text,d->sf);
	if ( sf->anchor==NULL )
	    sf->anchor = test;
	else
	    klast->next = test;
	klast = test;
    }
}

static unichar_t *GFI_AskNameTag(char *title,unichar_t *def,uint32 def_tag, uint16 flags,
	int sli, enum possub_type type, struct gfi_data *d,
	SplineChar *default_script, int merge_with, int act_type ) {
    AnchorClass *oldancs;
    unichar_t *newname;

    oldancs = d->sf->anchor;
    d->sf->anchor = NULL;
    GFI_GetAnchors(d);

    newname = AskNameTag(title,def,def_tag,flags,sli,type,d->sf,
	    default_script,merge_with,act_type);
    AnchorClassesFree(d->sf->anchor);
    d->sf->anchor = oldancs;
return( newname );
}

static char *OtfNameToText(int lang, const char *name) {
    const char *langname;
    char *text;
    int i;

    for ( i=sizeof(mslanguages)/sizeof(mslanguages[0])-1; i>=0 ; --i )
	if ( mslanguages[i].userdata == (void *) (intpt) lang )
    break;
    if ( i==-1 )
	for ( i=sizeof(mslanguages)/sizeof(mslanguages[0])-1; i>=0 ; --i )
	    if ( ((intpt) mslanguages[i].userdata&0xff) == (lang&0xff) )
	break;
    if ( i==-1 )
	langname = "";
    else
	langname = (char*) (mslanguages[i].text);

    text = galloc((strlen(langname)+strlen(name)+4));
    strcpy(text,name);
    strcat(text," | ");
    strcat(text,langname);
return( text );
}

static GTextInfo **StyleNames(struct otfname *otfn) {
    int cnt;
    struct otfname *on;
    GTextInfo **tis;
    char *cname;

    for ( cnt=0, on=otfn; on!=NULL; on=on->next )
	++cnt;
    tis = galloc((cnt+1)*sizeof(GTextInfo *));
    for ( cnt=0, on=otfn; on!=NULL; on=on->next, ++cnt ) {
	tis[cnt] = gcalloc(1,sizeof(GTextInfo));
	tis[cnt]->fg = tis[cnt]->bg = COLOR_DEFAULT;
	tis[cnt]->userdata = (void *) (intpt) otfn->lang;
	cname = OtfNameToText(on->lang,on->name);
	tis[cnt]->text = utf82u_copy(cname);
	free(cname);
    }
    tis[cnt] = gcalloc(1,sizeof(GTextInfo));
return( tis );
}

static struct otfname *OtfNameFromStyleNames(GGadget *list) {
    int32 len; int i;
    GTextInfo **old = GGadgetGetList(list,&len);
    struct otfname *head=NULL, *last, *cur;
    unichar_t *pt, *temp;

    for ( i=0; i<len; ++i ) {
	cur = chunkalloc(sizeof(struct otfname));
	cur->lang = (intpt) old[i]->userdata;
	pt = uc_strstr(old[i]->text," | ");
	temp = u_copyn(old[i]->text,pt-old[i]->text);
	cur->name = u2utf8_copy(temp);
	free(temp);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
return( head );
}

static int sn_e_h(GWindow gw, GEvent *event) {

    if ( event->type==et_close ) {
	int *d = GDrawGetUserData(gw);
	*d = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	int *d = GDrawGetUserData(gw);
	*d = GGadgetGetCid(event->u.control.g)+1;
    }
return( true );
}

static void AskForLangName(GGadget *list,int sel) {
    int32 len; int i;
    GTextInfo **old = GGadgetGetList(list,&len);
    unichar_t *name, *pt;
    char *cname;
    int lang_index;
    GGadgetCreateData gcd[7];
    GTextInfo label[5];
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    int done = 0;
    int k;
    GTextInfo **ti;
    char *temp;

    if ( sel==-1 ) {
	for ( i=0; i<len; ++i )
	    if ( old[i]->userdata == (void *) 0x409 )
	break;
	if ( i==len ) {
	    for ( i=sizeof(mslanguages)/sizeof(mslanguages[0])-1; i>=0 ; --i )
		if ( mslanguages[i].userdata == (void *) 0x409 )
	    break;
	    lang_index = i;
	} else {
	    for ( lang_index=sizeof(mslanguages)/sizeof(mslanguages[0])-1; lang_index>=0 ; --lang_index ) {
		for ( i=0; i<len; ++i )
		    if ( mslanguages[lang_index].userdata == old[i]->userdata )
		break;
		if ( i==len )
	    break;
	    }
	}
	if ( lang_index < 0 )
	    lang_index = 0;
	name = uc_copy("");
    } else {
	for ( lang_index=sizeof(mslanguages)/sizeof(mslanguages[0])-1; lang_index>=0 ; --lang_index )
	    if ( mslanguages[lang_index].userdata == old[sel]->userdata )
	break;
	if ( lang_index < 0 )
	    lang_index = 0;
	pt = uc_strstr(old[sel]->text," | ");
	name = u_copyn(old[sel]->text,pt-old[sel]->text);
    }

    memset(gcd,0,sizeof(gcd));
    memset(label,0,sizeof(label));

    gcd[0].gd.pos.x = 7; gcd[0].gd.pos.y = 7;
    gcd[0].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic;
    gcd[0].gd.cid = CID_Language;
    gcd[0].gd.u.list = mslanguages;
    gcd[0].creator = GListButtonCreate;
    for ( i=0; mslanguages[i].text!=NULL; ++i )
	mslanguages[i].selected = false;
    mslanguages[lang_index].selected = true;

    k = 1;
    label[k].text = (unichar_t *) _("_Name:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLabelCreate;

    label[k].text = name;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 50; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;
    gcd[k].gd.pos.width = 122;
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_StyleName;
    gcd[k++].creator = GTextFieldCreate;

    gcd[k].gd.pos.x = 25-3; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+30;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = true;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -25; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
    gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.cid = false;
    gcd[k++].creator = GButtonCreate;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Style Name:");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(180));
    pos.height = GDrawPointsToPixels(NULL,2*26+45);
    gw = GDrawCreateTopWindow(NULL,&pos,sn_e_h,&done,&wattrs);

    GGadgetsCreate(gw,gcd);
    free(name);
    ti = GGadgetGetList(gcd[0].ret,&len);
    for ( i=0; i<len; ++i )
	if ( ti[i]->userdata == mslanguages[lang_index].userdata ) {
	    GGadgetSelectOneListItem(gcd[0].ret,i);
    break;
	}
    GDrawSetVisible(gw,true);

    while ( !done )
	GDrawProcessOneEvent(NULL);

    if ( done==2 ) {
	lang_index = GGadgetGetFirstListSelectedItem(gcd[0].ret);
	cname = OtfNameToText((intpt) ti[lang_index]->userdata,
		(temp = GGadgetGetTitle8(GWidgetGetControl(gw,CID_StyleName))));
	free(temp);
	if ( sel==-1 )
	    GListAppendLine8(list,cname,false)->userdata =
		    ti[lang_index]->userdata;
	else
	    GListChangeLine8(list,sel,cname)->userdata =
		    ti[lang_index]->userdata;
	free(name);
    }

    GDrawDestroyWindow(gw);
}

static int GFI_StyleNameNew(GGadget *g, GEvent *e) {
    GGadget *list;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_StyleName);
	AskForLangName(list,-1);
    }
return( true );
}

static int GFI_StyleNameDel(GGadget *g, GEvent *e) {
    GGadget *list;
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_StyleName);
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_StyleNameDel),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_StyleNameRename),false);
    }
return( true );
}

static int GFI_StyleNameRename(GGadget *g, GEvent *e) {
    GGadget *list;
    int sel;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_StyleName);
	if ( (sel=GGadgetGetFirstListSelectedItem(list))==-1 )
return( true );
	AskForLangName(list,sel);
    }
return( true );
}

static int GFI_StyleNameSelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_StyleNameDel),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_StyleNameRename),sel!=-1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	e->u.control.subtype = et_buttonactivate;
	GFI_StyleNameRename(g,e);
    }
return( true );
}

static int GFI_AnchorNew(GGadget *g, GEvent *e) {
    int32 len; int i;
    GTextInfo **old, **new;
    GGadget *list;
    unichar_t *newname;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));

	newname = GFI_AskNameTag(_("New anchor class"),NULL,CHR('m','a','r','k'),0,
		-1, pst_anchors,d,NULL,AnchorClassesNextMerge(d->sf->anchor),act_mark);

	if ( newname!=NULL ) {
	    list = GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorClasses);
	    old = GGadgetGetList(list,&len);
	    for ( i=0; i<len; ++i ) {
		if ( u_strcmp(old[i]->text+4,newname+4)==0 )
	    break;
	    }
	    if ( i<len ) {
/* GT: We have a name that appears twice in a list of names and that's bad */
		gwwv_post_error(_("Duplicate Name"),_("Duplicate Name"));
		free(newname);
return( true );
	    }
	    if ( uc_strncmp(newname,"curs",4)==0 ) {
		for ( i=0; i<len; ++i ) {
		    if ( uc_strncmp(old[i]->text,"curs",4)==0 )
		break;
		}
		if ( i<len ) {
		    gwwv_post_error(_("Only One"),_("There may be only one anchor class tagged with 'curs'"));
		    free(newname);
return( true );
		}
	    }
	    new = gcalloc(len+2,sizeof(GTextInfo *));
	    for ( i=0; i<len; ++i ) {
		new[i] = galloc(sizeof(GTextInfo));
		*new[i] = *old[i];
		new[i]->text = u_copy(new[i]->text);
	    }
	    new[i] = gcalloc(1,sizeof(GTextInfo));
	    new[i]->fg = new[i]->bg = COLOR_DEFAULT;
	    new[i]->userdata = NULL;
	    new[i]->text = newname;
	    new[i+1] = gcalloc(1,sizeof(GTextInfo));
	    GGadgetSetList(list,new,false);
	}
    }
return( true );
}

void GListMoveSelected(GGadget *list,int offset) {
    int32 len; int i,j;
    GTextInfo **old, **new;

    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    j = (offset<0 ) ? 0 : len-1;
    for ( i=0; i<len; ++i ) if ( old[i]->selected ) {
	if ( offset==0x80000000 || offset==0x7fffffff )
	    /* Do Nothing */;
	else if ( offset<0 ) {
	    if ( (j= i+offset)<0 ) j=0;
	    while ( new[j] ) ++j;
	} else {
	    if ( (j= i+offset)>=len ) j=len-1;
	    while ( new[j] ) --j;
	}
	new[j] = galloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	if ( offset<0 ) ++j; else --j;
    }
    for ( i=j=0; i<len; ++i ) if ( !old[i]->selected ) {
	while ( new[j] ) ++j;
	new[j] = galloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	++j;
    }
    new[len] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
}

void GListDelSelected(GGadget *list) {
    int32 len; int i,j;
    GTextInfo **old, **new;

    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    for ( i=j=0; i<len; ++i ) if ( !old[i]->selected ) {
	new[j] = galloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	++j;
    }
    new[j] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
}

GTextInfo *GListChangeLine(GGadget *list,int pos, const unichar_t *line) {
    GTextInfo **old, **new;
    int32 i,len;
    
    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = galloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	if ( i!=pos )
	    new[i]->text = u_copy(new[i]->text);
	else
	    new[i]->text = u_copy(line);
    }
    new[i] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,pos);
return( new[pos]);
}

GTextInfo *GListAppendLine(GGadget *list,const unichar_t *line,int select) {
    GTextInfo **old, **new;
    int32 i,len;
    
    old = GGadgetGetList(list,&len);
    new = gcalloc(len+2,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = galloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	new[i]->text = u_copy(new[i]->text);
	if ( select ) new[i]->selected = false;
    }
    new[i] = gcalloc(1,sizeof(GTextInfo));
    new[i]->fg = new[i]->bg = COLOR_DEFAULT;
    new[i]->userdata = NULL;
    new[i]->text = u_copy(line);
    new[i]->selected = select;
    new[i+1] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,i);
return( new[i]);
}

GTextInfo *GListChangeLine8(GGadget *list,int pos, const char *line) {
    GTextInfo **old, **new;
    int32 i,len;
    
    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = galloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	if ( i!=pos )
	    new[i]->text = u_copy(new[i]->text);
	else
	    new[i]->text = utf82u_copy(line);
    }
    new[i] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,pos);
return( new[pos]);
}

GTextInfo *GListAppendLine8(GGadget *list,const char *line,int select) {
    GTextInfo **old, **new;
    int32 i,len;
    
    old = GGadgetGetList(list,&len);
    new = gcalloc(len+2,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = galloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	new[i]->text = u_copy(new[i]->text);
	if ( select ) new[i]->selected = false;
    }
    new[i] = gcalloc(1,sizeof(GTextInfo));
    new[i]->fg = new[i]->bg = COLOR_DEFAULT;
    new[i]->userdata = NULL;
    new[i]->text = utf82u_copy(line);
    new[i]->selected = select;
    new[i+1] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,i);
return( new[i]);
}

static int GFI_AnchorDel(GGadget *g, GEvent *e) {
    GGadget *list;
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorClasses);
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorDel),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorRename),false);
    }
return( true );
}

static int GFI_AnchorRename(GGadget *g, GEvent *e) {
    int32 len; int i;
    GTextInfo **old, **new, *ti;
    GGadget *list;
    unichar_t *newname;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorClasses);
	if ( (ti = GGadgetGetListItemSelected(list))==NULL )
return( true );
	newname = GFI_AskNameTag(_("Edit anchor class"),ti->text,0,0,0,pst_anchors,
		d,NULL,0,0);
	if ( newname!=NULL ) {
	    old = GGadgetGetList(list,&len);
	    if (( uc_strncmp(newname,"curs",4)==0 && uc_strncmp(ti->text,"curs",4)!=0 ) ||
		    ( uc_strncmp(newname,"curs",4)!=0 && uc_strncmp(ti->text,"curs",4)==0 )) {
		gwwv_post_error(_("Can't do this change"),_("You may not change the tag on an anchor class to or from 'curs'"));
		free(newname);
return( false );
	    }
	    for ( i=0; i<len; ++i ) if ( old[i]!=ti ) {
		if ( u_strcmp(old[i]->text,newname)==0 )
	    break;
	    }
	    if ( i==len ) {
		for ( i=0; i<len; ++i ) if ( old[i]!=ti ) {
		    if ( u_strcmp(old[i]->text+4,newname+4)==0 )
		break;
		}
		if ( i<len ) {
		    gwwv_post_error(_("Duplicate Name"),_("The name, %s, is already in use with a different tag."),newname);
		    free(newname);
return( false );
		}
	    } else {
		char *buts[3];
		buts[0] = _("C_ontinue");
#if defined(FONTFORGE_CONFIG_GDRAW)
		buts[1] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
		buts[1] = GTK_STOCK_CANCEL;
#endif
		buts[2] = NULL;
		if ( gwwv_ask(_("Duplicate Name"),(const char **) buts,0,1,_("The name, %s, is already in use.\nIf you elect to continue, these two anchor classes\nwill be merged when you press the OK button."),newname)==1 )
return( false );
	    }
	    new = gcalloc(len+1,sizeof(GTextInfo *));
	    for ( i=0; i<len; ++i ) {
		new[i] = galloc(sizeof(GTextInfo));
		*new[i] = *old[i];
		if ( new[i]->selected && newname!=NULL ) {
		    new[i]->text = newname;
		    newname = NULL;
		} else
		    new[i]->text = u_copy(new[i]->text);
	    }
	    new[i] = gcalloc(1,sizeof(GTextInfo));
	    GGadgetSetList(list,new,false);
	}
    }
return( true );
}

static int GFI_AnchorSelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int32 sel = GGadgetGetFirstListSelectedItem(g), len;
	GTextInfo **old = GGadgetGetList(g,&len);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_AnchorDel),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_AnchorRename),sel!=-1);
	d->anchor_shows[0].restart = true;
	d->anchor_shows[1].restart = true;
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_ShowMark),
		_("Show First Mark"));
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_ShowBase),
		_("Show First Base"));
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ShowMark),sel!=-1 && old[sel]->userdata!=NULL);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ShowBase),sel!=-1 && old[sel]->userdata!=NULL);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	e->u.control.subtype = et_buttonactivate;
	GFI_AnchorRename(g,e);
    }
return( true );
}

static GTextInfo *FPSTList(SplineFont *sf,enum possub_type type) {
    int len;
    FPST *fpst;
    GTextInfo *ti;

    for ( len=0, fpst = sf->possub; fpst!=NULL; fpst=fpst->next )
	if ( fpst->type == type )
	    ++len;
    ti = gcalloc(len+1,sizeof(GTextInfo));
    for ( len=0, fpst = sf->possub; fpst!=NULL; fpst=fpst->next ) if ( fpst->type==type ) {
	ti[len].text = ClassName("",fpst->tag,fpst->flags,
		fpst->script_lang_index, -1, -1,false,sf);
	ti[len].fg = ti[len].bg = COLOR_DEFAULT;
	ti[len++].userdata = fpst;
    }
return( ti );
}

void GFI_CCDEnd(struct gfi_data *d) {
    int i;

    d->ccd = NULL;
    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_ContextClasses+i*100);
	int sel = GGadgetGetFirstListSelectedItem(list);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextDel+i*100),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEdit+i*100),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEditData+i*100),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextNew+i*100),true);
    }
}

void GFI_FinishContextNew(struct gfi_data *d,FPST *fpst, unichar_t *newname,
	int success) {
    int off;
    GGadget *list;

    if ( success ) {
	off = fpst->type == pst_contextpos ? 000 :
		fpst->type == pst_contextsub ? 100 :
		fpst->type == pst_chainpos ? 200 :
		fpst->type == pst_chainsub ? 300 : 400;
	list = GWidgetGetControl(d->gw,CID_ContextClasses+off);
	GListAppendLine(list,newname,false)->userdata = fpst;
    } else {
	chunkfree(fpst,sizeof(FPST));
    }
    free(newname);
}

static int GFI_ContextNew(GGadget *g, GEvent *e) {
    int i;
    unichar_t *newname;
    FPST *fpst;
    static char *titles[] = { N_("New Contextual Position"),
	N_("New Contextual Substitution"),
	N_("New Chaining Position"),
	N_("New Chaining Substitution"),
	N_("New Reverse Chaining Substitution"),
	0 };

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int which = (GGadgetGetCid(g)-CID_ContextNew)/100;

	if ( d->ccd )
return( true );

	newname = GFI_AskNameTag(_(titles[which]),NULL,0,0,
		-1, pst_contextpos+which,d,NULL,-2,-1);

	if ( newname!=NULL ) {
	    fpst = chunkalloc(sizeof(FPST));
	    fpst->type = pst_contextpos + which;
	    fpst->format = fpst->type==pst_reversesub ? pst_reversecoverage : pst_class;
	    DecomposeClassName(newname,NULL,&fpst->tag,NULL,&fpst->flags,
		    &fpst->script_lang_index,NULL,NULL,d->sf);
	    if ( (d->ccd = ContextChainEdit(d->sf,fpst,d,newname))!=NULL ) {
	    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
		    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextDel+i*100),false);
		    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEdit+i*100),false);
		    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEditData+i*100),false);
		    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextNew+i*100),false);
		}
	    }
	}
    }
return( true );
}

static int GFI_ContextDel(GGadget *g, GEvent *e) {
    GGadget *list;
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int off = GGadgetGetCid(g)-CID_ContextDel;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_ContextClasses+off);
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_ContextDel+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_ContextEdit+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_ContextEditData+off),false);
    }
return( true );
}

static int GFI_ContextEdit(GGadget *g, GEvent *e) {
    int32 len; int i;
    GTextInfo **old, **new, *ti;
    GGadget *list;
    unichar_t *newname;
    FPST *fpst;
    static char *titles[] = { N_("Edit Contextual Position"),
	N_("Edit Contextual Substitution"),
	N_("Edit Chaining Position"),
	N_("Edit Chaining Substitution"),
	N_("Edit Reverse Chaining Substitution"),
	0 };

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int which = (GGadgetGetCid(g)-CID_ContextEdit)/100;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_ContextClasses+which*100);
	if ( (ti = GGadgetGetListItemSelected(list))==NULL )
return( true );
	fpst = (FPST *) (ti->userdata);
	newname = GFI_AskNameTag(_(titles[which]),ti->text,0,0,0,
		pst_contextpos+which, d,NULL,-2,-1);
	if ( newname!=NULL ) {
	    DecomposeClassName(newname,NULL,&fpst->tag,NULL,&fpst->flags,
		    &fpst->script_lang_index,NULL,NULL,d->sf);
	    old = GGadgetGetList(list,&len);
	    new = gcalloc(len+1,sizeof(GTextInfo *));
	    for ( i=0; i<len; ++i ) {
		new[i] = galloc(sizeof(GTextInfo));
		*new[i] = *old[i];
		if ( new[i]->selected && newname!=NULL ) {
		    new[i]->text = newname;
		    newname = NULL;
		} else
		    new[i]->text = u_copy(new[i]->text);
	    }
	    new[i] = gcalloc(1,sizeof(GTextInfo));
	    GGadgetSetList(list,new,false);
	}
    }
return( true );
}

static int GFI_ContextEditData(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *list = GWidgetGetControl(d->gw,GGadgetGetCid(g)-CID_ContextEditData+CID_ContextClasses);
	int32 sel = GGadgetGetFirstListSelectedItem(list), len;
	GTextInfo **old = GGadgetGetList(list,&len);
	int i;
	if ( d->ccd )
return( true );
	if ( (d->ccd = ContextChainEdit(d->sf,(FPST *) (old[sel]->userdata),d,NULL))!=NULL ) {
	    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextDel+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEdit+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEditData+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextNew+i*100),false);
	    }
	}
    }
return( true );
}

static int GFI_ContextSelChanged(GGadget *g, GEvent *e) {
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    int sel = GGadgetGetFirstListSelectedItem(g);
    int off = GGadgetGetCid(g)-CID_ContextClasses;

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextDel+off),sel!=-1 && d->ccd==NULL);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEdit+off),sel!=-1 && d->ccd==NULL);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEditData+off),
		d->ccd==NULL && sel!=-1 );
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	e->u.control.subtype = et_buttonactivate;
	g = GWidgetGetControl(GGadgetGetWindow(g),GGadgetGetCid(g)-CID_ContextClasses+CID_ContextEditData);
	GFI_ContextEditData(g,e);
    }
return( true );
}

static char *FeatSetName(SplineFont *sf, int feat, int set) {
    char buf[32];
    char *temp, *full;

    sprintf( buf, "<%d,%d> ", feat, set );
    temp = PickNameFromMacName(FindMacSettingName(sf,feat,set));
    if ( temp==NULL )
	full = copy(buf);
    else {
	full = galloc((strlen(buf)+strlen(temp)+1));
	strcpy(full,buf);
	strcat(full,temp);
	free(temp);
    }
return( full );
}

static GTextInfo *SMList(SplineFont *sf,enum asm_type type) {
    int len;
    ASM *sm;
    GTextInfo *ti;

    for ( len=0, sm = sf->sm; sm!=NULL; sm=sm->next )
	if ( sm->type == type )
	    ++len;
    ti = gcalloc(len+1,sizeof(GTextInfo));
    for ( len=0, sm = sf->sm; sm!=NULL; sm=sm->next ) if ( sm->type==type ) {
	if ( type==asm_kern )
	    ti[len].text = (unichar_t *) copy(_("Kerning"));
	else
	    ti[len].text = (unichar_t *) FeatSetName(sf,sm->feature,sm->setting);
	ti[len].text_is_1byte = true;
	ti[len].fg = ti[len].bg = COLOR_DEFAULT;
	ti[len++].userdata = sm;
    }
return( ti );
}

void GFI_SMDEnd(struct gfi_data *d) {
    int i;

    d->smd = NULL;
    for ( i=0; i<3; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+i*100);
	int sel = GGadgetGetFirstListSelectedItem(list);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMDel+i*100),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMEdit+i*100),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMNew+i*100),true);
    }
}

void GFI_FinishSMNew(struct gfi_data *d,ASM *sm, int success, int isnew) {
    int off;
    GGadget *list;
    char *name;

    if ( success ) {
	off = sm->type == asm_indic ? 000 :
		sm->type == asm_context ? 100 :
		sm->type == asm_insert ? 200 : 300;
	list = GWidgetGetControl(d->gw,CID_SMList+off);
	if ( sm->type!=asm_kern )
	    name = FeatSetName(d->sf,sm->feature,sm->setting);
	else
	    name = copy(_("Kerning"));
	if ( isnew )
	    GListAppendLine8(list,name,false)->userdata = sm;
	else
	    GListChangeLine8(list,GGadgetGetFirstListSelectedItem(list),name);
    } else if ( isnew ) {
	chunkfree(sm,sizeof(ASM));
    }
}

static int GFI_SMNew(GGadget *g, GEvent *e) {
    int i;
    ASM *sm;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int which = (GGadgetGetCid(g)-CID_SMNew)/100;

	if ( d->smd )
return( true );

	sm = chunkalloc(sizeof(ASM));
	sm->type = which==0 ? asm_indic : which==1 ? asm_context : which==2 ? asm_insert : asm_kern;
	if ( (d->smd = StateMachineEdit(d->sf,sm,d))!=NULL ) {
	    for ( i=0; i<3; ++i ) {
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMDel+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMEdit+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMNew+i*100),false);
	    }
	}
    }
return( true );
}

static int GFI_SMDel(GGadget *g, GEvent *e) {
    GGadget *list;
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int off = GGadgetGetCid(g)-CID_SMDel;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_SMList+off);
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_SMDel+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_SMEdit+off),false);
    }
return( true );
}

static int GFI_SMEdit(GGadget *g, GEvent *e) {
    int i;
    GTextInfo *ti;
    GGadget *list;
    ASM *sm;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int which = (GGadgetGetCid(g)-CID_SMEdit)/100;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_SMList+which*100);
	if ( (ti = GGadgetGetListItemSelected(list))==NULL )
return( true );
	if ( d->smd!=NULL )
return( true );
	sm = (ASM *) (ti->userdata);
	if ( (d->smd = StateMachineEdit(d->sf,sm,d))!=NULL ) {
	    for ( i=0; i<3; ++i ) {
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMDel+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMEdit+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMNew+i*100),false);
	    }
	}
    }
return( true );
}

static int GFI_SMSelChanged(GGadget *g, GEvent *e) {
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    int sel = GGadgetGetFirstListSelectedItem(g);
    int off = GGadgetGetCid(g)-CID_SMList;

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMDel+off),sel!=-1 && d->ccd==NULL);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMEdit+off),sel!=-1 && d->ccd==NULL);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	e->u.control.subtype = et_buttonactivate;
	g = GWidgetGetControl(GGadgetGetWindow(g),GGadgetGetCid(g)-CID_SMList+CID_SMEdit);
	GFI_SMEdit(g,e);
    }
return( true );
}

static int GFI_SMConvert(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	ASM *sm = SMConvertDlg(d->sf);
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+100);
	while ( sm!=NULL ) {
	    GListAppendLine8(list,FeatSetName(d->sf,sm->feature,sm->setting),false)->userdata = sm;
	    sm = sm->next;
	}
    }
return( true );
}

static int GFI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GFI_CancelClose(d);
    }
return( true );
}

static int AskLoseUndoes() {
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif
return( gwwv_ask(_("Losing Undoes"),(const char **) buts,0,1,_("Changing the order of the splines in the font will lose all undoes.\nContinue anyway?")) );
}

static void BadFamily() {
    gwwv_post_error(_("Bad Family Name"),_("Bad Family Name, must begin with an alphabetic character."));
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static char *modifierlist[] = { "Ital", "Obli", "Kursive", "Cursive", "Slanted",
	"Expa", "Cond", NULL };
static char *modifierlistfull[] = { "Italic", "Oblique", "Kursive", "Cursive", "Slanted",
    "Expanded", "Condensed", NULL };
static char **mods[] = { knownweights, modifierlist, NULL };
static char **fullmods[] = { realweights, modifierlistfull, NULL };

char *_GetModifiers(char *fontname, char *familyname,char *weight) {
    char *pt, *fpt;
    int i, j;

    /* URW fontnames don't match the familyname */
    /* "NimbusSanL-Regu" vs "Nimbus Sans L" (note "San" vs "Sans") */
    /* so look for a '-' if there is one and use that as the break point... */

    if ( (fpt=strchr(fontname,'-'))!=NULL ) {
	++fpt;
	if ( *fpt=='\0' )
	    fpt = NULL;
    } else if ( familyname!=NULL ) {
	for ( pt = fontname, fpt=familyname; *fpt!='\0' && *pt!='\0'; ) {
	    if ( *fpt == *pt ) {
		++fpt; ++pt;
	    } else if ( *fpt==' ' )
		++fpt;
	    else if ( *pt==' ' )
		++pt;
	    else if ( *fpt=='a' || *fpt=='e' || *fpt=='i' || *fpt=='o' || *fpt=='u' )
		++fpt;	/* allow vowels to be omitted from family when in fontname */
	    else
	break;
	}
	if ( *fpt=='\0' && *pt!='\0' )
	    fpt = pt;
	else
	    fpt = NULL;
    }

    if ( fpt == NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    pt = strstr(fontname,mods[i][j]);
	    if ( pt!=NULL && (fpt==NULL || pt<fpt))
		fpt = pt;
	}
    }
    if ( fpt!=NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    if ( strcmp(fpt,mods[i][j])==0 )
return( fullmods[i][j]);
	}
	if ( strcmp(fpt,"BoldItal")==0 )
return( "BoldItalic" );
	else if ( strcmp(fpt,"BoldObli")==0 )
return( "BoldOblique" );

return( fpt );
    }

return( weight==NULL || *weight=='\0' ? "Regular": weight );
}

char *SFGetModifiers(SplineFont *sf) {
return( _GetModifiers(sf->fontname,sf->familyname,sf->weight));
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static const unichar_t *_uGetModifiers(const unichar_t *fontname, const unichar_t *familyname,
	const unichar_t *weight) {
    const unichar_t *pt, *fpt;
    static unichar_t regular[] = { 'R','e','g','u','l','a','r', 0 };
    static unichar_t space[20];
    int i,j;

    /* URW fontnames don't match the familyname */
    /* "NimbusSanL-Regu" vs "Nimbus Sans L" (note "San" vs "Sans") */
    /* so look for a '-' if there is one and use that as the break point... */

    if ( (fpt=u_strchr(fontname,'-'))!=NULL ) {
	++fpt;
	if ( *fpt=='\0' )
	    fpt = NULL;
    } else if ( familyname!=NULL ) {
	for ( pt = fontname, fpt=familyname; *fpt!='\0' && *pt!='\0'; ) {
	    if ( *fpt == *pt ) {
		++fpt; ++pt;
	    } else if ( *fpt==' ' )
		++fpt;
	    else if ( *pt==' ' )
		++pt;
	    else if ( *fpt=='a' || *fpt=='e' || *fpt=='i' || *fpt=='o' || *fpt=='u' )
		++fpt;	/* allow vowels to be omitted from family when in fontname */
	    else
	break;
	}
	if ( *fpt=='\0' && *pt!='\0' )
	    fpt = pt;
	else
	    fpt = NULL;
    }

    if ( fpt==NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    pt = uc_strstr(fontname,mods[i][j]);
	    if ( pt!=NULL && (fpt==NULL || pt<fpt))
		fpt = pt;
	}
    }

    if ( fpt!=NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    if ( uc_strcmp(fpt,mods[i][j])==0 ) {
		uc_strcpy(space,fullmods[i][j]);
return( space );
	    }
	}
	if ( uc_strcmp(fpt,"BoldItal")==0 ) {
	    uc_strcpy(space,"BoldItalic");
return( space );
	} else if ( uc_strcmp(fpt,"BoldObli")==0 ) {
	    uc_strcpy(space,"BoldOblique");
return( space );
	}
return( fpt );
    }

return( weight==NULL || *weight=='\0' ? regular: weight );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void SFSetFontName(SplineFont *sf, char *family, char *mods,char *full) {
    char *n;
    char *pt, *tpt;

    n = galloc(strlen(family)+strlen(mods)+2);
    strcpy(n,family); strcat(n," "); strcat(n,mods);
    if ( full==NULL || *full == '\0' )
	full = copy(n);
    for ( pt=tpt=n; *pt; ) {
	if ( !isspace(*pt))
	    *tpt++ = *pt++;
	else
	    ++pt;
    }
    *tpt = '\0';
#if 0
    for ( pt=tpt=family; *pt; ) {
	if ( !isspace(*pt))
	    *tpt++ = *pt++;
	else
	    ++pt;
    }
    *tpt = '\0';
#endif

    free(sf->fullname); sf->fullname = copy(full);

    /* In the URW world fontnames aren't just a simple concatenation of */
    /*  family name and modifiers, so neither the family name nor the modifiers */
    /*  changed, then don't change the font name */
    if ( strcmp(family,sf->familyname)==0 && strcmp(n,sf->fontname)==0 )
	/* Don't change the fontname */;
	/* or anything else */
    else {
	free(sf->fontname); sf->fontname = n;
	free(sf->familyname); sf->familyname = copy(family);
	free(sf->weight); sf->weight = NULL;
	if ( strstrmatch(mods,"extralight")!=NULL || strstrmatch(mods,"extra-light")!=NULL )
	    sf->weight = copy("ExtraLight");
	else if ( strstrmatch(mods,"demilight")!=NULL || strstrmatch(mods,"demi-light")!=NULL )
	    sf->weight = copy("DemiLight");
	else if ( strstrmatch(mods,"demibold")!=NULL || strstrmatch(mods,"demi-bold")!=NULL )
	    sf->weight = copy("DemiBold");
	else if ( strstrmatch(mods,"semibold")!=NULL || strstrmatch(mods,"semi-bold")!=NULL )
	    sf->weight = copy("SemiBold");
	else if ( strstrmatch(mods,"demiblack")!=NULL || strstrmatch(mods,"demi-black")!=NULL )
	    sf->weight = copy("DemiBlack");
	else if ( strstrmatch(mods,"extrabold")!=NULL || strstrmatch(mods,"extra-bold")!=NULL )
	    sf->weight = copy("ExtraBold");
	else if ( strstrmatch(mods,"extrablack")!=NULL || strstrmatch(mods,"extra-black")!=NULL )
	    sf->weight = copy("ExtraBlack");
	else if ( strstrmatch(mods,"book")!=NULL )
	    sf->weight = copy("Book");
	else if ( strstrmatch(mods,"regular")!=NULL )
	    sf->weight = copy("Regular");
	else if ( strstrmatch(mods,"roman")!=NULL )
	    sf->weight = copy("Roman");
	else if ( strstrmatch(mods,"normal")!=NULL )
	    sf->weight = copy("Normal");
	else if ( strstrmatch(mods,"demi")!=NULL )
	    sf->weight = copy("Demi");
	else if ( strstrmatch(mods,"medium")!=NULL )
	    sf->weight = copy("Medium");
	else if ( strstrmatch(mods,"bold")!=NULL )
	    sf->weight = copy("Bold");
	else if ( strstrmatch(mods,"heavy")!=NULL )
	    sf->weight = copy("Heavy");
	else if ( strstrmatch(mods,"black")!=NULL )
	    sf->weight = copy("Black");
	else if ( strstrmatch(mods,"Nord")!=NULL )
	    sf->weight = copy("Nord");
/* Sigh. URW uses 4 letter abreviations... */
	else if ( strstrmatch(mods,"Regu")!=NULL )
	    sf->weight = copy("Regular");
	else if ( strstrmatch(mods,"Medi")!=NULL )
	    sf->weight = copy("Medium");
	else if ( strstrmatch(mods,"blac")!=NULL )
	    sf->weight = copy("Black");
	else
	    sf->weight = copy("Medium");
    }

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( sf->fv!=NULL && sf->fv->gw!=NULL ) {
	unichar_t *temp;
	int i;
	GDrawSetWindowTitles(sf->fv->gw,temp = uc_copy(sf->fontname),NULL);
	free(temp);
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]->views!=NULL ) {
	    char buffer[300]; unichar_t ubuf[300]; CharView *cv;
	    sprintf( buffer, "%.90s from %.90s", sf->glyphs[i]->name, sf->fontname );
	    uc_strcpy(ubuf,buffer);
	    for ( cv = sf->glyphs[i]->views; cv!=NULL; cv=cv->next )
		GDrawSetWindowTitles(cv->gw,ubuf,NULL);
	}
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI

static int SetFontName(GWindow gw, SplineFont *sf) {
    const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Family));
    const unichar_t *ufont = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Fontname));
    const unichar_t *uweight = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Weight));
    const unichar_t *uhum = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Human));
    int diff = uc_strcmp(ufont,sf->fontname)!=0;

    free(sf->familyname);
    free(sf->fontname);
    free(sf->weight);
    free(sf->fullname);
    sf->familyname = cu_copy(ufamily);
    sf->fontname = cu_copy(ufont);
    sf->weight = cu_copy(uweight);
    sf->fullname = cu_copy(uhum);
return( diff );
}

static int CheckNames(struct gfi_data *d) {
    const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Family));
    const unichar_t *ufont = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Fontname));
    unichar_t *end; const unichar_t *pt;
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif

    if ( u_strlen(ufont)>63 ) {
	gwwv_post_error(_("Bad Font Name"),_("A Postscript name should be ASCII\nand must not contain (){}[]<>%%/ or space\nand must be shorter than 63 characters"));
return( false );
    }

    if ( *ufamily=='\0' ) {
	gwwv_post_error(_("A Font Family name is required"),_("A Font Family name is required"));
return( false );
    }
    /* A postscript name cannot be a number. There are two ways it can be a */
    /*  number, it can be a real (which we can check for with strtod) or */
    /*  it can be a "radix number" which is <intval>'#'<intval>. I'll only */
    /*  do a cursory test for that */
    u_strtod(ufamily,&end);
    if ( *end=='\0' || (isdigit(ufamily[0]) && u_strchr(ufamily,'#')!=NULL) ) {
	gwwv_post_error(_("Bad Font Family Name"),_("A Postscript name may not be a number"));
return( false );
    }
    if ( u_strlen(ufamily)>31 ) {
	if ( gwwv_ask(_("Bad Font Family Name"),(const char **) buts,0,1,_("Some versions of Windows will refuse to install postscript fonts if the familyname is longer than 31 characters. Do you want to continue anyway?"))==1 )
return( false );
    } else {
	if ( u_strlen(ufont)>31 ) {
	    if ( gwwv_ask(_("Bad Font Name"),(const char **) buts,0,1,_("Some versions of Windows will refuse to install postscript fonts if the fontname is longer than 31 characters. Do you want to continue anyway?"))==1 )
return( false );
	} else if ( u_strlen(ufont)>29 ) {
	    if ( gwwv_ask(_("Bad Font Name"),(const char **) buts,0,1,_("Adobe's fontname spec (5088.FontNames.pdf) says that fontnames must not be longer than 29 characters. Do you want to continue anyway?"))==1 )
return( false );
	}
    }
    while ( *ufamily ) {
	if ( *ufamily<' ' || *ufamily>=0x7f ) {
	    gwwv_post_error(_("Bad Font Family Name"),_("A Postscript name should be ASCII\nand must not contain (){}[]<>%%/ or space"));
return( false );
	}
	++ufamily;
    }

    u_strtod(ufont,&end);
    if ( (*end=='\0' || (isdigit(ufont[0]) && u_strchr(ufont,'#')!=NULL)) &&
	    *ufont!='\0' ) {
	gwwv_post_error(_("Bad Font Name"),_("A Postscript name may not be a number"));
return( false );
    }
    for ( pt=ufont; *pt; ++pt ) {
	if ( *pt<=' ' || *pt>=0x7f ||
		*pt=='(' || *pt=='[' || *pt=='{' || *pt=='<' ||
		*pt==')' || *pt==']' || *pt=='}' || *pt=='>' ||
		*pt=='%' || *pt=='/' ) {
	    gwwv_post_error(_("Bad Font Name"),_("A Postscript name should be ASCII\nand must not contain (){}[]<>%%/ or space"));
return( false );
	}
    }
return( true );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void TTF_PSDupsDefault(SplineFont *sf) {
    struct ttflangname *english;
    char versionbuf[40];

    /* Ok, if we've just loaded a ttf file then we've got a bunch of langnames*/
    /*  we copied some of them (copyright, family, fullname, etc) into equiv */
    /*  postscript entries in the sf. If we then use FontInfo and change the */
    /*  obvious postscript entries we are left with the old ttf entries. If */
    /*  we generate a ttf file and then load it the old values pop up. */
    /* Solution: Anything we can generate by default should be set to NULL */
    for ( english=sf->names; english!=NULL && english->lang!=0x409; english=english->next );
    if ( english==NULL )
return;
    if ( english->names[ttf_family]!=NULL &&
	    strcmp(english->names[ttf_family],sf->familyname)==0 ) {
	free(english->names[ttf_family]);
	english->names[ttf_family]=NULL;
    }
    if ( english->names[ttf_copyright]!=NULL &&
	    strcmp(english->names[ttf_copyright],sf->copyright)==0 ) {
	free(english->names[ttf_copyright]);
	english->names[ttf_copyright]=NULL;
    }
    if ( english->names[ttf_fullname]!=NULL &&
	    strcmp(english->names[ttf_fullname],sf->fullname)==0 ) {
	free(english->names[ttf_fullname]);
	english->names[ttf_fullname]=NULL;
    }
    if ( sf->subfontcnt!=0 || sf->version!=NULL ) {
	if ( sf->subfontcnt!=0 )
	    sprintf( versionbuf, "Version %f", sf->cidversion );
	else
	    sprintf(versionbuf,"Version %.20s ", sf->version);
	if ( english->names[ttf_version]!=NULL &&
		strcmp(english->names[ttf_version],versionbuf)==0 ) {
	    free(english->names[ttf_version]);
	    english->names[ttf_version]=NULL;
	}
    }
    if ( english->names[ttf_subfamily]!=NULL &&
	    strcmp(english->names[ttf_subfamily],SFGetModifiers(sf))==0 ) {
	free(english->names[ttf_subfamily]);
	english->names[ttf_subfamily]=NULL;
    }

    /* User should not be allowed any access to this one, not ever */
    free(english->names[ttf_postscriptname]);
    english->names[ttf_postscriptname]=NULL;
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int ttfspecials[] = { ttf_copyright, ttf_family, ttf_fullname,
	ttf_subfamily, ttf_version, -1 };

static char *tn_recalculatedef(struct gfi_data *d,int cur_id) {
    char versionbuf[40], *v;

    switch ( cur_id ) {
      case ttf_copyright:
return( GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_Notice)));
      case ttf_family:
return( GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_Family)));
      case ttf_fullname:
return( GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_Human)));
      case ttf_subfamily:
return( u2utf8_copy(_uGetModifiers(
		_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Fontname)),
		_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Family)),
		_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Weight)))));
      case ttf_version:
	sprintf(versionbuf,_("Version %.20s"),
		v=GGadgetGetTitle8(GWidgetGetControl(d->gw,CID_Version)));
	free(v);
return( copy(versionbuf));
      default:
return( NULL );
    }
}

static const char *langname(int lang,char *buffer) {
    int i;
    for ( i=0; mslanguages[i].text!=NULL; ++i )
	if ( mslanguages[i].userdata == (void *) (intpt) lang )
return( (char *) mslanguages[i].text );

    sprintf( buffer, "%04X", lang );
return( buffer );
}

static int strid_sorter(const void *pt1, const void *pt2) {
    const struct sortablenames *n1 = pt1, *n2 = pt2;
    char ubuf1[20], ubuf2[20];
    const char *l1, *l2;

    if ( n1->strid!=n2->strid )
return( n1->strid - n2->strid );

    l1 = langname(n1->lang,ubuf1);
    l2 = langname(n2->lang,ubuf2);
return( strcmp(l1,l2));
}

static int lang_sorter(const void *pt1, const void *pt2) {
    const struct sortablenames *n1 = pt1, *n2 = pt2;
    char ubuf1[20], ubuf2[20];
    const char *l1, *l2;

    if ( n1->lang==n2->lang )
return( n1->strid - n2->strid );

    l1 = langname(n1->lang,ubuf1);
    l2 = langname(n2->lang,ubuf2);
return( strcmp(l1,l2));
}

static int specialvals(const struct sortablenames *n) {
    if ( n->lang == n->thislocale )
return( -10000000 );
    else if ( (n->lang&0x3ff) == (n->thislocale&0x3ff) )
return( -10000000 + (n->lang&~0x3ff) );
    if ( n->lang == 0x409 )	/* English */
return( -1000000 );
    else if ( (n->lang&0x3ff) == 9 )
return( -1000000 + (n->lang&~0x3ff) );

return( 1 );
}

static int speciallang_sorter(const void *pt1, const void *pt2) {
    const struct sortablenames *n1 = pt1, *n2 = pt2;
    char ubuf1[20], ubuf2[20];
    const char *l1, *l2;
    int pos1=1, pos2=1;

    /* sort so that entries for the current language are first, then English */
    /*  then alphabetical order */
    if ( n1->lang==n2->lang )
return( n1->strid - n2->strid );

    pos1 = specialvals(n1); pos2 = specialvals(n2);
    if ( pos1<0 || pos2<0 )
return( pos1-pos2 );
    l1 = langname(n1->lang,ubuf1);
    l2 = langname(n2->lang,ubuf2);
return( strcmp(l1,l2));
}

static void TTFNames_Resort(struct gfi_data *d) {
    int(*compar)(const void *, const void *);

    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TNLangSort)) )
	compar = lang_sorter;
    else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TNStringSort)) )
	compar = strid_sorter;
    else
	compar = speciallang_sorter;
    qsort(d->ttfnames,d->tn_cnt,sizeof(struct sortablenames),compar);
}

static void DefaultLanguage(struct gfi_data *d) {
    const char *lang=NULL;
    int i, langlen;
    static char *envs[] = { "LC_ALL", "LC_MESSAGES", "LANG", NULL };
    char langcountry[8], language[4];
    int langcode, langlocalecode;

    for ( i=0; envs[i]!=NULL; ++i ) {
	lang = getenv(envs[i]);
	if ( lang!=NULL ) {
	    langlen = strlen(lang);
	    if (( langlen>5 && lang[5]=='.' && lang[2]=='_' ) ||
		    (langlen==5 && lang[2]=='_' ) ||
		    (langlen==2) ||
		    (langlen==3))	/* Some obscure languages have a 3 letter code */
		/* I understand this language */
    break;
	}
    }
    if ( lang==NULL )
	lang = "en_US";
    strncpy(langcountry,lang,5); langcountry[5] = '\0';
    strncpy(language,lang,3); language[3] = '\0';
    if ( language[2]=='_' ) language[2] = '\0';
    langlen = strlen(language);

    langcode = langlocalecode = -1;
    for ( i=0; ms_2_locals[i].loc_name!=NULL; ++i ) {
	if ( strmatch(langcountry,ms_2_locals[i].loc_name)==0 ) {
	    langlocalecode = ms_2_locals[i].local_id;
	    langcode = langlocalecode&0x3ff;
    break;
	} else if ( strncmp(language,ms_2_locals[i].loc_name,langlen)==0 )
	    langcode = ms_2_locals[i].local_id&0x3ff;
    }
    if ( langcode==-1 )		/* Default to English */
	langcode = 0x9;
    d->langlocalecode = langlocalecode==-1 ? (langcode|0x400) : langlocalecode;
}

static void TN_HScroll(struct gfi_data *d,struct sbevent *sb) {
    int newpos = d->tn_offleft;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*d->tn_width/10;
      break;
      case et_sb_up:
        newpos -= d->tn_width/15;
      break;
      case et_sb_down:
        newpos += d->tn_width/15;
      break;
      case et_sb_downpage:
        newpos += 9*d->tn_width/10;
      break;
      case et_sb_bottom:
        newpos = d->tn_hend;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos + d->tn_width > d->tn_hend )
	newpos = d->tn_hend - d->tn_width;
    if ( newpos<0 )
	newpos = 0;
    if ( newpos!=d->tn_offleft ) {
	int diff = d->tn_offleft-newpos;
	GRect r;
	d->tn_offleft = newpos;
	GScrollBarSetPos(GWidgetGetControl(d->gw,CID_TNHScroll),newpos);
	r.x = 1; r.y = 1; r.width = d->tn_width-1; r.height = d->tn_height-1;
	GDrawScroll(d->tn_v,&r,diff,0);
    }
}

static void TN_VScroll(struct gfi_data *d,struct sbevent *sb) {
    int newpos = d->tn_offtop;
    int page = d->tn_height/d->tn_fh;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*page/10;
      break;
      case et_sb_up:
        newpos--;
      break;
      case et_sb_down:
        newpos++;
      break;
      case et_sb_downpage:
        newpos += 9*page/10;
      break;
      case et_sb_bottom:
        newpos = d->tn_cnt+1;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos + page > d->tn_cnt+1 )
	newpos = d->tn_cnt+1 - page;
    if ( newpos<0 )
	newpos = 0;
    if ( newpos!=d->tn_offtop ) {
	int diff = (newpos-d->tn_offtop)*d->tn_fh;
	GRect r;
	d->tn_offtop = newpos;
	GScrollBarSetPos(GWidgetGetControl(d->gw,CID_TNVScroll),newpos);
	r.x = 1; r.y = 1; r.width = d->tn_width-1; r.height = d->tn_height-1;
	GDrawScroll(d->tn_v,&r,0,diff);
    }
}

static int _TN_HScroll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_scrollbarchange ) {
	struct gfi_data *d = (struct gfi_data *) GDrawGetUserData(GGadgetGetWindow(g));
	TN_HScroll(d,&e->u.control.u.sb);
    }
return( true );
}

static int _TN_VScroll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_scrollbarchange ) {
	struct gfi_data *d = (struct gfi_data *) GDrawGetUserData(GGadgetGetWindow(g));
	TN_VScroll(d,&e->u.control.u.sb);
    }
return( true );
}

static void TTFN_SetSBs(struct gfi_data *d) {
    GGadget *vsb, *hsb;
    int langmax,stridmax,strmax, i,k;
    char buf[20];
    int len;
    const char *l;
    char *str, *freeme, *pt;

    vsb = GWidgetGetControl(d->gw,CID_TNVScroll);
    hsb = GWidgetGetControl(d->gw,CID_TNHScroll);

    GScrollBarSetBounds(vsb,0,d->tn_cnt+1,d->tn_height/d->tn_fh);
    if ( d->tn_offtop + d->tn_height/d->tn_fh > d->tn_cnt+1 )
	d->tn_offtop = d->tn_cnt+1 - d->tn_height/d->tn_fh;
    if ( d->tn_offtop<0 ) d->tn_offtop = 0;
    GScrollBarSetPos(vsb,d->tn_offtop);

    GDrawSetFont(d->tn_v,d->tn_font);
    langmax = stridmax = strmax = 0;
    if ( d->tn_cnt>0 ) {
	for ( i=0; i<d->tn_cnt; ++i ) {
	    for ( k=0; ttfnameids[k].text!=NULL && ttfnameids[k].userdata!=(void *) (intpt) d->ttfnames[i].strid;
		    ++k );
	    if ( ttfnameids[k].text!=NULL ) {
		const char *strid = (char *) ttfnameids[k].text;
		len = GDrawGetText8Width(d->tn_v,strid,-1,NULL);
		if ( len>stridmax ) stridmax = len;
	    }

	    l = langname( d->ttfnames[i].lang,buf );
	    len = GDrawGetText8Width(d->tn_v,l,-1,NULL);
	    if ( len>langmax ) langmax = len;

	    str = d->ttfnames[i].str;
	    freeme = NULL;
	    if ( str==NULL )
		freeme = str = tn_recalculatedef(d,d->ttfnames[i].strid);
	    pt = strchr(str,'\n');
	    len = GDrawGetText8Width(d->tn_v,str,pt==NULL ? -1 : pt-str,NULL);
	    free(freeme);
	    if ( len>strmax ) strmax = len;
	}
    }
    d->tn_langstart = 3;
    d->tn_stridstart = d->tn_langstart+langmax+6;
    d->tn_strstart = d->tn_stridstart+stridmax+6;
    d->tn_hend = d->tn_strstart+strmax+3;

    GScrollBarSetBounds(hsb,0,d->tn_hend,d->tn_width);
    if ( d->tn_offleft + d->tn_width > d->tn_hend )
	d->tn_offleft = d->tn_hend - d->tn_width;
    if ( d->tn_offleft<0 ) d->tn_offleft = 0;
    GScrollBarSetPos(hsb,d->tn_offleft);
}

static void TNExpose(struct gfi_data *d,GWindow pixmap,GEvent *event) {
    int i,k;
    const char *l, *strid;
    char buf[20];
    char *str, *freeme, *pt;

    GDrawDrawLine(pixmap,0,0,0,d->tn_height,0x000000);
    GDrawDrawLine(pixmap,d->tn_stridstart-3-d->tn_offleft,0,d->tn_stridstart-3-d->tn_offleft,d->tn_height,0x000000);
    GDrawDrawLine(pixmap,d->tn_strstart-3-d->tn_offleft,0,d->tn_strstart-3-d->tn_offleft,d->tn_height,0x000000);
    GDrawDrawLine(pixmap,0,0,d->tn_width,0,0x000000);

    GDrawSetFont(pixmap,d->tn_font);
    for ( i=event->u.expose.rect.y/d->tn_fh;
	    i<=(event->u.expose.rect.y+event->u.expose.rect.height+d->tn_fh-1)/d->tn_fh &&
	     i+d->tn_offtop<=d->tn_cnt;
	    ++i ) {
	if ( i+d->tn_offtop==d->tn_cnt ) {
	    strncpy(buf+1,_("New"),sizeof(buf)-2);
	    buf[0] = '<';
	    buf[18] = '\0';
	    k = strlen(buf);
	    buf[k] = '>'; buf[k+1] = '\0';
	    GDrawDrawText8(pixmap,d->tn_langstart-d->tn_offleft,i*d->tn_fh+d->tn_as,
		    buf,-1,NULL,0xff0000);
    break;
	}
	if ( d->tn_stridstart-3 > d->tn_offleft ) {
	    l = langname( d->ttfnames[i+d->tn_offtop].lang,buf );
	    GDrawDrawText8(pixmap,d->tn_langstart-d->tn_offleft,i*d->tn_fh+d->tn_as,
		    l,-1,NULL,0x000000);
	}
	if ( d->tn_strstart-3 > d->tn_offleft ) {
	    for ( k=0; ttfnameids[k].text!=NULL && ttfnameids[k].userdata!=(void *) (intpt) d->ttfnames[i+d->tn_offtop].strid;
		    ++k );
	    if ( ttfnameids[k].text!=NULL ) {
		strid = (char *) ttfnameids[k].text;
		GDrawDrawText8(pixmap,d->tn_stridstart-d->tn_offleft,i*d->tn_fh+d->tn_as,
			strid,-1,NULL,0x000000);
	    }
	}

	str = d->ttfnames[i+d->tn_offtop].str;
	freeme = NULL;
	if ( str==NULL )
	    freeme = str = tn_recalculatedef(d,d->ttfnames[i+d->tn_offtop].strid);
	pt = strchr(str,'\n');
	GDrawDrawText8(pixmap,d->tn_strstart-d->tn_offleft,i*d->tn_fh+d->tn_as,
		str,pt==NULL ? -1 : pt-str,NULL,freeme==NULL ? 0x000000 : 0xff0000);
	free(freeme);
    }
}

static int GFI_Char(struct gfi_data *d,GEvent *event) {
    if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	help("fontinfo.html");
return( true );
    } else if ( event->u.chr.keysym=='s' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) ) {
	MenuSaveAll(NULL,NULL,NULL);
return( true );
    } else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	if ( event->u.chr.state&ksm_shift ) {
	    GFI_CancelClose(d);
	} else
	    MenuExit(NULL,NULL,NULL);
return( true );
    }
return( false );
}

static void TN_DeleteActive(struct gfi_data *d) {
    int i;

    if ( d->ttfnames[d->tn_active].cantremove ) {
	GDrawBeep(NULL);
return;
    }
    free(d->ttfnames[d->tn_active].str);
    --d->tn_cnt;
    for ( i=d->tn_active; i<d->tn_cnt; ++i )
	d->ttfnames[i] = d->ttfnames[i+1];
    TTFN_SetSBs(d);
    GDrawRequestExpose(d->tn_v,NULL,false);
}
    
static void CheckActiveStyleTranslation(struct gfi_data *d) {
    int i,j, eng_pos, other_pos;
    char *english, *new=NULL, *temp, *pt;
    int other_lang = d->ttfnames[d->tn_active].lang;
    int changed = false;

    if ( d->ttfnames[d->tn_active].strid!=ttf_subfamily ||
	    other_lang==0x409 ||
	    (d->ttfnames[d->tn_active].str!=NULL &&
	     *d->ttfnames[d->tn_active].str!='\0' ))
return;

    for ( i=d->tn_cnt-1; i>=0 ; --i )
	if ( d->ttfnames[i].strid==ttf_subfamily &&
		d->ttfnames[i].lang == 0x409 )
    break;
    if ( i<0 || (english = d->ttfnames[i].str)==NULL )
	new = tn_recalculatedef(d,ttf_subfamily);
    else
	new = copy(english);
    for ( i=0; stylelist[i]!=NULL; ++i ) {
	eng_pos = other_pos = -1;
	for ( j=0; stylelist[i][j].str!=NULL; ++j ) {
	    if ( stylelist[i][j].lang == other_lang ) {
		other_pos = j;
	break;
	    }
	}
	if ( other_pos==-1 )
    continue;
	for ( j=0; stylelist[i][j].str!=NULL; ++j ) {
	    if ( stylelist[i][j].lang == 0x409 &&
		    (pt = strstrmatch(new,stylelist[i][j].str))!=NULL ) {
		if ( pt==new && strlen(stylelist[i][j].str)==strlen(new) ) {
		    free(new);
		    free(d->ttfnames[d->tn_active].str);
		    d->ttfnames[d->tn_active].str = copy(stylelist[i][other_pos].str);
return;
		}
		temp = galloc((strlen(new)
			+ strlen(stylelist[i][other_pos].str)
			- strlen(stylelist[i][j].str)
			+1));
		strncpy(temp,new,pt-new);
		strcpy(temp+(pt-new),stylelist[i][other_pos].str);
		strcat(temp+(pt-new),pt+strlen(stylelist[i][j].str));
		free(new);
		new = temp;
		changed = true;
	continue;
	    }
	}
    }
    if ( changed ) {
	free(d->ttfnames[d->tn_active].str);
	d->ttfnames[d->tn_active].str = new;
    } else
	free(new);
}

static void TN_StrSmallEdit(struct gfi_data *d) {
    int len;

    if ( d->tn_active < 0 || d->tn_active >= d->tn_cnt ) {
	d->tn_active = -1;
return;
    }
    d->tn_smallactive = d->tn_active;
    CheckActiveStyleTranslation(d);
    GGadgetSetTitle8(d->tn_smalledit,
	    d->ttfnames[d->tn_smallactive].str!=NULL
		    ? d->ttfnames[d->tn_smallactive].str
		    : "" );
    len = d->tn_width - d->tn_strstart + 1;
    if ( len<60 )
	len = 60;
    GGadgetResize(d->tn_smalledit,len,d->tn_fh);
    GGadgetMove(d->tn_smalledit,d->tn_width-len,d->tn_offtop+d->tn_smallactive*d->tn_fh);
    GGadgetSetVisible(d->tn_smalledit,true);
    GGadgetSetEnabled(d->tn_smalledit,true);
    GWidgetIndicateFocusGadget(d->tn_smalledit);
}

static void TN_FinishSmallEdit(struct gfi_data *d) {
    if ( d->tn_smallactive < 0 || d->tn_smallactive >= d->tn_cnt ) {
	d->tn_smallactive = -1;
return;
    }
    free(d->ttfnames[d->tn_smallactive].str);
    d->ttfnames[d->tn_smallactive].str = GGadgetGetTitle8(d->tn_smalledit);
    GGadgetSetVisible(d->tn_smalledit,false);
    if ( *d->ttfnames[d->tn_smallactive].str=='\0' && !d->ttfnames[d->tn_smallactive].cantremove ) {
	d->tn_active = d->tn_smallactive;
	TN_DeleteActive(d);
    }
    d->tn_active = d->tn_smallactive = -1;
}

static int big_e_h(GWindow gw, GEvent *event) {
    struct gfi_data *d = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	d->tn_done = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type == et_resize ) {
	GRect size, temp, temp2;
	GDrawGetSize(gw,&size);
	GGadgetGetSize(GWidgetGetControl(gw,CID_TNEntryField),&temp);
	GGadgetResize(GWidgetGetControl(gw,CID_TNEntryField),size.width-2*temp.x,
		size.height-GDrawPointsToPixels(gw,46));
	GGadgetResize(GWidgetGetControl(gw,CID_MainGroup),size.width-4,
		size.height-4);
	GGadgetGetSize(GWidgetGetControl(gw,CID_OK),&temp);
	GGadgetMove(GWidgetGetControl(gw,CID_OK),temp.x,
		size.height-GDrawPointsToPixels(gw,34+3));
	GGadgetGetSize(GWidgetGetControl(gw,CID_Cancel),&temp2);
	GGadgetMove(GWidgetGetControl(gw,CID_Cancel),size.width-temp.x-temp2.width,
		size.height-GDrawPointsToPixels(gw,34));
	GDrawRequestExpose(gw,NULL,false);
    } else if ( event->type == et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	d->tn_done = true;
	if ( GGadgetGetCid(event->u.control.g)==CID_OK ) {
	    free(d->ttfnames[d->tn_active].str);
	    d->ttfnames[d->tn_active].str = GGadgetGetTitle8(GWidgetGetControl(gw,CID_TNEntryField));
	    if ( *d->ttfnames[d->tn_active].str=='\0' && !d->ttfnames[d->tn_active].cantremove )
		TN_DeleteActive(d);
	}
    }
return( true );
}

static void TN_StrBigEdit(struct gfi_data *d) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData mgcd[6];
    GTextInfo mlabel[6];
    char buf[100], buf2[20];
    const char *lang;
    int k;

    if ( d->tn_active < 0 || d->tn_active >= d->tn_cnt ) {
	d->tn_active = -1;
return;
    }

    lang = langname(d->ttfnames[d->tn_active].lang,buf2);
    for ( k=0; ttfnameids[k].text!=NULL && ttfnameids[k].userdata!=(void *) (intpt) d->ttfnames[d->tn_active].strid;
	    ++k );
    snprintf(buf,sizeof(buf),_("%1$.30s string for %2$.30s"),
	    lang, (char *) ttfnameids[k].text );

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = buf;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(200));
    pos.height = GDrawPointsToPixels(NULL,300);
    d->tn_done = 0;
    gw = GDrawCreateTopWindow(NULL,&pos,big_e_h,d,&wattrs);

    memset(&mgcd,0,sizeof(mgcd));
    memset(&mlabel,0,sizeof(mlabel));
    mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
    mgcd[0].gd.pos.width = 192;
    mgcd[0].gd.pos.height = 260;
    mgcd[0].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap | gg_text_xim;
    mgcd[0].gd.cid = CID_TNEntryField;
    mgcd[0].creator = GTextAreaCreate;

    mgcd[1].gd.pos.x = 30-3; mgcd[1].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
    mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _("_OK");
    mlabel[1].text_is_1byte = true;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.cid = CID_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.pos.x = -30; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y+3;
    mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _("_Cancel");
    mlabel[2].text_is_1byte = true;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.cid = CID_Cancel;
    mgcd[2].creator = GButtonCreate;

    mgcd[3].gd.pos.x = 2; mgcd[3].gd.pos.y = 2;
    mgcd[3].gd.pos.width = pos.width-4; mgcd[3].gd.pos.height = pos.height-4;
    mgcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    mgcd[3].gd.cid = CID_MainGroup;
    mgcd[3].creator = GGroupCreate;

    GGadgetsCreate(gw,mgcd);
    CheckActiveStyleTranslation(d);
    if ( d->ttfnames[d->tn_active].str!=NULL )
	GGadgetSetTitle8(mgcd[0].ret,d->ttfnames[d->tn_active].str);
    GDrawSetVisible(gw,true);
    while ( !d->tn_done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    GDrawRequestExpose(d->tn_v,NULL,false);
}

#define MID_ToggleBase	1
#define MID_MultiEdit	2
#define MID_Delete	3

static void TN_StrPopupDispatch(GWindow gw, GMenuItem *mi, GEvent *e) {
    struct gfi_data *d = GDrawGetUserData(gw);

    switch ( mi->mid ) {
      case MID_ToggleBase:
	d->ttfnames[d->tn_active].basedon = !d->ttfnames[d->tn_active].basedon;
	if ( d->ttfnames[d->tn_active].basedon ) {
	    free( d->ttfnames[d->tn_active].str);
	    d->ttfnames[d->tn_active].str = NULL;
	} else {
	    d->ttfnames[d->tn_active].str = tn_recalculatedef(d,d->ttfnames[d->tn_active].strid);
	}
      break;
      case MID_MultiEdit:
	TN_StrBigEdit(d);
      break;
      case MID_Delete:
	TN_DeleteActive(d);
      break;
    }
    TTFN_SetSBs(d);
    GDrawRequestExpose(d->tn_v,NULL,false);
}

static void TN_StrPopup(struct gfi_data *d,GEvent *event) {
    GMenuItem mi[5];
    int i;

    memset(mi,'\0',sizeof(mi));
    for ( i=0; i<3; ++i ) {
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = i+1;
	mi[i].invoke = TN_StrPopupDispatch;
	mi[i].ti.text_is_1byte = true;
    }
    mi[MID_Delete-1].ti.disabled = d->ttfnames[d->tn_active].cantremove;
    mi[MID_ToggleBase-1].ti.disabled = !d->ttfnames[d->tn_active].cantremove;
    if ( d->ttfnames[d->tn_active].basedon ) {
	mi[MID_MultiEdit-1].ti.disabled = true;
	mi[MID_ToggleBase-1].ti.text = (unichar_t *) _("Detach from PostScript Names");
    } else
	mi[MID_ToggleBase-1].ti.text = (unichar_t *) _("Same as PostScript Names");
    mi[MID_MultiEdit-1].ti.text = (unichar_t *) _("Multi-line edit");
    mi[MID_Delete-1].ti.text = (unichar_t *) _("Delete");
    GMenuCreatePopupMenu(d->tn_v,event, mi);
}

static void TN_StrIDPopupDispatch(GWindow gw, GMenuItem *mi, GEvent *e) {
    struct gfi_data *d = GDrawGetUserData(gw);
    int lang = d->ttfnames[d->tn_active].lang;
    int strid = mi->mid;
    int i;

    for ( i=0; i<d->tn_cnt; ++i ) if ( i!=d->tn_active ) {
	if ( d->ttfnames[i].lang == lang && d->ttfnames[i].strid == strid ) {
	    gwwv_post_error(_("Duplicate Name"),_("This combination of StrID and Language is already in use"));
return;
	}
    }
    d->ttfnames[d->tn_active].strid=strid;
    TTFN_SetSBs(d);
    GDrawRequestExpose(d->tn_v,NULL,false);
}

static void TN_StrIDPopup(struct gfi_data *d,GEvent *event) {
    GMenuItem mi[ttf_namemax+2];
    int i;

    memset(mi,'\0',sizeof(mi));
    for ( i=0; ttfnameids[i].text!=NULL; ++i ) {
	mi[i].mid = (intpt) ttfnameids[i].userdata;
	mi[i].invoke = TN_StrIDPopupDispatch;
	mi[i].ti = ttfnameids[i];
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].ti.checkable = true;
	mi[i].ti.checked = d->ttfnames[d->tn_active].strid==mi[i].mid;
    }
    GMenuCreatePopupMenu(d->tn_v,event, mi);
}

static void TN_LangPopupDispatch(GWindow gw, GMenuItem *mi, GEvent *e) {
    struct gfi_data *d = GDrawGetUserData(gw);
    int lang = mi->mid;
    int strid;
    int i;

    if ( d->tn_active==d->tn_cnt ) {
	strid = 0;
	for ( i=0; i<d->tn_cnt; ++i ) {
	    if ( d->ttfnames[i].lang == lang && d->ttfnames[i].strid == strid ) {
		++strid;
		if ( strid==ttf_postscriptname ) ++strid;
		if ( strid==ttf_idontknow ) ++strid;
		if ( strid==ttf_namemax ) {
		    gwwv_post_error(_("Duplicate Name"),_("All strings for this language have been assigned, you mayn't create another"));
return;
		}
	    }
	}
	if ( d->tn_cnt>=d->tn_max )
	    d->ttfnames = grealloc(d->ttfnames,(d->tn_max+=10)*sizeof(struct sortablenames));
	memset(&d->ttfnames[d->tn_cnt],0,sizeof(struct sortablenames));
	d->ttfnames[d->tn_cnt].lang = lang;
	d->ttfnames[d->tn_cnt].strid = strid;
	d->ttfnames[d->tn_cnt].thislocale = d->langlocalecode;
	d->ttfnames[d->tn_cnt].str = copy("");
	++d->tn_cnt;
    } else {
	strid = d->ttfnames[d->tn_active].strid;
	for ( i=0; i<d->tn_cnt; ++i ) if ( i!=d->tn_active ) {
	    if ( d->ttfnames[i].lang == lang && d->ttfnames[i].strid == strid ) {
		gwwv_post_error(_("Duplicate Name"),_("This combination of StrID and Language is already in use"));
return;
	    }
	}
	d->ttfnames[d->tn_active].lang = lang;
    }
    TTFN_SetSBs(d);
    GDrawRequestExpose(d->tn_v,NULL,false);
}

static int menusort(const void *m1, const void *m2) {
    const GMenuItem *mi1 = m1, *mi2 = m2;

    /* Should do a strcoll here, but I never wrote one */
return( u_strcmp(mi1->ti.text,mi2->ti.text));
}

static void TN_LangPopup(struct gfi_data *d,GEvent *event) {
    GMenuItem mi[250];
    int i;

    if ( sizeof(mi)/sizeof(mi[0]) < sizeof(mslanguages)/sizeof(mslanguages[0])) {
	IError( "Too many locales" );
return;
    }

    memset(mi,'\0',sizeof(mi));
    for ( i=0; mslanguages[i].text!=NULL; ++i ) {
	mi[i].mid = (intpt) mslanguages[i].userdata;
	mi[i].invoke = TN_LangPopupDispatch;
	mi[i].ti = mslanguages[i];
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].ti.checkable = true;
	mi[i].ti.checked = d->tn_active<d->tn_cnt && d->ttfnames[d->tn_active].lang==mi[i].mid;
    }
    qsort(mi,i,sizeof(mi[0]),menusort);
    GMenuCreatePopupMenu(d->tn_v,event, mi);
}

static void TN_Popups(struct gfi_data *d,GEvent *event) {
    int line = event->u.mouse.y/d->tn_fh + d->tn_offtop;
    int x = event->u.mouse.x + d->tn_offleft;

    if ( d->tn_smallactive!=-1 ) {
	TN_FinishSmallEdit(d);
return;
    }
    if ( line<0 )
return;
    d->tn_active = line;
    if ( event->u.mouse.button==3 && line<d->tn_cnt )
	TN_StrPopup(d,event);
    else if ( line<d->tn_cnt && d->ttfnames[line].basedon ) {
	GDrawBeep(NULL);
    } else if ( line>=d->tn_cnt || x<d->tn_stridstart-3 )
	TN_LangPopup(d,event);
    else if ( x<d->tn_strstart-3 )
	TN_StrIDPopup(d,event);
    else if ( d->ttfnames[line].str!=NULL &&
	    (strlen(d->ttfnames[line].str)>40 ||
	     strchr(d->ttfnames[line].str,'\n')!=NULL )) {
	TN_StrBigEdit(d);
    } else
	TN_StrSmallEdit(d);
}

static int tn_e_h(GWindow gw, GEvent *event) {
    struct gfi_data *d = (struct gfi_data *) GDrawGetUserData(gw);

    GGadgetPopupExternalEvent(event);
    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) ) {
	if ( !(event->u.mouse.state&(ksm_shift|ksm_control)) )	/* bind shift to magnify/minify */
return( GGadgetDispatchEvent(GWidgetGetControl(d->gw,CID_TNVScroll),event));
    }

    switch ( event->type ) {
      case et_expose:
	GDrawSetLineWidth(gw,0);
	TNExpose(d,gw,event);
      break;
      case et_mousedown:
	TN_Popups(d,event);
      break;
      case et_mousemove:
      break;
      case et_mouseup:
      break;
      case et_char:
return( GFI_Char(d,event));
      break;
      case et_timer:
      break;
    }
return( true );
}

static void InitTTFNames(struct gfi_data *d) {
    SplineFont *sf = d->sf;
    int i,j,k,cnt;
    uint8 sawEnglishUS[ttf_namemax];
    struct ttflangname *tln;
    struct sortablenames *stn;

    DefaultLanguage(d);

    stn = NULL;
    for ( k=0; k<2; ++k ) {
	memset(sawEnglishUS,0,sizeof(sawEnglishUS));
	cnt = 0;
	for ( tln = sf->names; tln!=NULL; tln = tln->next ) {
	    for ( i=0; i<ttf_namemax; ++i ) if ( i!=ttf_postscriptname && tln->names[i]!=NULL ) {
		if ( stn!=NULL ) {
		    stn[cnt].strid = i;
		    stn[cnt].lang = tln->lang;
		    stn[cnt].basedon = false;
		    stn[cnt].str = copy(tln->names[i]);
		}
		++cnt;
		if ( tln->lang==0x409 )
		    sawEnglishUS[i] = true;
	    }
	}
	for ( i=0; ttfspecials[i]!=-1; ++i ) if ( !sawEnglishUS[ttfspecials[i]] ) {
	    if ( stn!=NULL ) {
		stn[cnt].strid = ttfspecials[i];
		stn[cnt].lang = 0x409;
		stn[cnt].basedon = true;
		stn[cnt].str = NULL;
	    }
	    ++cnt;
	}
	if ( stn==NULL )
	    stn = galloc((cnt+10)*sizeof(struct sortablenames));
    }
    for ( i=0; i<cnt; ++i )
	stn[i].thislocale = d->langlocalecode;
    for ( i=0; i<cnt; ++i ) if ( stn[i].lang==0x409 ) {
	for ( j=0; ttfspecials[j]!=-1 && ttfspecials[j]!=stn[i].strid; ++j );
	if ( ttfspecials[j]!=-1 )
	    stn[i].cantremove = true;
    }
    d->ttfnames = stn;
    d->tn_cnt = cnt; d->tn_max = cnt+10;
    TTFNames_Resort(d);
    TTFN_SetSBs(d);
    d->names_set = true;
}

static int GFI_SortBy(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = (struct gfi_data *) GDrawGetUserData(GGadgetGetWindow(g));
	TTFNames_Resort(d);
	GDrawRequestExpose(d->tn_v,NULL,false);
    }
return( true );
}

static void GFI_ProcessAnchor(struct gfi_data *d) {
    GGadget *list = GWidgetGetControl(d->gw,CID_AnchorClasses);
    int32 len;
    GTextInfo **old = GGadgetGetList(list,&len);
    AnchorClass *keep=NULL, *klast, *test, *prev, *next, *test2;
    int i;

    for ( i=0; i<len; ++i ) {
	if ( old[i]->userdata==NULL ) {
	    test = chunkalloc(sizeof(AnchorClass));
	} else {
	    prev = NULL;
	    for ( test = d->sf->anchor; test!=old[i]->userdata; prev=test, test=test->next );
	    if ( prev==NULL )
		d->sf->anchor = test->next;
	    else
		prev->next = test->next;
	    test->next = NULL;
	}
	AnchorClassNameDecompose(test,old[i]->text,d->sf);
	if ( keep==NULL )
	    keep = test;
	else
	    klast->next = test;
	klast = test;
    }
    for ( test = d->sf->anchor; test!=NULL; test = next ) {
	next = test->next;
	SFRemoveAnchorClass(d->sf,test);
    }
    d->sf->anchor = keep;

    for ( test=d->sf->anchor; test!=NULL; test=test->next ) {
	prev = test;
	for ( test2=test->next; test2!=NULL; test2 = next ) {
	    next = test2->next;
	    if ( strcmp(test->name,test2->name)==0 ) {
		AnchorClassMerge(d->sf,test,test2);
		prev->next = next;
		chunkfree(test2,sizeof(*test2));
	    } else
		prev = test2;
	}
    }
}

static void BDFsSetAsDs(SplineFont *sf) {
    BDFFont *bdf;
    real scale;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	scale = bdf->pixelsize / (real) (sf->ascent+sf->descent);
	bdf->ascent = rint(sf->ascent*scale);
	bdf->descent = bdf->pixelsize-bdf->ascent;
    }
}

static char *texparams[] = { N_("Slant:"), N_("Space:"), N_("Stretch:"),
	N_("Shrink:"), N_("XHeight:"), N_("Quad:"),
/* GT: Extra Space, see below for a full comment */
	N_("Extra Sp:"), NULL };
static char *texpopups[] = { N_("In an italic font the horizontal change per unit vertical change"),
    N_("The amount of space between words when using this font"),
    N_("The amount of strechable space between words when using this font"),
    N_("The amount the space between words may shrink when using this font"),
    N_("The height of the lower case letters with flat tops"),
    N_("The width of one em"),
    N_("Either:\nThe amount of extra space to be added after a sentence\nOr the space to be used within math formulae"),
    NULL};

static int ParseTeX(struct gfi_data *d) {
    int i, err=false;
    double em = (d->sf->ascent+d->sf->descent), val;

    for ( i=0; texparams[i]!=0 ; ++i ) {
	val = GetReal8(d->gw,CID_TeX+i,texparams[i],&err);
	d->texdata.params[i] = rint( val/em * (1<<20) );
    }
    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXText)) )
	d->texdata.type = tex_text;
    else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMath)) )
	d->texdata.type = tex_math;
    else
	d->texdata.type = tex_mathext;
return( !err );
}

static int ttfmultuniqueids(SplineFont *sf,struct gfi_data *d) {
    struct ttflangname *tln;
    int found = false;
    int i;

    if ( d->names_set ) {
	for ( i=0; i<d->tn_cnt; ++i )
	    if ( d->ttfnames[i].strid==ttf_uniqueid ) {
		if ( found )
return( true );
		found = true;
	    }
    } else {
	for ( tln = sf->names; tln!=NULL; tln=tln->next )
	    if ( tln->names[ttf_uniqueid]!=NULL ) {
		if ( found )
return( true );
		found = true;
	    }
    }
return( false );
}

static int ttfuniqueidmatch(SplineFont *sf,struct gfi_data *d) {
    struct ttflangname *tln;
    int i;

    if ( sf->names==NULL )
return( false );

    if ( !d->names_set ) {
	for ( tln = sf->names; tln!=NULL; tln=tln->next )
	    if ( tln->names[ttf_uniqueid]!=NULL )
return( true );
    } else {
	for ( tln = sf->names; tln!=NULL; tln=tln->next ) {
	    if ( tln->names[ttf_uniqueid]==NULL )
	continue;		/* Not set, so if it has been given a new value */
				/*  that's a change, and if it hasn't that's ok */
	    for ( i=0; i<d->tn_cnt; ++i )
		if ( d->ttfnames[i].strid==ttf_uniqueid && d->ttfnames[i].lang==tln->lang )
	    break;
	    if ( i==d->tn_cnt )
	continue;		/* removed. That's a change */
	    if ( strcmp(tln->names[ttf_uniqueid],d->ttfnames[i].str)==0 )
return( true );		/* name unchanged */
	}
    }
return( false );
}

static void ttfuniqueidfixup(SplineFont *sf,struct gfi_data *d) {
    struct ttflangname *tln;
    char *changed = NULL;
    int i;

    if ( sf->names==NULL )
return;

    if ( !d->names_set ) {
	for ( tln = sf->names; tln!=NULL; tln=tln->next ) {
	    free( tln->names[ttf_uniqueid]);
	    tln->names[ttf_uniqueid] = NULL;
	}
    } else {
	/* see if any instances of the name have changed */
	for ( tln = sf->names; tln!=NULL; tln=tln->next ) {
	    if ( tln->names[ttf_uniqueid]==NULL )
	continue;
	    for ( i=0; i<d->tn_cnt; ++i )
		if ( d->ttfnames[i].strid==ttf_uniqueid && d->ttfnames[i].lang==tln->lang )
	    break;
	    if ( i==d->tn_cnt )
	continue;
	    if ( strcmp(tln->names[ttf_uniqueid],d->ttfnames[i].str)!=0 ) {
		changed = copy(d->ttfnames[i].str);
	break;
	    }
	}
	/* All unique ids should be the same, if any changed set the unchanged */
	/*  ones to the one that did (or the first of many if several changed) */
	for ( tln = sf->names; tln!=NULL; tln=tln->next ) {
	    if ( tln->names[ttf_uniqueid]==NULL )
	continue;
	    for ( i=0; i<d->tn_cnt; ++i )
		if ( d->ttfnames[i].strid==ttf_uniqueid && d->ttfnames[i].lang==tln->lang )
	    break;
	    if ( i==d->tn_cnt )
	continue;
	    if ( strcmp(tln->names[ttf_uniqueid],d->ttfnames[i].str)==0 ) {
		free(d->ttfnames[i].str);
		d->ttfnames[i].str = changed!=NULL
			? copy( changed )
			: NULL;
	    }
	}
    }
}

static void SortableToTTFNames(struct gfi_data *d) {
    struct ttflangname *tln;
    SplineFont *sf = d->sf;
    int i;

    TTFLangNamesFree(sf->names); sf->names = NULL;

    for ( i=0; i<d->tn_cnt; ++i ) {
	for ( tln=sf->names; tln!=NULL && tln->lang!=d->ttfnames[i].lang; tln=tln->next );
	if ( tln==NULL ) {
	    tln = chunkalloc(sizeof(struct ttflangname));
	    tln->lang = d->ttfnames[i].lang;
	    tln->next = sf->names;
	    sf->names = tln;
	}
	tln->names[d->ttfnames[i].strid] = d->ttfnames[i].str;
    }
    TTF_PSDupsDefault(sf);
    free(d->ttfnames); d->ttfnames = NULL;
}

/* If we change the ascent/descent of a sub font then consider changing the */
/*  as/ds of the master font. I used to think this irrelevant, but as the */
/*  typoAscent/Descent is based on the master's ascent/descent it actually */
/*  is meaningful. Set the master to the subfont with the most glyphs */
void CIDMasterAsDes(SplineFont *sf) {
    SplineFont *cidmaster = sf->cidmaster;
    SplineFont *best;
    int i, cid, cnt, bcnt;

    if ( cidmaster==NULL )
return;
    best = NULL; bcnt = 0;
    for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	sf = cidmaster->subfonts[i];
	for ( cid=cnt=0; cid<sf->glyphcnt; ++cid )
	    if ( sf->glyphs[cid]!=NULL )
		++cnt;
	if ( cnt>bcnt ) {
	    best = sf;
	    bcnt = cnt;
	}
    }
    if ( best==NULL && cidmaster->subfontcnt>0 )
	best = cidmaster->subfonts[0];
    if ( best!=NULL ) {
	double ratio = 1000.0/(best->ascent+best->descent);
	int ascent = rint(best->ascent*ratio);
	if ( cidmaster->ascent!=ascent || cidmaster->descent!=1000-ascent ) {
	    cidmaster->ascent = ascent;
	    cidmaster->descent = 1000-ascent;
	}
    }
}

static int GFI_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct gfi_data *d = GDrawGetUserData(gw);
	SplineFont *sf = d->sf, *_sf;
	int interp;
	int reformat_fv=0, retitle_fv=false;
	int upos, uwid, as, des, err = false, weight=0;
	int uniqueid, linegap=0, vlinegap=0, winascent=0, windescent=0;
	int tlinegap=0, tascent=0, tdescent=0, hascent=0, hdescent=0;
	int winaoff=true, windoff=true;
	int taoff=true, tdoff=true, haoff = true, hdoff = true;
	real ia, cidversion;
	const unichar_t *txt, *fond; unichar_t *end;
	int i,j, mcs;
	int vmetrics, vorigin, namechange, order2;
	int xuidchanged = false;
	GTextInfo *pfmfam, *ibmfam, *fstype, *nlitem;
	int32 len;
	GTextInfo **ti;
	int subs[4], super[4], strike[2];
	int design_size, size_top, size_bottom, styleid;
	int strokedfont = false;
	real strokewidth;
#ifdef FONTFORGE_CONFIG_TYPE3
	int multilayer = false;
#endif
	char os2_vendor[4];
	NameList *nl;
	extern int allow_utf8_glyphnames;

	if ( d->tn_smallactive!=-1 )
	    TN_FinishSmallEdit(d);
	if ( !CheckNames(d))
return( true );
	if ( !PIFinishFormer(d))
return( true );
	if ( d->ccd )
	    CCD_Close(d->ccd);
	if ( d->smd )
	    SMD_Close(d->smd);

	if ( ttfmultuniqueids(sf,d)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    char *buts[3];
	    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
	    static char *buts[] = { GTK_STOCK_OK, GTK_STOCK_CANCEL, NULL };
#endif
	    if ( gwwv_ask(_("Too many Unique Font IDs"),(const char **) buts,0,1,_("You should only specify the TrueType Unique Font Identification string in one language. This font has more. Do you want to continue anyway?"))==1 )
return( true );
	}
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Family));
	if ( !isalpha(*txt)) {
	    BadFamily();
return( true );
	}
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_ItalicAngle));
	ia = u_strtod(txt,&end);
	if ( *end!='\0' ) {
	    Protest8(_("_Italic Angle:"));
return(true);
	}
	order2 = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsOrder2));
	strokedfont = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsStrokedFont));
	strokewidth = GetReal8(gw,CID_StrokeWidth,_("Stroke _Width:"),&err);
#ifdef FONTFORGE_CONFIG_TYPE3
	multilayer = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsMultiLayer));
#endif
	vmetrics = GGadgetIsChecked(GWidgetGetControl(gw,CID_HasVerticalMetrics));
	upos = GetReal8(gw,CID_UPos, _("Underline _Position:"),&err);
	uwid = GetReal8(gw,CID_UWidth,_("_Height:"),&err);
	GetInt8(gw,CID_Em,_("_Em Size:"),&err);	/* just check for errors. redundant info */
	as = GetInt8(gw,CID_Ascent,_("_Ascent:"),&err);
	des = GetInt8(gw,CID_Descent,_("_Descent:"),&err);
	uniqueid = GetInt8(gw,CID_UniqueID,_("UniqueID"),&err);
	design_size = rint(10*GetReal8(gw,CID_DesignSize,_("De_sign Size:"),&err));
	size_bottom = rint(10*GetReal8(gw,CID_DesignBottom,_("_Bottom"),&err));
	size_top = rint(10*GetReal8(gw,CID_DesignTop,_("_Top"),&err));
	styleid = GetInt8(gw,CID_StyleID,_("Style _ID:"),&err);
	if ( err )
return(true);
	if ( sf->subfontcnt!=0 ) {
	    cidversion = GetReal8(gw,CID_Version,_("_Version"),&err);
	    if ( err )
return(true);
	}
	if ( vmetrics )
	    vorigin = GetInt8(gw,CID_VOrigin,_("Vertical _Origin:"),&err);
	if ( d->ttf_set ) {
	    /* Only use the normal routine if we get no value, because */
	    /*  "400 Book" is a reasonable setting, but would cause GetInt */
	    /*  to complain */
	    weight = u_strtol(_GGadgetGetTitle(GWidgetGetControl(gw,CID_WeightClass)),NULL,10);
	    if ( weight == 0 )
		weight = GetInt8(gw,CID_WeightClass,_("_Weight Class"),&err);
	    linegap = GetInt8(gw,CID_LineGap,_("HHead _Line Gap:"),&err);
	    tlinegap = GetInt8(gw,CID_TypoLineGap,_("Typo Line _Gap:"),&err);
	    if ( vmetrics )
		vlinegap = GetInt8(gw,CID_VLineGap,_("VHead _Column Spacing:"),&err);
	    winaoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_WinAscentIsOff));
	    windoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_WinDescentIsOff));
	    winascent  = GetInt8(gw,CID_WinAscent,winaoff ? _("Win _Ascent Offset:") : _("Win Ascent:"),&err);
	    windescent = GetInt8(gw,CID_WinDescent,windoff ? _("Win _Descent Offset:") : _("Win Descent:"),&err);
	    taoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_TypoAscentIsOff));
	    tdoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_TypoDescentIsOff));
	    tascent  = GetInt8(gw,CID_TypoAscent,taoff ? _("_Typo Ascent Offset:") : _("Typo Ascent:"),&err);
	    tdescent = GetInt8(gw,CID_TypoDescent,tdoff ? _("T_ypo Descent Offset:") : _("Typo Descent:"),&err);
	    haoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_HHeadAscentIsOff));
	    hdoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_HHeadDescentIsOff));
	    hascent  = GetInt8(gw,CID_HHeadAscent,haoff ? _("_HHead Ascent Offset:") : _("HHead Ascent:"),&err);
	    hdescent = GetInt8(gw,CID_HHeadDescent,hdoff ? _("HHead De_scent Offset:") : _("HHead Descent:"),&err);
	    if ( err )
return(true);

	    if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_SubSuperDefault)) ) {
		for ( i=0; i<4; ++i )
		    subs[i] = GetInt8(gw,CID_SubXSize+i,_("Subscript"),&err);
		for ( i=0; i<4; ++i )
		    super[i] = GetInt8(gw,CID_SuperXSize+i,_("Superscript"),&err);
		for ( i=0; i<2; ++i )
		    strike[i] = GetInt8(gw,CID_StrikeoutSize+i,_("Strikeout"),&err);
	    }
	    txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Vendor));
	    if ( u_strlen(txt)>4 || txt[0]>0x7e || (txt[0]!='\0' && (txt[1]>0x7e ||
		    (txt[1]!='\0' && (txt[2]>0x7e || (txt[2]!='\0' && txt[3]>0x7e))))) ) {
		if ( u_strlen(txt)>4 )
		    gwwv_post_error(_("Bad IBM Family"),_("Tag must be 4 characters long"));
		else
		    gwwv_post_error(_("Bad IBM Family"),_("A tag must be 4 ASCII characters"));
return( true );
	    }
	    os2_vendor[0] = txt[0]==0 ? ' ' : txt[0];
	    os2_vendor[1] = txt[0]==0 || txt[1]=='\0' ? ' ' : txt[1];
	    os2_vendor[2] = txt[0]==0 || txt[1]=='\0' || txt[2]=='\0' ? ' ' : txt[2];
	    os2_vendor[3] = txt[0]==0 || txt[1]=='\0' || txt[2]=='\0' || txt[3]=='\0' ? ' ' : txt[3];
	}
	if ( err )
return(true);
	if ( d->tex_set ) {
	    if ( !ParseTeX(d))
return( true );
	}
	if ( as+des>16384 || des<0 || as<0 ) {
	    gwwv_post_error(_("Bad Ascent/Descent"),_("Ascent and Descent must be positive and their sum less than 16384"));
return( true );
	}
	mcs = -1;
	if ( !GGadgetIsChecked(GWidgetGetControl(d->gw,CID_MacAutomatic)) ) {
	    mcs = 0;
	    ti = GGadgetGetList(GWidgetGetControl(d->gw,CID_MacStyles),&len);
	    for ( i=0; i<len; ++i )
		if ( ti[i]->selected )
		    mcs |= (int) (intpt) ti[i]->userdata;
	    if ( (mcs&sf_condense) && (mcs&sf_extend)) {
		gwwv_post_error(_("Bad Style"),_("A style may not have both condense and extend set (it makes no sense)"));
return( true );
	    }
	}
	if ( order2!=sf->order2 && sf->changed && AskLoseUndoes())
return( true );
	if ( order2!=sf->order2 && !SFCloseAllInstrs(sf))
return( true );

	nlitem = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_Namelist));
	if ( nlitem==NULL )
	    nl = DefaultNameListForNewFonts();
	else {
	    char *name = u2utf8_copy(nlitem->text);
	    nl = NameListByName(name);
	    free(name);
	}
	if ( nl->uses_unicode && !allow_utf8_glyphnames ) {
	    gwwv_post_error(_("Namelist contains non-ASCII names"),_("Glyph names should be limited to characters in the ASCII character set,\nbut there are names in this namelist which use characters outside\nthat range."));
return(true);
	}
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( strokedfont!=sf->strokedfont || multilayer!=sf->multilayer ) {
	    if ( sf->strokedfont && multilayer )
		SFSetLayerWidthsStroked(sf,sf->strokewidth);
	    else if ( sf->multilayer )
		SFSplinesFromLayers(sf,strokedfont);
	    SFReinstanciateRefs(sf);
	    if ( multilayer!=sf->multilayer ) {
		sf->multilayer = multilayer;
		SFLayerChange(sf);
	    }
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
		sf->glyphs[i]->changedsincelasthinted = !strokedfont && !multilayer;
	}
#else
	if ( strokedfont!=sf->strokedfont )
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
		sf->glyphs[i]->changedsincelasthinted = !strokedfont;
#endif
	sf->strokedfont = strokedfont;
	sf->strokewidth = strokewidth;
	GDrawSetCursor(gw,ct_watch);
	namechange = SetFontName(gw,sf);
	if ( namechange ) retitle_fv = true;
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_XUID));
	xuidchanged = (sf->xuid==NULL && *txt!='\0') ||
			(sf->xuid!=NULL && uc_strcmp(txt,sf->xuid)==0);
	if ( namechange &&
		((uniqueid!=0 && uniqueid==sf->uniqueid) ||
		 (sf->xuid!=NULL && uc_strcmp(txt,sf->xuid)==0) ||
		 ttfuniqueidmatch(sf,d)) ) {
	    char *buts[4];
	    int ans;
	    buts[0] = _("Change");
	    buts[1] = _("Retain");
#if defined(FONTFORGE_CONFIG_GDRAW)
	    buts[2] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
	    buts[2] = GTK_STOCK_CANCEL;
#endif
	    buts[3] = NULL;
	    ans = gwwv_ask(_("Change UniqueID?"),(const char **) buts,0,2,_("You have changed this font's name without changing the UniqueID (or XUID).\nThis is probably not a good idea, would you like me to\ngenerate a random new value?"));
	    if ( ans==2 ) {
		GDrawSetCursor(gw,ct_pointer);
return(true);
	    }
	    if ( ans==0 ) {
		if ( uniqueid!=0 && uniqueid==sf->uniqueid )
		    uniqueid = 4000000 + (rand()&0x3ffff);
		if ( sf->xuid!=NULL && uc_strcmp(txt,sf->xuid)==0 ) {
		    SFRandomChangeXUID(sf);
		    xuidchanged = true;
		}
	    }
	    if ( ttfuniqueidmatch(sf,d))
		ttfuniqueidfixup(sf,d);
	} else {
	    free(sf->xuid);
	    sf->xuid = *txt=='\0'?NULL:cu_copy(txt);
	}

	OtfNameListFree(sf->fontstyle_name);
	sf->fontstyle_name = OtfNameFromStyleNames(GWidgetGetControl(gw,CID_StyleName));
	sf->design_size = design_size;
	sf->design_range_bottom = size_bottom;
	sf->design_range_top = size_top;
	sf->fontstyle_id = styleid;

	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Notice));
	free(sf->copyright); sf->copyright = cu_copy(txt);
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Comment));
	free(sf->comments); sf->comments = cu_copy(*txt?txt:NULL);
	if ( sf->subfontcnt!=0 ) {
	    sf->cidversion = cidversion;
	} else {
	    txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Version));
	    free(sf->version); sf->version = cu_copy(txt);
	}
	if ( as!=sf->ascent || des!=sf->descent ) {
	    if ( as+des != sf->ascent+sf->descent && GGadgetIsChecked(GWidgetGetControl(gw,CID_Scale)) )
		SFScaleToEm(sf,as,des);
	    else {
		sf->ascent = as;
		sf->descent = des;
	    }
	    BDFsSetAsDs(sf);
	    reformat_fv = true;
	    CIDMasterAsDes(sf);
	}
	fond = _GGadgetGetTitle(GWidgetGetControl(gw,CID_MacFOND));
	free(sf->fondname); sf->fondname = NULL;
	if ( *fond )
	    sf->fondname = cu_copy(fond);
	sf->macstyle = mcs;
	sf->italicangle = ia;
	sf->upos = upos;
	sf->uwidth = uwid;
	sf->uniqueid = uniqueid;
	sf->texdata = d->texdata;

	GFI_ProcessAnchor(d);
	GFI_ProcessContexts(d);

	interp = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_Interpretation));
	if ( interp==-1 ) sf->uni_interp = ui_none;
	else sf->uni_interp = (intpt) interpretations[interp].userdata;

	sf->for_new_glyphs = nl;

	if ( sf->hasvmetrics!=vmetrics )
	    CVPaletteDeactivate();		/* Force a refresh later */
	_sf = sf->cidmaster?sf->cidmaster:sf;
	_sf->hasvmetrics = vmetrics;
	for ( j=0; j<_sf->subfontcnt; ++j )
	    _sf->subfonts[j]->hasvmetrics = vmetrics;
	if ( vmetrics ) {
	    _sf->vertical_origin = vorigin;
	    for ( j=0; j<_sf->subfontcnt; ++j )
		_sf->subfonts[j]->vertical_origin = vorigin;
	}

	if ( d->private!=NULL ) {
	    PSDictFree(sf->private);
	    sf->private = d->private;
	    d->private = NULL;
	}
	if ( d->names_set )
	    SortableToTTFNames(d);
	if ( d->ttf_set ) {
	    sf->pfminfo.weight = weight;
	    sf->pfminfo.width = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_WidthClass))+1;
	    pfmfam = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PFMFamily));
	    if ( pfmfam!=NULL )
		sf->pfminfo.pfmfamily = (intpt) (pfmfam->userdata);
	    else
		sf->pfminfo.pfmfamily = 0x11;
	    ibmfam = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_IBMFamily));
	    if ( pfmfam!=NULL )
		sf->pfminfo.os2_family_class = (intpt) (ibmfam->userdata);
	    else
		sf->pfminfo.os2_family_class = 0x00;
	    memcpy(sf->pfminfo.os2_vendor,os2_vendor,sizeof(os2_vendor));
	    fstype = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_FSType));
	    if ( fstype!=NULL )
		sf->pfminfo.fstype = (intpt) (fstype->userdata);
	    else
		sf->pfminfo.fstype = 0xc;
	    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_NoSubsetting)))
		sf->pfminfo.fstype |=0x100;
	    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_OnlyBitmaps)))
		sf->pfminfo.fstype |=0x200;
	    for ( i=0; i<10; ++i )
		sf->pfminfo.panose[i] = (intpt) (GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PanFamily+i))->userdata);
	    sf->pfminfo.panose_set = !GGadgetIsChecked(GWidgetGetControl(gw,CID_PanDefault));
	    sf->pfminfo.os2_typolinegap = tlinegap;
	    sf->pfminfo.linegap = linegap;
	    if ( vmetrics )
		sf->pfminfo.vlinegap = vlinegap;
	    sf->pfminfo.os2_winascent = winascent;
	    sf->pfminfo.os2_windescent = windescent;
	    sf->pfminfo.winascent_add = winaoff;
	    sf->pfminfo.windescent_add = windoff;
	    sf->pfminfo.os2_typoascent = tascent;
	    sf->pfminfo.os2_typodescent = tdescent;
	    sf->pfminfo.typoascent_add = taoff;
	    sf->pfminfo.typodescent_add = tdoff;
	    sf->pfminfo.hhead_ascent = hascent;
	    sf->pfminfo.hhead_descent = hdescent;
	    sf->pfminfo.hheadascent_add = haoff;
	    sf->pfminfo.hheaddescent_add = hdoff;
	    sf->pfminfo.pfmset = true;

	    sf->pfminfo.subsuper_set = !GGadgetIsChecked(GWidgetGetControl(gw,CID_PanDefault));
	    if ( sf->pfminfo.subsuper_set ) {
		sf->pfminfo.os2_subxsize = subs[0];
		sf->pfminfo.os2_subysize = subs[1];
		sf->pfminfo.os2_subxoff = subs[2];
		sf->pfminfo.os2_subyoff = subs[3];
		sf->pfminfo.os2_supxsize = super[0];
		sf->pfminfo.os2_supysize = super[1];
		sf->pfminfo.os2_supxoff = super[2];
		sf->pfminfo.os2_supyoff = super[3];
		sf->pfminfo.os2_strikeysize = strike[0];
		sf->pfminfo.os2_strikeypos = strike[1];
	    }
	}
	if ( order2!=sf->order2 ) {
	    if ( order2 )
		SFConvertToOrder2(sf);
	    else
		SFConvertToOrder3(sf);
	}
	if ( retitle_fv ) { FontView *fvs;
	    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame )
		FVSetTitle(fvs);
	}
	if ( reformat_fv )
	    FontViewReformatAll(sf);
	sf->changed = true;
	sf->changed_since_autosave = true;
	sf->changed_since_xuidchanged = !xuidchanged;
	/* Just in case they changed the blue values and we are showing blues */
	/*  in outline views... */
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    CharView *cv;
	    for ( cv = sf->glyphs[i]->views; cv!=NULL; cv=cv->next ) {
		cv->back_img_out_of_date = true;
		GDrawRequestExpose(cv->v,NULL,false);
	    }
	}
	MacFeatListFree(sf->features);
	sf->features = GGadgetGetUserData(GWidgetGetControl(d->gw,CID_Features));
	last_aspect = d->old_aspect;

	MarkClassFree(sf->mark_class_cnt,sf->mark_classes,sf->mark_class_names);
	sf->mark_class_cnt = d->mark_class_cnt;
	sf->mark_classes = d->mark_classes;
	sf->mark_class_names = d->mark_class_names;

	GFI_Close(d);

	SFReplaceFontnameBDFProps(sf);
    }
return( true );
}

static void GFI_AsDsLab(struct gfi_data *d, int cid) {
    int isoffset = GGadgetIsChecked(GWidgetGetControl(d->gw,cid));
    DBounds b;
    int ocid, labcid;
    double val;
    char buf[40];
    char *offt, *baret;
    int ismax=0;
    unichar_t *end;

    switch ( cid ) {
      case CID_WinAscentIsOff:
	offt = _("Win Ascent Offset:"); baret = _("Win Ascent:");
	ocid = CID_WinAscent; labcid = CID_WinAscentLab;
	ismax = true;
      break;
      case CID_WinDescentIsOff:
	offt = _("Win Descent Offset:"); baret = _("Win Descent:");
	ocid = CID_WinDescent; labcid = CID_WinDescentLab;
      break;
      case CID_TypoAscentIsOff:
	offt = _("Typo Ascent Offset:"); baret = _("Typo Ascent:");
	ocid = CID_TypoAscent; labcid = CID_TypoAscentLab;
	ismax = true;
      break;
      case CID_TypoDescentIsOff:
	offt = _("Typo Descent Offset:"); baret = _("Typo Descent:");
	ocid = CID_TypoDescent; labcid = CID_TypoDescentLab;
      break;
      case CID_HHeadAscentIsOff:
	offt = _("HHead Ascent Offset:"); baret = _("HHead Ascent:");
	ocid = CID_HHeadAscent; labcid = CID_HHeadAscentLab;
	ismax = true;
      break;
      case CID_HHeadDescentIsOff:
	offt = _("HHead Descent Offset:"); baret = _("HHead Descent:");
	ocid = CID_HHeadDescent; labcid = CID_HHeadDescentLab;
      break;
      default:
return;
    }

    GGadgetSetTitle8(GWidgetGetControl(d->gw,labcid),
	    isoffset?offt:baret);
    if ( cid == CID_TypoAscentIsOff ) {
	const unichar_t *as = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Ascent));
	double av=u_strtod(as,&end);
	b.maxy = *end=='\0' ? av : d->sf->ascent;
    } else if ( cid == CID_TypoDescentIsOff ) {
	const unichar_t *ds = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
	double dv=u_strtod(ds,&end);
	b.miny = *end=='\0' ? -dv : -d->sf->descent;
    } else {
	CIDFindBounds(d->sf,&b);
	if ( cid == CID_WinDescentIsOff ) b.miny = -b.miny;
    }

    val = u_strtod(_GGadgetGetTitle(GWidgetGetControl(d->gw,ocid)),NULL);
    if ( isoffset )
	sprintf( buf,"%g",rint( val-(ismax ? b.maxy : b.miny)) );
    else
	sprintf( buf,"%g",rint( val+(ismax ? b.maxy : b.miny)) );
    GGadgetSetTitle8(GWidgetGetControl(d->gw,ocid),buf);
}

static int GFI_AsDesIsOff(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GFI_AsDsLab(d,GGadgetGetCid(g));
    }
return( true );
}

static void _GFI_PanoseDefault(struct gfi_data *d) {
    int i;
    int isdefault = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_PanDefault));

    for ( i=0; i<10; ++i ) {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PanFamily+i),!isdefault);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PanFamilyLab+i),!isdefault);
    }
    if ( isdefault ) {
	char *n = cu_copy(_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Fontname)));
	struct pfminfo info;
	memset(&info,0,sizeof(info));
	SFDefaultOS2Info(&info,d->sf,n);
	free(n);
	for ( i=0; i<10; ++i )
	    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_PanFamily+i),info.panose[i]);
    }
}

static int GFI_PanoseDefault(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	_GFI_PanoseDefault(d);
    }
return( true );
}

static void GFI_SubSuperSet(struct gfi_data *d, struct pfminfo *info) {
    char buffer[40];
    unichar_t ubuf[40];

    sprintf( buffer, "%d", info->os2_subxsize );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SubXSize),ubuf);

    sprintf( buffer, "%d", info->os2_subysize );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SubYSize),ubuf);

    sprintf( buffer, "%d", info->os2_subxoff );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SubXOffset),ubuf);

    sprintf( buffer, "%d", info->os2_subyoff );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SubYOffset),ubuf);


    sprintf( buffer, "%d", info->os2_supxsize );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SuperXSize),ubuf);

    sprintf( buffer, "%d", info->os2_supysize );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SuperYSize),ubuf);

    sprintf( buffer, "%d", info->os2_supxoff );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SuperXOffset),ubuf);

    sprintf( buffer, "%d", info->os2_supyoff );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_SuperYOffset),ubuf);


    sprintf( buffer, "%d", info->os2_strikeysize );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_StrikeoutSize),ubuf);

    sprintf( buffer, "%d", info->os2_strikeypos );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_StrikeoutPos),ubuf);
}

static void _GFI_SubSuperDefault(struct gfi_data *d) {
    int i;
    int isdefault = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_SubSuperDefault));

    for ( i=0; i<10; ++i )
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SubXSize+i),!isdefault);
    if ( isdefault ) {
	const unichar_t *as = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Ascent));
	const unichar_t *ds = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
	unichar_t *aend, *dend;
	double av=u_strtod(as,&aend),dv=u_strtod(ds,&dend);
	struct pfminfo info;
	if ( *aend!='\0' ) av = d->sf->ascent;
	if ( *dend!='\0' ) dv = d->sf->descent;
	memset(&info,0,sizeof(info));
	SFDefaultOS2SubSuper(&info,(int) (dv+av));
	GFI_SubSuperSet(d,&info);
    }
}

static int GFI_SubSuperDefault(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	_GFI_SubSuperDefault(d);
    }
return( true );
}

static void TTFSetup(struct gfi_data *d) {
    struct pfminfo info;
    char buffer[10]; unichar_t ubuf[10];
    int i, lg, vlg, tlg;
    const unichar_t *as = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Ascent));
    const unichar_t *ds = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
    unichar_t *aend, *dend;
    double av=u_strtod(as,&aend),dv=u_strtod(ds,&dend);

    info = d->sf->pfminfo;
    if ( !info.pfmset ) {
	/* Base this stuff on the CURRENT name */
	/* if the user just created a font, and named it *Bold, then the sf */
	/*  won't yet have Bold in its name, and basing the weight on it would*/
	/*  give the wrong answer. That's why we don't do this init until we */
	/*  get to one of the ttf aspects, it gives the user time to set the */
	/*  name properly */
	/* And on CURRENT values of ascent and descent */
	char *n = cu_copy(_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Fontname)));
	if ( *aend=='\0' && *dend=='\0' ) {
	    if ( info.linegap==0 )
		info.linegap = rint(.09*(av+dv));
	    if ( info.vlinegap==0 )
		info.vlinegap = info.linegap;
	    if ( info.os2_typolinegap==0 )
		info.os2_typolinegap = info.linegap;
	}
	lg = info.linegap; vlg = info.vlinegap; tlg = info.os2_typolinegap;
	SFDefaultOS2Info(&info,d->sf,n);
	if ( lg != 0 )
	    info.linegap = lg;
	if ( vlg!= 0 )
	    info.vlinegap = vlg;
	if ( tlg!=0 )
	    info.os2_typolinegap = tlg;
	free(n);
    }

    if ( info.weight>0 && info.weight<=900 && info.weight%100==0 )
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_WeightClass),
		(char *) weightclass[info.weight/100-1].text);
    else {
	sprintf( buffer, "%d", info.weight );
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_WeightClass),buffer);
    }
    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_WidthClass),info.width-1);
    for ( i=0; pfmfamily[i].text!=NULL; ++i )
	if ( (intpt) (pfmfamily[i].userdata)==info.pfmfamily ) {
	    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_PFMFamily),i);
    break;
	}
    for ( i=0; ibmfamily[i].text!=NULL; ++i )
	if ( (intpt) (ibmfamily[i].userdata)==info.os2_family_class ) {
	    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_IBMFamily),i);
    break;
	}
    if ( info.os2_vendor[0]!='\0' ) {
	ubuf[0] = info.os2_vendor[0];
	ubuf[1] = info.os2_vendor[1];
	ubuf[2] = info.os2_vendor[2];
	ubuf[3] = info.os2_vendor[3];
	ubuf[4] = 0;
    } else if ( TTFFoundry!=NULL )
	uc_strncpy(ubuf,TTFFoundry,4);
    else
	uc_strcpy(ubuf,"PfEd");
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Vendor),ubuf);


    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_PanDefault),!info.panose_set);
    _GFI_PanoseDefault(d);
    if ( info.panose_set )
	for ( i=0; i<10; ++i )
	    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_PanFamily+i),info.panose[i]);

    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_SubSuperDefault),!info.subsuper_set);
    if ( info.subsuper_set )
	GFI_SubSuperSet(d,&info);
    _GFI_SubSuperDefault(d);

    d->ttf_set = true;
    /* FSType is already set */
    sprintf( buffer, "%d", info.linegap );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_LineGap),ubuf);
    sprintf( buffer, "%d", info.vlinegap );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_VLineGap),ubuf);
    sprintf( buffer, "%d", info.os2_typolinegap );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TypoLineGap),ubuf);

    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_WinAscentIsOff),info.winascent_add);
    GFI_AsDsLab(d,CID_WinAscentIsOff);
    sprintf( buffer, "%d", info.os2_winascent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WinAscent),ubuf);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_WinDescentIsOff),info.windescent_add);
    GFI_AsDsLab(d,CID_WinDescentIsOff);
    sprintf( buffer, "%d", info.os2_windescent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WinDescent),ubuf);

    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TypoAscentIsOff),info.typoascent_add);
    GFI_AsDsLab(d,CID_TypoAscentIsOff);
    sprintf( buffer, "%d", info.os2_typoascent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TypoAscent),ubuf);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TypoDescentIsOff),info.typodescent_add);
    GFI_AsDsLab(d,CID_TypoDescentIsOff);
    sprintf( buffer, "%d", info.os2_typodescent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TypoDescent),ubuf);

    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_HHeadAscentIsOff),info.hheadascent_add);
    GFI_AsDsLab(d,CID_HHeadAscentIsOff);
    sprintf( buffer, "%d", info.hhead_ascent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_HHeadAscent),ubuf);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_HHeadDescentIsOff),info.hheaddescent_add);
    GFI_AsDsLab(d,CID_HHeadDescentIsOff);
    sprintf( buffer, "%d", info.hhead_descent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_HHeadDescent),ubuf);
}

static char *mathparams[] = {
/* GT: TeX parameters for math fonts. "Num" means numerator, "Denom" */
/* GT: means denominator, "Sup" means superscript, "Sub" means subscript */
    N_("Num1:"),
    N_("Num2:"),  N_("Num3:"), N_("Denom1:"),
    N_("Denom2:"), N_("Sup1:"), N_("Sup2:"), N_("Sup3:"), N_("Sub1:"), N_("Sub2:"),
    N_("SupDrop:"), N_("SubDrop:"), N_("Delim1:"), N_("Delim2:"), N_("Axis Ht:"),
    0 };
static char *mathpopups[] = { N_("Amount to raise baseline for numerators in display styles"),
    N_("Amount to raise baseline for numerators in non-display styles"),
    N_("Amount to raise baseline for numerators in non-display atop styles"),
    N_("Amount to lower baseline for denominators in display styles"),
    N_("Amount to lower baseline for denominators in non-display styles"),
    N_("Amount to raise baseline for superscripts in display styles"),
    N_("Amount to raise baseline for superscripts in non-display styles"),
    N_("Amount to raise baseline for superscripts in modified styles"),
    N_("Amount to lower baseline for subscripts in display styles"),
    N_("Amount to lower baseline for subscripts in non-display styles"),
    N_("Amount above top of large box to place baseline of superscripts"),
    N_("Amount below bottom of large box to place baseline of subscripts"),
    N_("Size of comb delimiters in display styles"), 
    N_("Size of comb delimiters in non-display styles"),
    N_("Height of fraction bar above base line"),
    0 };
/* GT: Default Rule Thickness. A rule being a typographic term for a straight */
/* GT: black line on a printed page. */
static char *extparams[] = { N_("Def Rule Thick:"),
/* GT: I don't really understand these "Big Op Space" things. They have */
/* GT: something to do with TeX and are roughly defined a few strings down */
	N_("Big Op Space1:"),
	N_("Big Op Space2:"),
	N_("Big Op Space3:"),
	N_("Big Op Space4:"),
	N_("Big Op Space5:"), 0 };
static char *extpopups[] = { N_("Default thickness of over and overline bars"),
	N_("The minimum glue space above a large displayed operator"),
	N_("The minimum glue space below a large displayed operator"),
	N_("The minimum distance between a limit's baseline and a large displayed\noperator when the limit is above the operator"),
	N_("The minimum distance between a limit's baseline and a large displayed\noperator when the limit is below the operator"),
	N_("The extra glue place above and below displayed limits"),
	0 };

static int mp_e_h(GWindow gw, GEvent *event) {
    int i;

    if ( event->type==et_close ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	d->mpdone = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	if ( GGadgetGetCid(event->u.control.g)) {
	    int err=false;
	    double em = (d->sf->ascent+d->sf->descent), val;
	    char **params;
	    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMath)) )
		params = mathparams;
	    else
		params = extparams;
	    for ( i=0; params[i]!=0 && !err; ++i ) {
		val = GetReal8(gw,CID_TeX+i,params[i],&err);
		if ( !err )
		    d->texdata.params[i+7] = rint( val/em * (1<<20) );
	    }
	    if ( !err )
		d->mpdone = true;
	} else
	    d->mpdone = true;
    }
return( true );
}

static int GFI_MoreParams(GGadget *g, GEvent *e) {
    int tot;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData txgcd[35];
    GTextInfo txlabel[35];
    int i,y,k;
    char **params, **popups;
    char values[20][20];

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXText)) )
return( true );
	else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMath)) ) {
	    tot = 22-7;
	    params = mathparams;
	    popups = mathpopups;
	} else {
	    tot = 13-7;
	    params = extparams;
	    popups = extpopups;
	}

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.is_dlg = true;
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
/* GT: More Parameters */
	wattrs.utf8_window_title = _("More Params");
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,GGadgetScale(180));
	pos.height = GDrawPointsToPixels(NULL,tot*26+60);
	gw = GDrawCreateTopWindow(NULL,&pos,mp_e_h,d,&wattrs);

	memset(&txlabel,0,sizeof(txlabel));
	memset(&txgcd,0,sizeof(txgcd));

	k=0; y = 10;
	for ( i=0; params[i]!=0; ++i ) {
	    txlabel[k].text = (unichar_t *) params[i];
	    txlabel[k].text_is_1byte = true;
	    txgcd[k].gd.label = &txlabel[k];
	    txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = y+4;
	    txgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	    txgcd[k].gd.popup_msg = (unichar_t *) popups[i];
	    txgcd[k++].creator = GLabelCreate;

	    sprintf( values[i], "%g", d->texdata.params[i+7]*(double) (d->sf->ascent+d->sf->descent)/(double) (1<<20));
	    txlabel[k].text = (unichar_t *) values[i];
	    txlabel[k].text_is_1byte = true;
	    txgcd[k].gd.label = &txlabel[k];
	    txgcd[k].gd.pos.x = 85; txgcd[k].gd.pos.y = y;
	    txgcd[k].gd.pos.width = 75;
	    txgcd[k].gd.flags = gg_visible | gg_enabled;
	    txgcd[k].gd.cid = CID_TeX + i;
	    txgcd[k++].creator = GTextFieldCreate;
	    y += 26;
	}

	txgcd[k].gd.pos.x = 30-3; txgcd[k].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
	txgcd[k].gd.pos.width = -1; txgcd[k].gd.pos.height = 0;
	txgcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
	txlabel[k].text = (unichar_t *) _("_OK");
	txlabel[k].text_is_1byte = true;
	txlabel[k].text_in_resource = true;
	txgcd[k].gd.label = &txlabel[k];
	txgcd[k].gd.cid = true;
	txgcd[k++].creator = GButtonCreate;

	txgcd[k].gd.pos.x = -30; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y+3;
	txgcd[k].gd.pos.width = -1; txgcd[k].gd.pos.height = 0;
	txgcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	txlabel[k].text = (unichar_t *) _("_Cancel");
	txlabel[k].text_is_1byte = true;
	txlabel[k].text_in_resource = true;
	txgcd[k].gd.label = &txlabel[k];
	txgcd[k].gd.cid = false;
	txgcd[k++].creator = GButtonCreate;

	txgcd[k].gd.pos.x = 2; txgcd[k].gd.pos.y = 2;
	txgcd[k].gd.pos.width = pos.width-4; txgcd[k].gd.pos.height = pos.height-4;
	txgcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	txgcd[k].creator = GGroupCreate;

	GGadgetsCreate(gw,txgcd);
	d->mpdone = false;
	GDrawSetVisible(gw,true);

	while ( !d->mpdone )
	    GDrawProcessOneEvent(NULL);
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int GFI_TeXChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetGetCid(g)==CID_TeXText ) {
	    GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_TeXExtraSpLabel),
/* GT: Extra Space */
		    _("Extra Sp:"));
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MoreParams),false);
	} else {
	    GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_TeXExtraSpLabel),
		    _("Math Sp:"));
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MoreParams),true);
	}
    }
return( true );
}

static void DefaultTeX(struct gfi_data *d) {
    char buffer[20];
    int i;
    SplineFont *sf = d->sf;

    d->tex_set = true;

    if ( sf->texdata.type==tex_unset ) {
	TeXDefaultParams(sf);
	d->texdata = sf->texdata;
    }

    for ( i=0; i<7; ++i ) {
	sprintf( buffer,"%g", d->texdata.params[i]*(sf->ascent+sf->descent)/(double) (1<<20));
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_TeX+i),buffer);
    }
    if ( sf->texdata.type==tex_math )
	GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TeXMath), true);
    else if ( sf->texdata.type == tex_mathext )
	GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TeXMathExt), true);
    else {
	GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TeXText), true);
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_TeXExtraSpLabel),
/* GT: Extra Space */
		_("Extra Sp:"));
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MoreParams),false);
    }
}

static int GFI_MacAutomatic(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int autom = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_MacAutomatic));
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MacStyles),!autom);
    }
return( true );
}

static int GFI_AspectChange(GGadget *g, GEvent *e) {
    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_radiochanged )) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int new_aspect = GTabSetGetSel(g);
	if ( d->tn_smallactive!=-1 )
	    TN_FinishSmallEdit(d);
	if ( !d->ttf_set && new_aspect == d->ttfv_aspect )
	    TTFSetup(d);
	else if ( !d->names_set && new_aspect == d->tn_aspect ) {
	    InitTTFNames(d);
	} else if ( !d->tex_set && new_aspect == d->tx_aspect )
	    DefaultTeX(d);
	d->old_aspect = new_aspect;
    }
return( true );
}

static void GFI_Resize(struct gfi_data *d) {
    GRect size, temp, temp2, subsize;
    int yoff, xoff, xw, yh, i, okdown;
    static int xy_cids[] = { CID_Notice, CID_Comment, CID_Contextual, CID_StateMachine, CID_TTFTabs, 0 };
    static int x_cids[] = { CID_PrivateEntries, CID_PrivateValues, CID_MarkClasses, CID_AnchorClasses, 0 };
    int sbsize = GDrawPointsToPixels(d->gw,_GScrollBar_Width);

    GDrawGetSize(GGadgetGetWindow(GWidgetGetControl(d->gw,CID_TTFTabs)),&subsize);

    GDrawGetSize(d->gw,&size);
    GGadgetResize(GWidgetGetControl(d->gw,CID_MainGroup),size.width-4,size.height-4);
    GGadgetGetSize(GWidgetGetControl(d->gw,CID_OK),&temp);
    okdown = temp.height+GDrawPointsToPixels(d->gw,3)+2+GDrawPointsToPixels(d->gw,2);
    yoff = (size.height - okdown) - temp.y;
    GGadgetMove(GWidgetGetControl(d->gw,CID_OK),temp.x,temp.y+yoff);
    GGadgetGetSize(GWidgetGetControl(d->gw,CID_Cancel),&temp2);
    GGadgetMove(GWidgetGetControl(d->gw,CID_Cancel),size.width-temp2.width-temp.x,temp2.y+yoff);
    GGadgetGetSize(GWidgetGetControl(d->gw,CID_Tabs),&temp);
    xoff = size.width-2*GDrawPointsToPixels(d->gw,4) - temp.width;
    GGadgetResize(GWidgetGetControl(d->gw,CID_Tabs),temp.width+xoff,
	    temp.height+yoff);
    subsize.height = size.height- subsize.y- okdown-GDrawPointsToPixels(d->gw,8); subsize.width += xoff;

    for ( i=0; xy_cids[i]!=0; ++i ) {
	GGadgetGetSize(GWidgetGetControl(d->gw,xy_cids[i]),&temp);
	if ( (xw = subsize.width - temp.x - GDrawPointsToPixels(d->gw,3)) < 40 )
	    xw = 40;
	if ( (yh = subsize.height - temp.y - GDrawPointsToPixels(d->gw,3)) < 40 )
	    yh = 40;
	GGadgetResize(GWidgetGetControl(d->gw,xy_cids[i]),xw,yh);
    }

    for ( i=0; x_cids[i]!=0; ++i ) {
	GGadgetGetSize(GWidgetGetControl(d->gw,x_cids[i]),&temp);
	if ( (xw = subsize.width - temp.x - GDrawPointsToPixels(d->gw,3)) < 40 )
	    xw = 40;
	GGadgetResize(GWidgetGetControl(d->gw,x_cids[i]),xw,temp.height);
    }

    if ( d->tn_v!=NULL ) {
	int len;
	GDrawGetSize(d->tn_v,&temp);
	d->tn_width = subsize.width-2*temp.x-sbsize;
	d->tn_height = subsize.height-temp.y-GDrawPointsToPixels(d->gw,3)-sbsize;
	GDrawResize(d->tn_v,d->tn_width,d->tn_height);
	GGadgetResize(GWidgetGetControl(d->gw,CID_TNVScroll),sbsize,d->tn_height);
	GGadgetResize(GWidgetGetControl(d->gw,CID_TNHScroll),d->tn_width,sbsize);
	GGadgetMove(GWidgetGetControl(d->gw,CID_TNVScroll),temp.x+d->tn_width,temp.y);
	GGadgetMove(GWidgetGetControl(d->gw,CID_TNHScroll),temp.x,temp.y+d->tn_height);
	TTFN_SetSBs(d);
	len = d->tn_width - d->tn_strstart + 1;
	if ( len<60 )
	    len = 60;
	GGadgetResize(d->tn_smalledit,len,d->tn_fh);
    }

    GDrawRequestExpose(d->gw,NULL,false);
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	GFI_CancelClose(d);
    } else if ( event->type==et_destroy ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	free(d);
    } else if ( event->type==et_resize ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	GFI_Resize(d);
    } else if ( event->type==et_char ) {
return( GFI_Char(GDrawGetUserData(gw),event));
    }
return( true );
}

static int OrderGSUB(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	OrderTable(d->sf,CHR('G','S','U','B'));
    }
return( true );
}

static void GFI_InitMarkClasses(struct gfi_data *d) {
    SplineFont *sf = d->sf;
    int i;

    d->mark_class_cnt = sf->mark_class_cnt;
    if ( d->mark_class_cnt!=0 ) {
	d->mark_classes = galloc(d->mark_class_cnt*sizeof(char *));
	d->mark_class_names = galloc(d->mark_class_cnt*sizeof(unichar_t *));
	d->mark_classes[0] = NULL; d->mark_class_names[0] = NULL;
	for ( i=1; i<d->mark_class_cnt; ++i ) {
	    d->mark_classes[i] = copy(sf->mark_classes[i]);
	    d->mark_class_names[i] = copy(sf->mark_class_names[i]);
	}
    }
}

void FontInfo(SplineFont *sf,int defaspect,int sync) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GTabInfo aspects[18], conaspects[7], smaspects[5], vaspects[5];
    GGadgetCreateData mgcd[10], ngcd[13], psgcd[30], tngcd[8],
	pgcd[8], vgcd[16], pangcd[22], comgcd[3], atgcd[7], txgcd[23],
	congcd[3], csubgcd[fpst_max-pst_contextpos][6], smgcd[3], smsubgcd[4][6],
	mfgcd[8], mcgcd[8], szgcd[19], mkgcd[5], metgcd[28], vagcd[3], ssgcd[23],
	xugcd[7];
    GTextInfo mlabel[10], nlabel[12], pslabel[30], tnlabel[7],
	plabel[8], vlabel[16], panlabel[22], comlabel[3], atlabel[7], txlabel[23],
	csublabel[fpst_max-pst_contextpos][6], smsublabel[4][6],
	mflabel[8], mclabel[8], szlabel[17], mklabel[5], metlabel[28],
	sslabel[23], xulabel[6];
    GTextInfo *namelistnames;
    struct gfi_data *d;
    char iabuf[20], upbuf[20], uwbuf[20], asbuf[20], dsbuf[20],
	    vbuf[20], uibuf[12], vorig[20], embuf[20];
    char dszbuf[20], dsbbuf[20], dstbuf[21], sibuf[20], swbuf[20];
    int i,k;
    int mcs;
    FontView *fvs;
    char title[130];
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    static unichar_t sans[] = { 'h','e','l','v','e','t','i','c','a',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    FontRequest rq;
    GFont *font;
    int sbwidth;
    static char *connames[] = {
/* GT: Contextual Positioning. For definition of these terms see */
/* GT: http://partners.adobe.com/public/developer/opentype/index_table_formats2.html */
	    N_("Context Pos"),
/* GT: Contextual Substitution */
	    N_("Context Sub"),
/* GT: Contextual Chaining Positioning */
	    N_("Chain Pos"),
/* GT: Contextual Chaining Substitution */
	    N_("Chain Sub"),
	    N_("Reverse Chain Sub"), NULL };
    static int contypes[] = { pst_contextpos, pst_contextsub, pst_chainpos, pst_chainsub, pst_reversesub, 0 };
    static char *smnames[] = {
	    N_("Indic"),
/* GT: Contextual Substitution */
	    N_("Context Sub"),
/* GT: Contextual Insertions */
	    N_("Context Ins"),
	    N_("Kerning"), NULL };
    static int smtypes[] = { asm_indic, asm_context, asm_insert, asm_kern };
    static int done = false;
    char **nlnames;

    FontInfoInit();
    if ( !done ) {
	for ( i=0; connames[i]!=NULL; ++i )
	    connames[i] = _(connames[i]);
	for ( i=0; smnames[i]!=NULL; ++i )
	    smnames[i] = _(smnames[i]);
	done = true;
    }

    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	if ( fvs->fontinfo ) {
	    GDrawSetVisible(((struct gfi_data *) (fvs->fontinfo))->gw,true);
	    GDrawRaise( ((struct gfi_data *) (fvs->fontinfo))->gw );
return;
	}
    }
    if ( defaspect==-1 )
	defaspect = last_aspect;

    d = gcalloc(1,sizeof(struct gfi_data));
    if ( sf->fv!=NULL )
	sf->fv->fontinfo = d;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    if ( sync ) {
	wattrs.mask |= wam_restrict;
	wattrs.restrict_input_to_me = 1;
    }
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    snprintf(title,sizeof(title),_("Font Information for %.90s"),
	    sf->fontname);
    wattrs.utf8_window_title = title;
    pos.x = pos.y = 0;
#ifndef FONTFORGE_CONFIG_INFO_HORIZONTAL
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268+85));
#else
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268));
#endif
    pos.height = GDrawPointsToPixels(NULL,375);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,d,&wattrs);

    d->sf = sf;
    d->gw = gw;
    d->old_sel = -2;
    d->texdata = sf->texdata;

    memset(&nlabel,0,sizeof(nlabel));
    memset(&ngcd,0,sizeof(ngcd));

    nlabel[0].text = (unichar_t *) _("Fo_ntname:");
    nlabel[0].text_is_1byte = true;
    nlabel[0].text_in_resource = true;
    ngcd[0].gd.label = &nlabel[0];
    ngcd[0].gd.pos.x = 12; ngcd[0].gd.pos.y = 6+6; 
    ngcd[0].gd.flags = gg_visible | gg_enabled;
    ngcd[0].creator = GLabelCreate;

    ngcd[1].gd.pos.x = 115; ngcd[1].gd.pos.y = ngcd[0].gd.pos.y-6; ngcd[1].gd.pos.width = 137;
    ngcd[1].gd.flags = gg_visible | gg_enabled;
    nlabel[1].text = (unichar_t *) sf->fontname;
    nlabel[1].text_is_1byte = true;
    ngcd[1].gd.label = &nlabel[1];
    ngcd[1].gd.cid = CID_Fontname;
    ngcd[1].gd.handle_controlevent = GFI_NameChange;
    ngcd[1].creator = GTextFieldCreate;

    nlabel[2].text = (unichar_t *) _("_Family Name:");
    nlabel[2].text_is_1byte = true;
    nlabel[2].text_in_resource = true;
    ngcd[2].gd.label = &nlabel[2];
    ngcd[2].gd.pos.x = 12; ngcd[2].gd.pos.y = ngcd[0].gd.pos.y+26; 
    ngcd[2].gd.flags = gg_visible | gg_enabled;
    ngcd[2].creator = GLabelCreate;

    ngcd[3].gd.pos.x = ngcd[1].gd.pos.x; ngcd[3].gd.pos.y = ngcd[2].gd.pos.y-6; ngcd[3].gd.pos.width = 137;
    ngcd[3].gd.flags = gg_visible | gg_enabled;
    nlabel[3].text = (unichar_t *) (sf->familyname?sf->familyname:sf->fontname);
    nlabel[3].text_is_1byte = true;
    ngcd[3].gd.label = &nlabel[3];
    ngcd[3].gd.cid = CID_Family;
    ngcd[3].gd.handle_controlevent = GFI_FamilyChange;
    ngcd[3].creator = GTextFieldCreate;
    if ( sf->familyname==NULL || strstr(sf->familyname,"Untitled")==sf->familyname )
	d->family_untitled = true;

    ngcd[4].gd.pos.x = 12; ngcd[4].gd.pos.y = ngcd[3].gd.pos.y+26+6;
    nlabel[4].text = (unichar_t *) _("Name For Human_s:");
    nlabel[4].text_is_1byte = true;
    nlabel[4].text_in_resource = true;
    ngcd[4].gd.label = &nlabel[4];
    ngcd[4].gd.flags = gg_visible | gg_enabled;
    ngcd[4].creator = GLabelCreate;

    ngcd[5].gd.pos.x = 115; ngcd[5].gd.pos.y = ngcd[4].gd.pos.y-6; ngcd[5].gd.pos.width = 137;
    ngcd[5].gd.flags = gg_visible | gg_enabled;
    nlabel[5].text = (unichar_t *) (sf->fullname?sf->fullname:sf->fontname);
    nlabel[5].text_is_1byte = true;
    ngcd[5].gd.label = &nlabel[5];
    ngcd[5].gd.cid = CID_Human;
    ngcd[5].gd.handle_controlevent = GFI_HumanChange;
    ngcd[5].creator = GTextFieldCreate;
    if ( sf->fullname==NULL || strstr(sf->fullname,"Untitled")==sf->fullname )
	d->human_untitled = true;

    nlabel[6].text = (unichar_t *) _("_Weight");
    nlabel[6].text_is_1byte = true;
    nlabel[6].text_in_resource = true;
    ngcd[6].gd.label = &nlabel[6];
    ngcd[6].gd.pos.x = ngcd[4].gd.pos.x; ngcd[6].gd.pos.y = ngcd[4].gd.pos.y+26; 
    ngcd[6].gd.flags = gg_visible | gg_enabled;
    ngcd[6].creator = GLabelCreate;

    ngcd[7].gd.pos.x = ngcd[1].gd.pos.x; ngcd[7].gd.pos.y = ngcd[6].gd.pos.y-6; ngcd[7].gd.pos.width = 137;
    ngcd[7].gd.flags = gg_visible | gg_enabled;
    nlabel[7].text = (unichar_t *) (sf->weight?sf->weight:"Regular");
    nlabel[7].text_is_1byte = true;
    ngcd[7].gd.label = &nlabel[7];
    ngcd[7].gd.cid = CID_Weight;
    ngcd[7].creator = GTextFieldCreate;

    ngcd[8].gd.pos.x = 12; ngcd[8].gd.pos.y = ngcd[7].gd.pos.y+30+6;
    nlabel[8].text = (unichar_t *) _("_Version:");
    nlabel[8].text_is_1byte = true;
    nlabel[8].text_in_resource = true;
    ngcd[8].gd.label = &nlabel[8];
    ngcd[8].gd.flags = gg_visible | gg_enabled;
    ngcd[8].creator = GLabelCreate;

    ngcd[9].gd.pos.x = 115; ngcd[9].gd.pos.y = ngcd[8].gd.pos.y-6; ngcd[9].gd.pos.width = 137;
    ngcd[9].gd.flags = gg_visible | gg_enabled;
    nlabel[9].text = (unichar_t *) (sf->version?sf->version:"");
    nlabel[9].text_is_1byte = true;
    if ( sf->subfontcnt!=0 ) {
	sprintf( vbuf,"%g", sf->cidversion );
	nlabel[9].text = (unichar_t *) vbuf;
    }
    ngcd[9].gd.label = &nlabel[9];
    ngcd[9].gd.cid = CID_Version;
    ngcd[9].creator = GTextFieldCreate;

    ngcd[10].gd.pos.x = 12; ngcd[10].gd.pos.y = ngcd[8].gd.pos.y+22;
    ngcd[10].gd.flags = gg_visible | gg_enabled;
    nlabel[10].text = (unichar_t *) _("Copy_right:");
    nlabel[10].text_is_1byte = true;
    nlabel[10].text_in_resource = true;
    ngcd[10].gd.label = &nlabel[10];
    ngcd[10].creator = GLabelCreate;

    ngcd[11].gd.pos.x = 12; ngcd[11].gd.pos.y = ngcd[10].gd.pos.y+14;
    ngcd[11].gd.pos.width = ngcd[5].gd.pos.x+ngcd[5].gd.pos.width-26;
    ngcd[11].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    if ( sf->copyright!=NULL ) {
	nlabel[11].text = (unichar_t *) sf->copyright;
	nlabel[11].text_is_1byte = true;
	ngcd[11].gd.label = &nlabel[11];
    }
    ngcd[11].gd.cid = CID_Notice;
    ngcd[11].creator = GTextAreaCreate;

/******************************************************************************/
    memset(&xulabel,0,sizeof(xulabel));
    memset(&xugcd,0,sizeof(xugcd));

    xugcd[0].gd.pos.x = 12; xugcd[0].gd.pos.y = 10+6;
    xugcd[0].gd.flags = gg_visible | gg_enabled;
    xulabel[0].text = (unichar_t *) _("_XUID:");
    xulabel[0].text_is_1byte = true;
    xulabel[0].text_in_resource = true;
    xugcd[0].gd.label = &xulabel[0];
    xugcd[0].creator = GLabelCreate;

    xugcd[1].gd.pos.x = 103; xugcd[1].gd.pos.y = xugcd[0].gd.pos.y-6; xugcd[1].gd.pos.width = 142;
    xugcd[1].gd.flags = gg_visible | gg_enabled;
    if ( sf->xuid!=NULL ) {
	xulabel[1].text = (unichar_t *) sf->xuid;
	xulabel[1].text_is_1byte = true;
	xugcd[1].gd.label = &xulabel[1];
    }
    xugcd[1].gd.cid = CID_XUID;
    xugcd[1].creator = GTextFieldCreate;

    xugcd[2].gd.pos.x = 12; xugcd[2].gd.pos.y = xugcd[1].gd.pos.y+26+6;
    xulabel[2].text = (unichar_t *) _("_UniqueID:");
    xulabel[2].text_is_1byte = true;
    xulabel[2].text_in_resource = true;
    xugcd[2].gd.label = &xulabel[2];
    xugcd[2].gd.flags = gg_visible | gg_enabled;
    xugcd[2].creator = GLabelCreate;

    xugcd[3].gd.pos.x = xugcd[1].gd.pos.x; xugcd[3].gd.pos.y = xugcd[2].gd.pos.y-6; xugcd[3].gd.pos.width = 80;
    xugcd[3].gd.flags = gg_visible | gg_enabled;
    xulabel[3].text = (unichar_t *) "";
    xulabel[3].text_is_1byte = true;
    if ( sf->uniqueid!=0 ) {
	sprintf( uibuf, "%d", sf->uniqueid );
	xulabel[3].text = (unichar_t *) uibuf;
    }
    xugcd[3].gd.label = &xulabel[3];
    xugcd[3].gd.cid = CID_UniqueID;
    xugcd[3].creator = GTextFieldCreate;

    xugcd[4].gd.pos.x = 8; xugcd[4].gd.pos.y = xugcd[3].gd.pos.y+26+6;
    xulabel[4].text = (unichar_t *) _("(Adobe now considers XUID/UniqueID unnecessary)");
    xulabel[4].text_is_1byte = true;
    xulabel[4].text_in_resource = true;
    xugcd[4].gd.label = &xulabel[4];
    xugcd[4].gd.flags = gg_visible | gg_enabled;
    xugcd[4].creator = GLabelCreate;

/******************************************************************************/
    memset(&pslabel,0,sizeof(pslabel));
    memset(&psgcd,0,sizeof(psgcd));

    psgcd[0].gd.pos.x = 12; psgcd[0].gd.pos.y = 12;
    psgcd[0].gd.flags = gg_visible | gg_enabled;
    pslabel[0].text = (unichar_t *) _("_Ascent:");
    pslabel[0].text_is_1byte = true;
    pslabel[0].text_in_resource = true;
    psgcd[0].gd.label = &pslabel[0];
    psgcd[0].creator = GLabelCreate;

    psgcd[1].gd.pos.x = 103; psgcd[1].gd.pos.y = psgcd[0].gd.pos.y-6; psgcd[1].gd.pos.width = 47;
    psgcd[1].gd.flags = gg_visible | gg_enabled;
    sprintf( asbuf, "%d", sf->ascent );
    pslabel[1].text = (unichar_t *) asbuf;
    pslabel[1].text_is_1byte = true;
    psgcd[1].gd.label = &pslabel[1];
    psgcd[1].gd.cid = CID_Ascent;
    psgcd[1].gd.handle_controlevent = GFI_EmChanged;
    psgcd[1].creator = GTextFieldCreate;

    psgcd[2].gd.pos.x = 155; psgcd[2].gd.pos.y = psgcd[0].gd.pos.y;
    psgcd[2].gd.flags = gg_visible | gg_enabled;
    pslabel[2].text = (unichar_t *) _("_Descent:");
    pslabel[2].text_is_1byte = true;
    pslabel[2].text_in_resource = true;
    psgcd[2].gd.label = &pslabel[2];
    psgcd[2].creator = GLabelCreate;

    psgcd[3].gd.pos.x = 200; psgcd[3].gd.pos.y = psgcd[1].gd.pos.y; psgcd[3].gd.pos.width = 47;
    psgcd[3].gd.flags = gg_visible | gg_enabled;
    sprintf( dsbuf, "%d", sf->descent );
    pslabel[3].text = (unichar_t *) dsbuf;
    pslabel[3].text_is_1byte = true;
    psgcd[3].gd.label = &pslabel[3];
    psgcd[3].gd.cid = CID_Descent;
    psgcd[3].gd.handle_controlevent = GFI_EmChanged;
    psgcd[3].creator = GTextFieldCreate;

    psgcd[4].gd.pos.x = psgcd[0].gd.pos.x+5; psgcd[4].gd.pos.y = psgcd[0].gd.pos.y+24;
    psgcd[4].gd.flags = gg_visible | gg_enabled;
    pslabel[4].text = (unichar_t *) _("_Em Size:");
    pslabel[4].text_is_1byte = true;
    pslabel[4].text_in_resource = true;
    psgcd[4].gd.label = &pslabel[4];
    psgcd[4].creator = GLabelCreate;

    psgcd[5].gd.pos.x = psgcd[1].gd.pos.x-20; psgcd[5].gd.pos.y = psgcd[1].gd.pos.y+24; psgcd[5].gd.pos.width = 67;
    psgcd[5].gd.flags = gg_visible | gg_enabled;
    sprintf( embuf, "%d", sf->descent+sf->ascent );
    pslabel[5].text = (unichar_t *) embuf;
    pslabel[5].text_is_1byte = true;
    psgcd[5].gd.label = &pslabel[5];
    psgcd[5].gd.cid = CID_Em;
    psgcd[5].gd.u.list = emsizes;
    psgcd[5].gd.handle_controlevent = GFI_EmChanged;
    psgcd[5].creator = GListFieldCreate;

    psgcd[6].gd.pos.x = psgcd[2].gd.pos.x; psgcd[6].gd.pos.y = psgcd[4].gd.pos.y-4;
    psgcd[6].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    pslabel[6].text = (unichar_t *) _("_Scale Outlines");
    pslabel[6].text_is_1byte = true;
    pslabel[6].text_in_resource = true;
    psgcd[6].gd.label = &pslabel[6];
    psgcd[6].gd.cid = CID_Scale;
    psgcd[6].creator = GCheckBoxCreate;

/* I've reversed the label, text field order in the gcd here because */
/*  that way the text field will be displayed on top of the label rather */
/*  than the reverse, and in Russian that's important translation of */
/*  "Italic Angle" is too long. Oops, no it's the next one, might as well leave in here though */
    psgcd[8].gd.pos.x = 12; psgcd[8].gd.pos.y = psgcd[5].gd.pos.y+26+6;
    psgcd[8].gd.flags = gg_visible | gg_enabled;
    pslabel[8].text = (unichar_t *) _("_Italic Angle:");
    pslabel[8].text_is_1byte = true;
    pslabel[8].text_in_resource = true;
    psgcd[8].gd.label = &pslabel[8];
    psgcd[8].creator = GLabelCreate;

    psgcd[7].gd.pos.x = 103; psgcd[7].gd.pos.y = psgcd[8].gd.pos.y-6;
    psgcd[7].gd.pos.width = 47;
    psgcd[7].gd.flags = gg_visible | gg_enabled;
    sprintf( iabuf, "%g", sf->italicangle );
    pslabel[7].text = (unichar_t *) iabuf;
    pslabel[7].text_is_1byte = true;
    psgcd[7].gd.label = &pslabel[7];
    psgcd[7].gd.cid = CID_ItalicAngle;
    psgcd[7].creator = GTextFieldCreate;

    psgcd[9].gd.pos.y = psgcd[7].gd.pos.y;
    psgcd[9].gd.pos.width = -1; psgcd[9].gd.pos.height = 0;
    psgcd[9].gd.pos.x = psgcd[3].gd.pos.x+psgcd[3].gd.pos.width-
	    GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    /*if ( strstrmatch(sf->fontname,"Italic")!=NULL ||strstrmatch(sf->fontname,"Oblique")!=NULL )*/
	psgcd[9].gd.flags = gg_visible | gg_enabled;
    pslabel[9].text = (unichar_t *) _("_Guess");
    pslabel[9].text_is_1byte = true;
    pslabel[9].text_in_resource = true;
    psgcd[9].gd.label = &pslabel[9];
    psgcd[9].gd.handle_controlevent = GFI_GuessItalic;
    psgcd[9].creator = GButtonCreate;

/* I've reversed the label, text field order in the gcd here because */
/*  that way the text field will be displayed on top of the label rather */
/*  than the reverse, and in Russian that's important translation of */
/*  "Underline position" is too long. */
    psgcd[11].gd.pos.x = 12; psgcd[11].gd.pos.y = psgcd[7].gd.pos.y+26+6;
    psgcd[11].gd.flags = gg_visible | gg_enabled;
    pslabel[11].text = (unichar_t *) _("Underline _Position:");
    pslabel[11].text_is_1byte = true;
    pslabel[11].text_in_resource = true;
    psgcd[11].gd.label = &pslabel[11];
    psgcd[11].creator = GLabelCreate;

    psgcd[10].gd.pos.x = 103; psgcd[10].gd.pos.y = psgcd[11].gd.pos.y-6; psgcd[10].gd.pos.width = 47;
    psgcd[10].gd.flags = gg_visible | gg_enabled;
    sprintf( upbuf, "%g", sf->upos );
    pslabel[10].text = (unichar_t *) upbuf;
    pslabel[10].text_is_1byte = true;
    psgcd[10].gd.label = &pslabel[10];
    psgcd[10].gd.cid = CID_UPos;
    psgcd[10].creator = GTextFieldCreate;

    psgcd[12].gd.pos.x = 155; psgcd[12].gd.pos.y = psgcd[11].gd.pos.y;
    psgcd[12].gd.flags = gg_visible | gg_enabled;
    pslabel[12].text = (unichar_t *) _("_Height:");
    pslabel[12].text_is_1byte = true;
    pslabel[12].text_in_resource = true;
    psgcd[12].gd.label = &pslabel[12];
    psgcd[12].creator = GLabelCreate;

    psgcd[13].gd.pos.x = 200; psgcd[13].gd.pos.y = psgcd[10].gd.pos.y; psgcd[13].gd.pos.width = 47;
    psgcd[13].gd.flags = gg_visible | gg_enabled;
    sprintf( uwbuf, "%g", sf->uwidth );
    pslabel[13].text = (unichar_t *) uwbuf;
    pslabel[13].text_is_1byte = true;
    psgcd[13].gd.label = &pslabel[13];
    psgcd[13].gd.cid = CID_UWidth;
    psgcd[13].creator = GTextFieldCreate;

    psgcd[14].gd.pos.x = 12; psgcd[14].gd.pos.y = psgcd[13].gd.pos.y+26;
    pslabel[14].text = (unichar_t *) _("Has _Vertical Metrics");
    pslabel[14].text_is_1byte = true;
    pslabel[14].text_in_resource = true;
    psgcd[14].gd.label = &pslabel[14];
    psgcd[14].gd.cid = CID_HasVerticalMetrics;
    psgcd[14].gd.flags = gg_visible | gg_enabled;
    if ( sf->hasvmetrics )
	psgcd[14].gd.flags |= gg_cb_on;
    psgcd[14].gd.handle_controlevent = GFI_VMetricsCheck;
    psgcd[14].creator = GCheckBoxCreate;

    psgcd[15].gd.pos.x = 12; psgcd[15].gd.pos.y = psgcd[14].gd.pos.y+22;
    pslabel[15].text = (unichar_t *) _("Vertical _Origin:");
    pslabel[15].text_is_1byte = true;
    pslabel[15].text_in_resource = true;
    psgcd[15].gd.label = &pslabel[15];
    psgcd[15].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled) : gg_visible;
    psgcd[15].gd.cid = CID_VOriginLab;
    psgcd[15].creator = GLabelCreate;

    psgcd[16].gd.pos.x = psgcd[7].gd.pos.x; psgcd[16].gd.pos.y = psgcd[15].gd.pos.y-4; psgcd[16].gd.pos.width = psgcd[15].gd.pos.width;
    psgcd[16].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled) : gg_visible;
    pslabel[16].text = (unichar_t *) "";
    pslabel[16].text_is_1byte = true;
    if ( sf->vertical_origin!=0 || sf->hasvmetrics ) {
	sprintf( vorig, "%d", sf->vertical_origin );
	pslabel[16].text = (unichar_t *) vorig;
    }
    psgcd[16].gd.label = &pslabel[16];
    psgcd[16].gd.cid = CID_VOrigin;
    psgcd[16].creator = GTextFieldCreate;

    psgcd[17].gd.pos.x = 12; psgcd[17].gd.pos.y = psgcd[16].gd.pos.y+22;
    pslabel[17].text = (unichar_t *) _("_Quadratic Splines");
    pslabel[17].text_is_1byte = true;
    pslabel[17].text_in_resource = true;
    psgcd[17].gd.label = &pslabel[17];
    psgcd[17].gd.flags = sf->order2 ? (gg_visible | gg_enabled | gg_cb_on | gg_utf8_popup) : (gg_visible | gg_enabled | gg_utf8_popup);
    psgcd[17].gd.cid = CID_IsOrder2;
    psgcd[17].creator = GCheckBoxCreate;
    psgcd[17].gd.popup_msg = (unichar_t *) _("Use quadratic (that is truetype) splines to hold the outlines of this\nfont rather than cubic (postscript) splines. Set this option if you\nare editing truetype font. Unset it if you are editing an opentype\nor postscript font (FontForge will convert to the appropriate\nspline type when it generates fonts so this is not required).");

#ifdef FONTFORGE_CONFIG_TYPE3
    psgcd[18].gd.pos.x = 12; psgcd[18].gd.pos.y = psgcd[17].gd.pos.y+18;
    pslabel[18].text = (unichar_t *) _("_Outline Font");
    pslabel[18].text_is_1byte = true;
    pslabel[18].text_in_resource = true;
    psgcd[18].gd.label = &pslabel[18];
    psgcd[18].gd.flags = (!sf->strokedfont && !sf->multilayer)?
	    (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    psgcd[18].creator = GRadioCreate;

    psgcd[19].gd.pos.x = 12; psgcd[19].gd.pos.y = psgcd[18].gd.pos.y+14;
    pslabel[19].text = (unichar_t *) _("_Multi Layered Font");
    pslabel[19].text_is_1byte = true;
    pslabel[19].text_in_resource = true;
    psgcd[19].gd.label = &pslabel[19];
    psgcd[19].gd.flags = sf->multilayer ? (gg_visible | gg_enabled | gg_utf8_popup | gg_cb_on) : (gg_visible | gg_enabled | gg_utf8_popup);
    psgcd[19].gd.cid = CID_IsMultiLayer;
    psgcd[19].creator = GRadioCreate;
    psgcd[19].gd.popup_msg = (unichar_t *) _("Allow editing of multiple colors and shades, fills and strokes.\nMulti layered fonts can only be output as type3 or svg fonts.");

    psgcd[20].gd.pos.x = 12; psgcd[20].gd.pos.y = psgcd[19].gd.pos.y+14;
    pslabel[20].text = (unichar_t *) _("_Stroked Font");
    pslabel[20].text_is_1byte = true;
    pslabel[20].text_in_resource = true;
    psgcd[20].gd.label = &pslabel[20];
    psgcd[20].gd.flags = sf->strokedfont ? (gg_visible | gg_enabled | gg_utf8_popup | gg_cb_on) : (gg_visible | gg_enabled | gg_utf8_popup);
    psgcd[20].gd.cid = CID_IsStrokedFont;
    psgcd[20].creator = GRadioCreate;
    psgcd[20].gd.popup_msg = (unichar_t *) _("Glyphs will be composed of stroked lines rather than filled outlines.\nAll glyphs are stroked at the following width");

    k=21;
#else
    psgcd[18].gd.pos.x = 12; psgcd[18].gd.pos.y = psgcd[17].gd.pos.y+16;
    pslabel[18].text = (unichar_t *) _("_Stroked Font");
    pslabel[18].text_is_1byte = true;
    pslabel[18].text_in_resource = true;
    psgcd[18].gd.label = &pslabel[18];
    psgcd[18].gd.flags = sf->strokedfont ? (gg_visible | gg_enabled | gg_utf8_popup | gg_cb_on) : (gg_visible | gg_enabled | gg_utf8_popup);
    psgcd[18].gd.cid = CID_IsStrokedFont;
    psgcd[18].creator = GCheckBoxCreate;
    psgcd[18].gd.popup_msg = (unichar_t *) _("Glyphs will be composed of stroked lines rather than filled outlines.\nAll glyphs are stroked at the following width");

    k=19;
#endif

    psgcd[k].gd.pos.x = 12; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y+20;
    pslabel[k].text = (unichar_t *) _("Stroke _Width:");
    pslabel[k].text_is_1byte = true;
    pslabel[k].text_in_resource = true;
    psgcd[k].gd.label = &pslabel[k];
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    psgcd[k++].creator = GLabelCreate;

    sprintf( swbuf,"%g", sf->strokewidth );
    psgcd[k].gd.pos.x = 115; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y-6; psgcd[k].gd.pos.width = 137;
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    pslabel[k].text = (unichar_t *) swbuf;
    pslabel[k].text_is_1byte = true;
    psgcd[k].gd.label = &pslabel[k];
    psgcd[k].gd.cid = CID_StrokeWidth;
    psgcd[k++].creator = GTextFieldCreate;

    psgcd[k].gd.pos.x = psgcd[k-2].gd.pos.x; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y+32;
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    pslabel[k].text = (unichar_t *) _("Interpretation:");
    pslabel[k].text_is_1byte = true;
    pslabel[k].text_in_resource = true;
    psgcd[k].gd.label = &pslabel[k];
    psgcd[k++].creator = GLabelCreate;

    psgcd[k].gd.pos.x = psgcd[k-2].gd.pos.x; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y-6;
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    psgcd[k].gd.u.list = interpretations;
    psgcd[k].gd.cid = CID_Interpretation;
    psgcd[k].creator = GListButtonCreate;
    for ( i=0; interpretations[i].text!=NULL || interpretations[i].line; ++i ) {
	if ( (void *) (sf->uni_interp)==interpretations[i].userdata &&
		interpretations[i].text!=NULL ) {
	    interpretations[i].selected = true;
	    psgcd[k].gd.label = &interpretations[i];
	} else
	    interpretations[i].selected = false;
    }
    ++k;

    psgcd[k].gd.pos.x = psgcd[k-2].gd.pos.x; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y+32;
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    pslabel[k].text = (unichar_t *) _("Name List:");
    pslabel[k].text_is_1byte = true;
    pslabel[k].text_in_resource = true;
    psgcd[k].gd.label = &pslabel[k];
    psgcd[k++].creator = GLabelCreate;

    psgcd[k].gd.pos.x = psgcd[k-2].gd.pos.x; psgcd[k].gd.pos.y = psgcd[k-1].gd.pos.y-6;
    psgcd[k].gd.flags = gg_visible | gg_enabled;
    psgcd[k].gd.cid = CID_Namelist;
    psgcd[k].creator = GListButtonCreate;
    nlnames = AllNamelistNames();
    for ( i=0; nlnames[i]!=NULL; ++i);
    namelistnames = gcalloc(i+1,sizeof(GTextInfo));
    for ( i=0; nlnames[i]!=NULL; ++i) {
	namelistnames[i].text = (unichar_t *) nlnames[i];
	namelistnames[i].text_is_1byte = true;
	if ( strcmp(_(sf->for_new_glyphs->title),nlnames[i])==0 ) {
	    namelistnames[i].selected = true;
	    psgcd[k].gd.label = &namelistnames[i];
	}
    }
    psgcd[k].gd.u.list = namelistnames;
    free(nlnames);
    ++k;


    if ( sf->subfontcnt!=0 ) {
	for ( i=0; i<=13; ++i )
	    psgcd[i].gd.flags &= ~gg_enabled;
    } else if ( sf->cidmaster!=NULL ) {
	for ( i=14; i<=16; ++i )
	    psgcd[i].gd.flags &= ~gg_enabled;
    }
/******************************************************************************/

    memset(&plabel,0,sizeof(plabel));
    memset(&pgcd,0,sizeof(pgcd));

    pgcd[0].gd.pos.x = 10; pgcd[0].gd.pos.y = 6;
    pgcd[0].gd.pos.width = 240; pgcd[0].gd.pos.height = 8*12+10;
    pgcd[0].gd.flags = gg_visible | gg_enabled;
    pgcd[0].gd.cid = CID_PrivateEntries;
    pgcd[0].gd.u.list = PI_ListSet(sf);
    pgcd[0].gd.handle_controlevent = PI_ListSel;
    pgcd[0].creator = GListCreate;

    pgcd[1].gd.pos.x = 10; pgcd[1].gd.pos.y = pgcd[0].gd.pos.y+pgcd[0].gd.pos.height+10;
    pgcd[1].gd.pos.width = pgcd[0].gd.pos.width; pgcd[1].gd.pos.height = 8*12+10;
    pgcd[1].gd.flags = gg_visible | gg_enabled;
    pgcd[1].gd.cid = CID_PrivateValues;
    pgcd[1].creator = GTextAreaCreate;

    pgcd[2].gd.pos.x = 10; pgcd[2].gd.pos.y = 300-35-30;
    pgcd[2].gd.pos.width = -1; pgcd[2].gd.pos.height = 0;
    pgcd[2].gd.flags = gg_visible | gg_enabled ;
    plabel[2].text = (unichar_t *) _("_Add");
    plabel[2].text_is_1byte = true;
    plabel[2].text_in_resource = true;
    pgcd[2].gd.label = &plabel[2];
    pgcd[2].gd.handle_controlevent = PI_Add;
    pgcd[2].gd.cid = CID_Add;
    pgcd[2].creator = GButtonCreate;

    pgcd[3].gd.pos.x = (260)/2-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)-5;
    pgcd[3].gd.pos.y = pgcd[2].gd.pos.y;
    pgcd[3].gd.pos.width = -1; pgcd[3].gd.pos.height = 0;
    pgcd[3].gd.flags = gg_visible ;
    plabel[3].text = (unichar_t *) _("_Guess");
    plabel[3].text_is_1byte = true;
    plabel[3].text_in_resource = true;
    pgcd[3].gd.label = &plabel[3];
    pgcd[3].gd.handle_controlevent = PI_Guess;
    pgcd[3].gd.cid = CID_Guess;
    pgcd[3].creator = GButtonCreate;

    pgcd[4].gd.pos.x = -(260/2-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)-5);
    pgcd[4].gd.pos.y = pgcd[2].gd.pos.y;
    pgcd[4].gd.pos.width = -1; pgcd[4].gd.pos.height = 0;
    pgcd[4].gd.flags = gg_visible | gg_utf8_popup ;
    plabel[4].text = (unichar_t *) _("_Hist");
    plabel[4].text_is_1byte = true;
    plabel[4].text_in_resource = true;
    pgcd[4].gd.label = &plabel[4];
    pgcd[4].gd.handle_controlevent = PI_Hist;
    pgcd[4].gd.cid = CID_Hist;
    pgcd[4].gd.popup_msg = (unichar_t *) _("Histogram Dialog");
    pgcd[4].creator = GButtonCreate;

    pgcd[5].gd.pos.x = -10; pgcd[5].gd.pos.y = pgcd[2].gd.pos.y;
    pgcd[5].gd.pos.width = -1; pgcd[5].gd.pos.height = 0;
    pgcd[5].gd.flags = gg_visible | gg_enabled ;
    plabel[5].text = (unichar_t *) _("Re_move");
    plabel[5].text_is_1byte = true;
    plabel[5].text_in_resource = true;
    pgcd[5].gd.label = &plabel[5];
    pgcd[5].gd.handle_controlevent = PI_Delete;
    pgcd[5].gd.cid = CID_Remove;
    pgcd[5].creator = GButtonCreate;
/******************************************************************************/
    memset(&vlabel,0,sizeof(vlabel));
    memset(&vgcd,0,sizeof(vgcd));

    vgcd[0].gd.pos.x = 10; vgcd[0].gd.pos.y = 12;
    vlabel[0].text = (unichar_t *) _("_Weight Class");
    vlabel[0].text_is_1byte = true;
    vlabel[0].text_in_resource = true;
    vgcd[0].gd.label = &vlabel[0];
    vgcd[0].gd.flags = gg_visible | gg_enabled;
    vgcd[0].creator = GLabelCreate;

    vgcd[1].gd.pos.x = 90; vgcd[1].gd.pos.y = vgcd[0].gd.pos.y-6; vgcd[1].gd.pos.width = 140;
    vgcd[1].gd.flags = gg_visible | gg_enabled;
    vgcd[1].gd.cid = CID_WeightClass;
    vgcd[1].gd.u.list = weightclass;
    vgcd[1].creator = GListFieldCreate;

    vgcd[2].gd.pos.x = 10; vgcd[2].gd.pos.y = vgcd[1].gd.pos.y+26+6;
    vlabel[2].text = (unichar_t *) _("Width _Class");
    vlabel[2].text_is_1byte = true;
    vlabel[2].text_in_resource = true;
    vgcd[2].gd.label = &vlabel[2];
    vgcd[2].gd.flags = gg_visible | gg_enabled;
    vgcd[2].creator = GLabelCreate;

    vgcd[3].gd.pos.x = 90; vgcd[3].gd.pos.y = vgcd[2].gd.pos.y-6;
    vgcd[3].gd.flags = gg_visible | gg_enabled;
    vgcd[3].gd.cid = CID_WidthClass;
    vgcd[3].gd.u.list = widthclass;
    vgcd[3].creator = GListButtonCreate;

    vgcd[4].gd.pos.x = 10; vgcd[4].gd.pos.y = vgcd[3].gd.pos.y+26+6;
    vlabel[4].text = (unichar_t *) _("P_FM Family");
    vlabel[4].text_is_1byte = true;
    vlabel[4].text_in_resource = true;
    vgcd[4].gd.label = &vlabel[4];
    vgcd[4].gd.flags = gg_visible | gg_enabled;
    vgcd[4].creator = GLabelCreate;

    vgcd[5].gd.pos.x = 90; vgcd[5].gd.pos.y = vgcd[4].gd.pos.y-6; vgcd[5].gd.pos.width = 140;
    vgcd[5].gd.flags = gg_visible | gg_enabled;
    vgcd[5].gd.cid = CID_PFMFamily;
    vgcd[5].gd.u.list = pfmfamily;
    vgcd[5].creator = GListButtonCreate;

    vgcd[6].gd.pos.x = 10; vgcd[6].gd.pos.y = vgcd[5].gd.pos.y+26+6;
    vlabel[6].text = (unichar_t *) _("_Embeddable");
    vlabel[6].text_is_1byte = true;
    vlabel[6].text_in_resource = true;
    vgcd[6].gd.label = &vlabel[6];
    vgcd[6].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    vgcd[6].gd.popup_msg = (unichar_t *) _("Can this font be embedded in a downloadable (pdf)\ndocument, and if so, what behaviors are permitted on\nboth the document and the font.");
    vgcd[6].creator = GLabelCreate;

    vgcd[7].gd.pos.x = 90; vgcd[7].gd.pos.y = vgcd[6].gd.pos.y-6;
    vgcd[7].gd.pos.width = 140;
    vgcd[7].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    vgcd[7].gd.cid = CID_FSType;
    vgcd[7].gd.u.list = fstype;
    vgcd[7].gd.popup_msg = vgcd[6].gd.popup_msg;
    vgcd[7].creator = GListButtonCreate;
    fstype[0].selected = fstype[1].selected =
	    fstype[2].selected = fstype[3].selected = false;
    if ( sf->pfminfo.fstype&0x8 /* also catches the "not set" case == -1 */ )
	i = 2;
    else if ( sf->pfminfo.fstype&0x4 )
	i = 1;
    else if ( sf->pfminfo.fstype&0x2 )
	i = 0;
    else
	i = 3;
    fstype[i].selected = true;
    vgcd[7].gd.label = &fstype[i];

    vgcd[8].gd.pos.x = 20; vgcd[8].gd.pos.y = vgcd[7].gd.pos.y+26;
    vlabel[8].text = (unichar_t *) _("No Subsetting");
    vlabel[8].text_is_1byte = true;
    vgcd[8].gd.label = &vlabel[8];
    vgcd[8].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    if ( sf->pfminfo.fstype!=-1 && (sf->pfminfo.fstype&0x100) )
	vgcd[8].gd.flags |= gg_cb_on;
    vgcd[8].gd.popup_msg = (unichar_t *) _("If set then the entire font must be\nembedded in a document when any character is.\nOtherwise the document creator need\nonly include the characters it uses.");
    vgcd[8].gd.cid = CID_NoSubsetting;
    vgcd[8].creator = GCheckBoxCreate;

    vgcd[9].gd.pos.x = 110; vgcd[9].gd.pos.y = vgcd[8].gd.pos.y;
    vlabel[9].text = (unichar_t *) _("Only Embed Bitmaps");
    vlabel[9].text_is_1byte = true;
    vgcd[9].gd.label = &vlabel[9];
    vgcd[9].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    if ( sf->pfminfo.fstype!=-1 && ( sf->pfminfo.fstype&0x200 ))
	vgcd[9].gd.flags |= gg_cb_on;
    vgcd[9].gd.popup_msg = (unichar_t *) _("Only Bitmaps may be embedded.\nOutline descriptions may not be\n(if font file contains no bitmaps\nthen nothing may be embedded).");
    vgcd[9].gd.cid = CID_OnlyBitmaps;
    vgcd[9].creator = GCheckBoxCreate;

    vgcd[10].gd.pos.x = 10; vgcd[10].gd.pos.y = vgcd[9].gd.pos.y+24;
    vlabel[10].text = (unichar_t *) _("Vendor ID:");
    vlabel[10].text_is_1byte = true;
    vgcd[10].gd.label = &vlabel[10];
    vgcd[10].gd.flags = gg_visible | gg_enabled;
    vgcd[10].creator = GLabelCreate;

    vgcd[11].gd.pos.x = 90; vgcd[11].gd.pos.y = vgcd[11-1].gd.pos.y-4;
    vgcd[11].gd.pos.width = 50;
    vgcd[11].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    vgcd[11].gd.cid = CID_Vendor;
    vgcd[11].creator = GTextFieldCreate;

    vgcd[12].gd.pos.x = 10; vgcd[12].gd.pos.y = vgcd[11].gd.pos.y+24+6;
    vlabel[12].text = (unichar_t *) _("_IBM Family:");
    vlabel[12].text_is_1byte = true;
    vlabel[12].text_in_resource = true;
    vgcd[12].gd.label = &vlabel[12];
    vgcd[12].gd.flags = gg_visible | gg_enabled;
    vgcd[12].creator = GLabelCreate;

    vgcd[13].gd.pos.x = 90; vgcd[13].gd.pos.y = vgcd[12].gd.pos.y-4; vgcd[13].gd.pos.width = vgcd[7].gd.pos.width;
    vgcd[13].gd.flags = gg_visible | gg_enabled;
    vgcd[13].gd.cid = CID_IBMFamily;
    vgcd[13].gd.u.list = ibmfamily;
    vgcd[13].creator = GListButtonCreate;

    vgcd[14].gd.pos.x = 10; vgcd[14].gd.pos.y = vgcd[13].gd.pos.y+56;
    vlabel[14].text = (unichar_t *) _("_Set GSUB/morx Ordering");
    vlabel[14].text_is_1byte = true;
    vlabel[14].text_in_resource = true;
    vgcd[14].gd.label = &vlabel[14];
    vgcd[14].gd.flags = gg_visible | gg_enabled;
    vgcd[14].gd.handle_controlevent = OrderGSUB;
    vgcd[14].creator = GButtonCreate;


/******************************************************************************/

    memset(&metgcd,0,sizeof(metgcd));
    memset(&metlabel,'\0',sizeof(metlabel));

    i = 0;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = 9;
    metlabel[i].text = (unichar_t *) _("Win _Ascent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("Anything outside the OS/2 WinAscent &\nWinDescent fields will be clipped by windows.\nThis includes marks, etc. that have been repositioned by GPOS.\n(The descent field is usually positive.)\nIf the \"[] Is Offset\" checkbox is clear then\nany number you enter will be the value used in OS/2.\nIf set then any number you enter will be added to the\nfont's bounds. You should leave this\nfield 0 and check \"[*] Is Offset\" in most cases.");
    metgcd[i].gd.cid = CID_WinAscentLab;
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_WinAscent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_WinAscentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metgcd[i++].creator = GCheckBoxCreate;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("Win _Descent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.cid = CID_WinDescentLab;
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_WinDescent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_WinDescentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metgcd[i++].creator = GCheckBoxCreate;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("_Typo Ascent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("The type ascent&descent fields are>supposed<\nto specify the line spacing on windows.\nIn fact usually the win ascent/descent fields do.\n(The descent field is usually negative.)\nIf the \"[] Is Offset\" checkbox is clear then\nany number you enter will be the value used in OS/2.\nIf set then any number you enter will be added to the\nEm-size. You should leave this\nfield 0 and check \"[*] Is Offset\" in most cases.");
    metgcd[i].gd.cid = CID_TypoAscentLab;
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_TypoAscent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_TypoAscentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metgcd[i++].creator = GCheckBoxCreate;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("T_ypo Descent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.cid = CID_TypoDescentLab;
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_TypoDescent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_TypoDescentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metgcd[i++].creator = GCheckBoxCreate;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("Typo Line _Gap:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("Sets the TypoLinegap field in the OS/2 table, used on MS Windows");
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* Line gap value set later */
    metgcd[i].gd.cid = CID_TypoLineGap;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("_HHead Ascent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("This specifies the line spacing on the mac.\n(The descent field is usually negative.)\nIf the \"[] Is Offset\" checkbox is clear then\nany number you enter will be the value used in hhea.\nIf set then any number you enter will be added to the\nfont's bounds. You should leave this\nfield 0 and check \"[*] Is Offset\" in most cases.");
    metgcd[i].gd.cid = CID_HHeadAscentLab;
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_HHeadAscent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_HHeadAscentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metgcd[i++].creator = GCheckBoxCreate;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("HHead De_scent Offset:");
    metlabel[i].text_in_resource = true;
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.cid = CID_HHeadDescentLab;
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_HHeadDescent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 178; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Is Offset");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_HHeadDescentIsOff;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.handle_controlevent = GFI_AsDesIsOff;
    metgcd[i++].creator = GCheckBoxCreate;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("HHead _Line Gap:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("Sets the linegap field in the hhea table, used on the mac");
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* Line gap value set later */
    metgcd[i].gd.cid = CID_LineGap;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i++].creator = GTextFieldCreate;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y+26+6;
    metlabel[i].text = (unichar_t *) _("VHead _Column Spacing:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled | gg_utf8_popup) : (gg_visible | gg_utf8_popup);
    metgcd[i].gd.popup_msg = (unichar_t *) _("Sets the linegap field in the vhea table.\nThis is the horizontal spacing between rows\nof vertically set text.");
    metgcd[i].gd.cid = CID_VLineGapLab;
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-6;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled | gg_utf8_popup) : (gg_visible | gg_utf8_popup);
	/* V Line gap value set later */
    metgcd[i].gd.cid = CID_VLineGap;
    metgcd[i].gd.popup_msg = metgcd[17].gd.popup_msg;
    metgcd[i++].creator = GTextFieldCreate;

/******************************************************************************/

    memset(&ssgcd,0,sizeof(ssgcd));
    memset(&sslabel,'\0',sizeof(sslabel));

    i = 0;

    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = 5;
    sslabel[i].text = (unichar_t *) _("Default");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    ssgcd[i].gd.cid = CID_SubSuperDefault;
    ssgcd[i].gd.handle_controlevent = GFI_SubSuperDefault;
    ssgcd[i++].creator = GCheckBoxCreate;

    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+16;
    sslabel[i].text = (unichar_t *) _("Subscript");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 120; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-4;
/* GT: X is a coordinate */
    sslabel[i].text = (unichar_t *) _("X");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 180; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
/* GT: Y is a coordinate */
    sslabel[i].text = (unichar_t *) _("Y");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-3].gd.pos.y+14+4;
    sslabel[i].text = (unichar_t *) _("Size");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubXSize;
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubYSize;
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-3].gd.pos.y+26;
    sslabel[i].text = (unichar_t *) _("Offset");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubXOffset;
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubYOffset;
    ssgcd[i++].creator = GTextFieldCreate;


    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+30;
    sslabel[i].text = (unichar_t *) _("Superscript");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+14+4;
    sslabel[i].text = (unichar_t *) _("Size");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperXSize;
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperYSize;
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-3].gd.pos.y+26;
    sslabel[i].text = (unichar_t *) _("Offset");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperXOffset;
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperYOffset;
    ssgcd[i++].creator = GTextFieldCreate;


    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+30;
    sslabel[i].text = (unichar_t *) _("Strikeout");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+14+4;
    sslabel[i].text = (unichar_t *) _("Size");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_StrikeoutSize;
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-2].gd.pos.y+26;
    sslabel[i].text = (unichar_t *) _("Pos");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_StrikeoutPos;
    ssgcd[i++].creator = GTextFieldCreate;
    
/******************************************************************************/
    memset(&panlabel,0,sizeof(panlabel));
    memset(&pangcd,0,sizeof(pangcd));

    i = 0;

    pangcd[i].gd.pos.x = 5; pangcd[i].gd.pos.y = 5;
    panlabel[i].text = (unichar_t *) _("Default");
    panlabel[i].text_is_1byte = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    pangcd[i].gd.cid = CID_PanDefault;
    /*pangcd[i].gd.popup_msg = pangcd[i-1].gd.popup_msg;*/
    pangcd[i].gd.handle_controlevent = GFI_PanoseDefault;
    pangcd[i++].creator = GCheckBoxCreate;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+14+4;
    panlabel[i].text = (unichar_t *) _("_Family");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanFamilyLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanFamily;
    pangcd[i].gd.u.list = panfamily;
    pangcd[i++].creator = GListButtonCreate;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Serifs");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanSerifsLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanSerifs;
    pangcd[i].gd.u.list = panserifs;
    pangcd[i++].creator = GListButtonCreate;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Weight");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanWeightLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanWeight;
    pangcd[i].gd.u.list = panweight;
    pangcd[i++].creator = GListButtonCreate;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Proportion");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanPropLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanProp;
    pangcd[i].gd.u.list = panprop;
    pangcd[i++].creator = GListButtonCreate;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Contrast");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanContrastLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanContrast;
    pangcd[i].gd.u.list = pancontrast;
    pangcd[i++].creator = GListButtonCreate;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("Stroke _Variation");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanStrokeVarLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanStrokeVar;
    pangcd[i].gd.u.list = panstrokevar;
    pangcd[i++].creator = GListButtonCreate;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Arm Style");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanArmStyleLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanArmStyle;
    pangcd[i].gd.u.list = panarmstyle;
    pangcd[i++].creator = GListButtonCreate;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Letterform");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanLetterformLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanLetterform;
    pangcd[i].gd.u.list = panletterform;
    pangcd[i++].creator = GListButtonCreate;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Midline");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanMidLineLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanMidLine;
    pangcd[i].gd.u.list = panmidline;
    pangcd[i++].creator = GListButtonCreate;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_X-Height");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanXHeightLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanXHeight;
    pangcd[i].gd.u.list = panxheight;
    pangcd[i++].creator = GListButtonCreate;

/******************************************************************************/

    memset(&vagcd,0,sizeof(vagcd));
    memset(&vaspects,'\0',sizeof(vaspects));

    i = 0;
    vaspects[i].text = (unichar_t *) _("Misc.");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = vgcd;

    vaspects[i].text = (unichar_t *) _("Metrics");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = metgcd;

    vaspects[i].text = (unichar_t *) _("Sub/Super");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = ssgcd;

    vaspects[i].text = (unichar_t *) _("Panose");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = pangcd;

    vagcd[0].gd.pos.x = 4; vagcd[0].gd.pos.y = 6;
    vagcd[0].gd.pos.width = 252;
    vagcd[0].gd.pos.height = 300;
    vagcd[0].gd.u.tabs = vaspects;
    vagcd[0].gd.flags = gg_visible | gg_enabled;
    /*vagcd[0].gd.handle_controlevent = GFI_TTFAspectChange;*/
    vagcd[0].gd.cid = CID_TTFTabs;
    vagcd[0].creator = GTabSetCreate;

/******************************************************************************/
    memset(&tnlabel,0,sizeof(tnlabel));
    memset(&tngcd,0,sizeof(tngcd));

    tngcd[0].gd.pos.x = 5; tngcd[0].gd.pos.y = 6;
    tngcd[0].gd.flags = gg_visible | gg_enabled;
    tnlabel[0].text = (unichar_t *) _("Sort By:");
    tnlabel[0].text_is_1byte = true;
    tngcd[0].gd.label = &tnlabel[0];
    tngcd[0].creator = GLabelCreate;

    tngcd[1].gd.pos.x = 50; tngcd[1].gd.pos.y = tngcd[0].gd.pos.y-4;
    tngcd[1].gd.flags = gg_enabled | gg_visible;
    tngcd[1].gd.cid = CID_TNLangSort;
    tnlabel[1].text = (unichar_t *) _("_Language");
    tnlabel[1].text_is_1byte = true;
    tnlabel[1].text_in_resource = true;
    tngcd[1].gd.label = &tnlabel[1];
    tngcd[1].creator = GRadioCreate;
    tngcd[1].gd.handle_controlevent = GFI_SortBy;

    tngcd[2].gd.pos.x = 120; tngcd[2].gd.pos.y = tngcd[1].gd.pos.y;
    tngcd[2].gd.flags = gg_visible | gg_enabled;
    tngcd[2].gd.cid = CID_TNStringSort;
    tnlabel[2].text = (unichar_t *) _("_String Type");
    tnlabel[2].text_is_1byte = true;
    tnlabel[2].text_in_resource = true;
    tngcd[2].gd.label = &tnlabel[2];
    tngcd[2].creator = GRadioCreate;
    tngcd[2].gd.handle_controlevent = GFI_SortBy;

    tngcd[3].gd.pos.x = 195; tngcd[3].gd.pos.y = tngcd[1].gd.pos.y;
    tngcd[3].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    tnlabel[3].text = (unichar_t *) _("Default");
    tnlabel[3].text_is_1byte = true;
    tngcd[3].gd.label = &tnlabel[3];
    tngcd[3].creator = GRadioCreate;
    tngcd[3].gd.handle_controlevent = GFI_SortBy;

    sbwidth = GDrawPointsToPixels(d->gw,_GScrollBar_Width);
    tngcd[4].gd.pos.x = 114; tngcd[4].gd.pos.y = tngcd[1].gd.pos.y+14;
    tngcd[4].gd.pos.width = sbwidth; tngcd[4].gd.pos.height = 100;
    tngcd[4].gd.flags = gg_enabled | gg_visible | gg_sb_vert;
    tngcd[4].gd.cid = CID_TNVScroll;
    tngcd[4].gd.handle_controlevent = _TN_VScroll;
    tngcd[4].creator = GScrollBarCreate;

    tngcd[5].gd.pos.x = 14; tngcd[5].gd.pos.y = tngcd[4].gd.pos.y+100;
    tngcd[5].gd.pos.width = 100; tngcd[5].gd.pos.height = sbwidth;
    tngcd[5].gd.flags = gg_visible | gg_enabled;
    tngcd[5].gd.cid = CID_TNHScroll;
    tngcd[5].gd.handle_controlevent = _TN_HScroll;
    tngcd[5].creator = GScrollBarCreate;
/******************************************************************************/
    memset(&comlabel,0,sizeof(comlabel));
    memset(&comgcd,0,sizeof(comgcd));

    comgcd[0].gd.pos.x = 10; comgcd[0].gd.pos.y = 10;
    comgcd[0].gd.pos.width = ngcd[11].gd.pos.width; comgcd[0].gd.pos.height = 230;
    comgcd[0].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    comgcd[0].gd.cid = CID_Comment;
    comlabel[0].text = (unichar_t *) sf->comments;
    comlabel[0].text_is_1byte = true;
    if ( comlabel[0].text==NULL ) comlabel[0].text = (unichar_t *) "";
    comgcd[0].gd.label = &comlabel[0];
    comgcd[0].creator = GTextAreaCreate;
/******************************************************************************/
    memset(&atlabel,0,sizeof(atlabel));
    memset(&atgcd,0,sizeof(atgcd));

    atgcd[0].gd.pos.x = 10; atgcd[0].gd.pos.y = 10;
    atgcd[0].gd.pos.width = ngcd[11].gd.pos.width; atgcd[0].gd.pos.height = 200;
    atgcd[0].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic;
    atgcd[0].gd.cid = CID_AnchorClasses;
    atgcd[0].gd.u.list = AnchorClassesList(sf);
    atgcd[0].gd.handle_controlevent = GFI_AnchorSelChanged;
    atgcd[0].creator = GListCreate;

    atgcd[1].gd.pos.x = 10; atgcd[1].gd.pos.y = atgcd[0].gd.pos.y+atgcd[0].gd.pos.height+4;
    atgcd[1].gd.pos.width = -1;
    atgcd[1].gd.flags = gg_visible | gg_enabled;
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "_New...", ignore "Anchor|" */
    atlabel[1].text = (unichar_t *) S_("Anchor|_New...");
    atlabel[1].text_is_1byte = true;
    atlabel[1].text_in_resource = true;
    atgcd[1].gd.label = &atlabel[1];
    atgcd[1].gd.cid = CID_AnchorNew;
    atgcd[1].gd.handle_controlevent = GFI_AnchorNew;
    atgcd[1].creator = GButtonCreate;

    atgcd[2].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor); atgcd[2].gd.pos.y = atgcd[1].gd.pos.y;
    atgcd[2].gd.pos.width = -1;
    atgcd[2].gd.flags = gg_visible;
    atlabel[2].text = (unichar_t *) _("_Delete");
    atlabel[2].text_is_1byte = true;
    atlabel[2].text_in_resource = true;
    atgcd[2].gd.label = &atlabel[2];
    atgcd[2].gd.cid = CID_AnchorDel;
    atgcd[2].gd.handle_controlevent = GFI_AnchorDel;
    atgcd[2].creator = GButtonCreate;

    atgcd[3].gd.pos.x = -10; atgcd[3].gd.pos.y = atgcd[1].gd.pos.y;
    atgcd[3].gd.pos.width = -1;
    atgcd[3].gd.flags = gg_visible;
    atlabel[3].text = (unichar_t *) _("_Edit...");
    atlabel[3].text_is_1byte = true;
    atlabel[3].text_in_resource = true;
    atgcd[3].gd.label = &atlabel[3];
    atgcd[3].gd.cid = CID_AnchorRename;
    atgcd[3].gd.handle_controlevent = GFI_AnchorRename;
    atgcd[3].creator = GButtonCreate;

    atgcd[4].gd.pos.x = 10; atgcd[4].gd.pos.y = atgcd[1].gd.pos.y+30;
    atgcd[4].gd.flags = gg_visible;
    atlabel[4].text = (unichar_t *) _("Show First Mark");
    atlabel[4].text_is_1byte = true;
    atgcd[4].gd.label = &atlabel[4];
    atgcd[4].gd.cid = CID_ShowMark;
    atgcd[4].gd.handle_controlevent = GFI_AnchorShowMark;
    atgcd[4].creator = GButtonCreate;

    atgcd[5].gd.pos.x = -10; atgcd[5].gd.pos.y = atgcd[4].gd.pos.y;
    atgcd[5].gd.flags = gg_visible;
    atlabel[5].text = (unichar_t *) _("Show First Base");
    atlabel[5].text_is_1byte = true;
    atgcd[5].gd.label = &atlabel[5];
    atgcd[5].gd.cid = CID_ShowBase;
    atgcd[5].gd.handle_controlevent = GFI_AnchorShowBase;
    atgcd[5].creator = GButtonCreate;

/******************************************************************************/
    memset(&mklabel,0,sizeof(mklabel));
    memset(&mkgcd,0,sizeof(mkgcd));

    mkgcd[0].gd.pos.x = 10; mkgcd[0].gd.pos.y = 10;
    mkgcd[0].gd.pos.width = ngcd[11].gd.pos.width; mkgcd[0].gd.pos.height = 200;
    mkgcd[0].gd.flags = gg_visible | gg_enabled;
    mkgcd[0].gd.cid = CID_MarkClasses;
    mkgcd[0].gd.u.list = MarkClassesList(sf);
    mkgcd[0].gd.handle_controlevent = GFI_MarkSelChanged;
    mkgcd[0].creator = GListCreate;

    mkgcd[1].gd.pos.x = 10; mkgcd[1].gd.pos.y = mkgcd[0].gd.pos.y+mkgcd[0].gd.pos.height+4;
    mkgcd[1].gd.pos.width = -1;
    mkgcd[1].gd.flags = gg_visible | gg_enabled;
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "_New...", ignore "Mark|" */
    mklabel[1].text = (unichar_t *) S_("Mark|_New...");
    mklabel[1].text_is_1byte = true;
    mklabel[1].text_in_resource = true;
    mkgcd[1].gd.label = &mklabel[1];
    mkgcd[1].gd.cid = CID_MarkNew;
    mkgcd[1].gd.handle_controlevent = GFI_MarkNew;
    mkgcd[1].creator = GButtonCreate;

    mkgcd[2].gd.pos.x = -10; mkgcd[2].gd.pos.y = mkgcd[1].gd.pos.y;
    mkgcd[2].gd.pos.width = -1;
    mkgcd[2].gd.flags = gg_visible;
    mklabel[2].text = (unichar_t *) _("_Edit...");
    mklabel[2].text_is_1byte = true;
    mklabel[2].text_in_resource = true;
    mkgcd[2].gd.label = &mklabel[2];
    mkgcd[2].gd.cid = CID_MarkEdit;
    mkgcd[2].gd.handle_controlevent = GFI_MarkEdit;
    mkgcd[2].creator = GButtonCreate;

/******************************************************************************/
    memset(&txlabel,0,sizeof(txlabel));
    memset(&txgcd,0,sizeof(txgcd));

    k=0;
    txlabel[k].text = (unichar_t *) U_("ΤεΧ Text");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = 10;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.cid = CID_TeXText;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    txlabel[k].text = (unichar_t *) U_("ΤεΧ Math");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 80; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.cid = CID_TeXMath;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    txlabel[k].text = (unichar_t *) U_("ΤεΧ Math Ext");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 155; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.cid = CID_TeXMathExt;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    for ( i=0; texparams[i]!=0; ++i ) {
	txlabel[k].text = (unichar_t *) texparams[i];
	txlabel[k].text_is_1byte = true;
	txgcd[k].gd.label = &txlabel[k];
	txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = txgcd[k-2].gd.pos.y+26;
	txgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	txgcd[k].gd.popup_msg = (unichar_t *) texpopups[i];
	txgcd[k++].creator = GLabelCreate;

	txgcd[k].gd.pos.x = 70; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y-4;
	txgcd[k].gd.flags = gg_visible | gg_enabled;
	txgcd[k].gd.cid = CID_TeX + i;
	txgcd[k++].creator = GTextFieldCreate;
    }
    txgcd[k-2].gd.cid = CID_TeXExtraSpLabel;

/* GT: More Parameters */
    txlabel[k].text = (unichar_t *) _("More Params");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 20; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y+26;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.handle_controlevent = GFI_MoreParams;
    txgcd[k].gd.cid = CID_MoreParams;
    txgcd[k++].creator = GButtonCreate;


/******************************************************************************/
    memset(&szlabel,0,sizeof(szlabel));
    memset(&szgcd,0,sizeof(szgcd));

    k=0;

    szlabel[k].text = (unichar_t *) _("De_sign Size:");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 10; szgcd[k].gd.pos.y = 9;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("The size (in points) for which this face was designed");
    szgcd[k++].creator = GLabelCreate;

    sprintf(dszbuf, "%.1f", sf->design_size/10.0);
    szlabel[k].text = (unichar_t *) dszbuf;
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 70; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y-4;
    szgcd[k].gd.pos.width = 60;
    szgcd[k].gd.flags = gg_visible | gg_enabled;
    szgcd[k].gd.cid = CID_DesignSize;
    szgcd[k++].creator = GTextFieldCreate;

    szlabel[k].text = (unichar_t *) _("Points");
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 134; szgcd[k].gd.pos.y = szgcd[k-2].gd.pos.y;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("The size (in points) for which this face was designed");
    szgcd[k++].creator = GLabelCreate;

    szlabel[k].text = (unichar_t *) _("Design Range");
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 14; szgcd[k].gd.pos.y = szgcd[k-2].gd.pos.y+24;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("The range of sizes (in points) to which this face applies.\nLower bound is exclusive, upper bound is inclusive.");
    szgcd[k++].creator = GLabelCreate;

    szgcd[k].gd.pos.x = 8; szgcd[k].gd.pos.y = GDrawPointsToPixels(NULL,szgcd[k-1].gd.pos.y+6);
    szgcd[k].gd.pos.width = pos.width-32; szgcd[k].gd.pos.height = GDrawPointsToPixels(NULL,36);
    szgcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    szgcd[k++].creator = GGroupCreate;

    szlabel[k].text = (unichar_t *) _("_Bottom:");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 14; szgcd[k].gd.pos.y = szgcd[k-2].gd.pos.y+18;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("The range of sizes (in points) to which this face applies.\nLower bound is exclusive, upper bound is inclusive.");
    szgcd[k++].creator = GLabelCreate;

    sprintf(dsbbuf, "%.1f", sf->design_range_bottom/10.0);
    szlabel[k].text = (unichar_t *) dsbbuf;
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 70; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y-4;
    szgcd[k].gd.pos.width = 60;
    szgcd[k].gd.flags = gg_visible | gg_enabled;
    szgcd[k].gd.cid = CID_DesignBottom;
    szgcd[k++].creator = GTextFieldCreate;

    szlabel[k].text = (unichar_t *) _("_Top:");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 140; szgcd[k].gd.pos.y = szgcd[k-2].gd.pos.y;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("The range of sizes (in points) to which this face applies.\nLower bound is exclusive, upper bound is inclusive.");
    szgcd[k++].creator = GLabelCreate;

    sprintf(dstbuf, "%.1f", sf->design_range_top/10.0);
    szlabel[k].text = (unichar_t *) dstbuf;
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 180; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y-4;
    szgcd[k].gd.pos.width = 60;
    szgcd[k].gd.flags = gg_visible | gg_enabled;
    szgcd[k].gd.cid = CID_DesignTop;
    szgcd[k++].creator = GTextFieldCreate;

    szlabel[k].text = (unichar_t *) _("Style _ID:");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 14; szgcd[k].gd.pos.y = GDrawPixelsToPoints(NULL,szgcd[k-5].gd.pos.y+szgcd[k-5].gd.pos.height)+10;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("This is an identifying number shared by all members of\nthis font family with the same style (I.e. 10pt Bold and\n24pt Bold would have the same id, but 10pt Italic would not");
    szgcd[k++].creator = GLabelCreate;

    sprintf(sibuf, "%d", sf->fontstyle_id);
    szlabel[k].text = (unichar_t *) sibuf;
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 70; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y-4;
    szgcd[k].gd.pos.width = 60;
    szgcd[k].gd.flags = gg_visible | gg_enabled;
    szgcd[k].gd.cid = CID_StyleID;
    szgcd[k++].creator = GTextFieldCreate;

    szlabel[k].text = (unichar_t *) _("Style Name:");
    szlabel[k].text_is_1byte = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.pos.x = 14; szgcd[k].gd.pos.y = szgcd[k-2].gd.pos.y+22;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    szgcd[k].gd.popup_msg = (unichar_t *) _("This provides a set of names used to identify the\nstyle of this font. Names may be translated into multiple\nlanguages (English is required, others are optional)\nAll fonts with the same Style ID should share this name.");
    szgcd[k++].creator = GLabelCreate;

    szgcd[k].gd.pos.x = 10; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y+14;
    szgcd[k].gd.pos.width = ngcd[11].gd.pos.width; szgcd[k].gd.pos.height = 100;
    szgcd[k].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic;
    szgcd[k].gd.cid = CID_StyleName;
    szgcd[k].gd.handle_controlevent = GFI_StyleNameSelChanged;
    szgcd[k++].creator = GListCreate;

    szgcd[k].gd.pos.x = 10; szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y+szgcd[k-1].gd.pos.height+4;
    szgcd[k].gd.pos.width = -1;
    szgcd[k].gd.flags = gg_visible | gg_enabled;
    szlabel[k].text = (unichar_t *) _("_New...");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.cid = CID_StyleNameNew;
    szgcd[k].gd.handle_controlevent = GFI_StyleNameNew;
    szgcd[k++].creator = GButtonCreate;

    szgcd[k].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y;
    szgcd[k].gd.pos.width = -1;
    szgcd[k].gd.flags = gg_visible;
    szlabel[k].text = (unichar_t *) _("_Delete");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.cid = CID_StyleNameDel;
    szgcd[k].gd.handle_controlevent = GFI_StyleNameDel;
    szgcd[k++].creator = GButtonCreate;

    szgcd[k].gd.pos.x = 10 + 2*(10+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor));
    szgcd[k].gd.pos.y = szgcd[k-1].gd.pos.y;
    szgcd[k].gd.pos.width = -1;
    szgcd[k].gd.flags = gg_visible;
    szlabel[k].text = (unichar_t *) _("_Edit...");
    szlabel[k].text_is_1byte = true;
    szlabel[k].text_in_resource = true;
    szgcd[k].gd.label = &szlabel[k];
    szgcd[k].gd.cid = CID_StyleNameRename;
    szgcd[k].gd.handle_controlevent = GFI_StyleNameRename;
    szgcd[k++].creator = GButtonCreate;

/******************************************************************************/
    memset(&csublabel,0,sizeof(csublabel));
    memset(&csubgcd,0,sizeof(csubgcd));
    memset(&congcd,0,sizeof(congcd));
    memset(&conaspects,'\0',sizeof(conaspects));

    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	conaspects[i].text = (unichar_t *) connames[i];
	conaspects[i].text_is_1byte = true;
	conaspects[i].gcd = csubgcd[i];

	csubgcd[i][0].gd.pos.x = 10; csubgcd[i][0].gd.pos.y = 10;
	csubgcd[i][0].gd.pos.width = ngcd[11].gd.pos.width;
	csubgcd[i][0].gd.pos.height = 150;
	csubgcd[i][0].gd.flags = gg_visible | gg_enabled;
	csubgcd[i][0].gd.cid = CID_ContextClasses+i*100;
	csubgcd[i][0].gd.u.list = FPSTList(sf,contypes[i]);
	csubgcd[i][0].gd.handle_controlevent = GFI_ContextSelChanged;
	csubgcd[i][0].creator = GListCreate;

	csubgcd[i][1].gd.pos.x = 10; csubgcd[i][1].gd.pos.y = csubgcd[i][0].gd.pos.y+csubgcd[i][0].gd.pos.height+4;
	csubgcd[i][1].gd.pos.width = -1;
	csubgcd[i][1].gd.flags = gg_visible | gg_enabled;
	csublabel[i][1].text = (unichar_t *) _("_New...");
	csublabel[i][1].text_is_1byte = true;
	csublabel[i][1].text_in_resource = true;
	csubgcd[i][1].gd.label = &csublabel[i][1];
	csubgcd[i][1].gd.cid = CID_ContextNew+i*100;
	csubgcd[i][1].gd.handle_controlevent = GFI_ContextNew;
	csubgcd[i][1].creator = GButtonCreate;

	csubgcd[i][2].gd.pos.x = 10+(csubgcd[i][0].gd.pos.width-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor))/2; csubgcd[i][2].gd.pos.y = csubgcd[i][1].gd.pos.y;
	csubgcd[i][2].gd.pos.width = -1;
	csubgcd[i][2].gd.flags = gg_visible;
	csublabel[i][2].text = (unichar_t *) _("_Delete");
	csublabel[i][2].text_is_1byte = true;
	csublabel[i][2].text_in_resource = true;
	csubgcd[i][2].gd.label = &csublabel[i][2];
	csubgcd[i][2].gd.cid = CID_ContextDel+i*100;
	csubgcd[i][2].gd.handle_controlevent = GFI_ContextDel;
	csubgcd[i][2].creator = GButtonCreate;

	csubgcd[i][3].gd.pos.x = 10+(csubgcd[i][0].gd.pos.width-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)); csubgcd[i][3].gd.pos.y = csubgcd[i][1].gd.pos.y;
	csubgcd[i][3].gd.pos.width = -1;
	csubgcd[i][3].gd.flags = gg_visible;
	csublabel[i][3].text = (unichar_t *) _("_Edit...");
	csublabel[i][3].text_is_1byte = true;
	csublabel[i][3].text_in_resource = true;
	csubgcd[i][3].gd.label = &csublabel[i][3];
	csubgcd[i][3].gd.cid = CID_ContextEdit+i*100;
	csubgcd[i][3].gd.handle_controlevent = GFI_ContextEdit;
	csubgcd[i][3].creator = GButtonCreate;

	csubgcd[i][4].gd.pos.x = csubgcd[i][2].gd.pos.x-2; csubgcd[i][4].gd.pos.y = csubgcd[i][1].gd.pos.y+28;
	csubgcd[i][4].gd.flags = gg_visible;
	csublabel[i][4].text = (unichar_t *) _("Edit Data...");
	csublabel[i][4].text_is_1byte = true;
	csubgcd[i][4].gd.label = &csublabel[i][4];
	csubgcd[i][4].gd.cid = CID_ContextEditData+i*100;
	csubgcd[i][4].gd.handle_controlevent = GFI_ContextEditData;
	csubgcd[i][4].creator = GButtonCreate;
    }

    conaspects[3].selected = true;	/* chaining subs is most likely to be used, so select it by default */

    congcd[0].gd.pos.x = 4; congcd[0].gd.pos.y = 10;
    congcd[0].gd.pos.width = 250;
    congcd[0].gd.pos.height = 260;
    congcd[0].gd.u.tabs = conaspects;
    congcd[0].gd.flags = gg_visible | gg_enabled;
    congcd[0].gd.cid = CID_Contextual;
    /*congcd[0].gd.handle_controlevent = GFI_AspectChange;*/
    congcd[0].creator = GTabSetCreate;

/******************************************************************************/
    memset(&mcgcd,0,sizeof(mcgcd));
    memset(&mclabel,'\0',sizeof(mclabel));

    k=0;
    mclabel[k].text = (unichar_t *) _("Mac Style Set:");
    mclabel[k].text_is_1byte = true;
    mcgcd[k].gd.label = &mclabel[k];
    mcgcd[k].gd.pos.x = 10; mcgcd[k].gd.pos.y = 7;
    mcgcd[k].gd.flags = gg_visible | gg_enabled;
    mcgcd[k++].creator = GLabelCreate;

    mclabel[k].text = (unichar_t *) _("Automatic");
    mclabel[k].text_is_1byte = true;
    mcgcd[k].gd.label = &mclabel[k];
    mcgcd[k].gd.pos.x = 10; mcgcd[k].gd.pos.y = 20;
    mcgcd[k].gd.flags = (sf->macstyle==-1) ? (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    mcgcd[k].gd.cid = CID_MacAutomatic;
    mcgcd[k].gd.handle_controlevent = GFI_MacAutomatic;
    mcgcd[k++].creator = GRadioCreate;

    mcgcd[k].gd.pos.x = 90; mcgcd[k].gd.pos.y = 20;
    mcgcd[k].gd.flags = (sf->macstyle!=-1) ? (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    mcgcd[k].gd.handle_controlevent = GFI_MacAutomatic;
    mcgcd[k++].creator = GRadioCreate;

    mcgcd[k].gd.pos.x = 110; mcgcd[k].gd.pos.y = 20;
    mcgcd[k].gd.pos.width = 120; mcgcd[k].gd.pos.height = 7*12+10;
    mcgcd[k].gd.flags = (sf->macstyle==-1) ? (gg_visible | gg_list_multiplesel) : (gg_visible | gg_enabled | gg_list_multiplesel);
    mcgcd[k].gd.cid = CID_MacStyles;
    mcgcd[k].gd.u.list = macstyles;
    mcgcd[k++].creator = GListCreate;

    mclabel[k].text = (unichar_t *) _("FOND Name:");
    mclabel[k].text_is_1byte = true;
    mcgcd[k].gd.label = &mclabel[k];
    mcgcd[k].gd.pos.x = 10; mcgcd[k].gd.pos.y = mcgcd[k-1].gd.pos.y + mcgcd[k-1].gd.pos.height+8;
    mcgcd[k].gd.flags = gg_visible | gg_enabled;
    mcgcd[k++].creator = GLabelCreate;

    mclabel[k].text = (unichar_t *) sf->fondname;
    mclabel[k].text_is_1byte = true;
    mcgcd[k].gd.label = sf->fondname==NULL ? NULL : &mclabel[k];
    mcgcd[k].gd.pos.x = 90; mcgcd[k].gd.pos.y = mcgcd[k-1].gd.pos.y - 4;
    mcgcd[k].gd.flags = gg_visible | gg_enabled;
    mcgcd[k].gd.cid = CID_MacFOND;
    mcgcd[k++].creator = GTextFieldCreate;


    mcs = MacStyleCode(sf,NULL);
    for ( i=0; macstyles[i].text!=NULL; ++i )
	macstyles[i].selected = (mcs&(int) (intpt) macstyles[i].userdata)? 1 : 0;

/******************************************************************************/
    memset(&smsublabel,0,sizeof(smsublabel));
    memset(&smsubgcd,0,sizeof(smsubgcd));
    memset(&smgcd,0,sizeof(smgcd));
    memset(&smaspects,'\0',sizeof(smaspects));

    for ( i=0; i<4; ++i ) {
	smaspects[i].text = (unichar_t *) smnames[i];
	smaspects[i].text_is_1byte = true;
	smaspects[i].gcd = smsubgcd[i];

	smsubgcd[i][0].gd.pos.x = 10; smsubgcd[i][0].gd.pos.y = 10;
	smsubgcd[i][0].gd.pos.width = ngcd[11].gd.pos.width;
	smsubgcd[i][0].gd.pos.height = 150;
	smsubgcd[i][0].gd.flags = gg_visible | gg_enabled;
	smsubgcd[i][0].gd.cid = CID_SMList+i*100;
	smsubgcd[i][0].gd.u.list = SMList(sf,smtypes[i]);
	smsubgcd[i][0].gd.handle_controlevent = GFI_SMSelChanged;
	smsubgcd[i][0].creator = GListCreate;

	smsubgcd[i][1].gd.pos.x = 10; smsubgcd[i][1].gd.pos.y = smsubgcd[i][0].gd.pos.y+smsubgcd[i][0].gd.pos.height+4;
	smsubgcd[i][1].gd.pos.width = -1;
	smsubgcd[i][1].gd.flags = gg_visible | gg_enabled;
	smsublabel[i][1].text = (unichar_t *) _("_New...");
	smsublabel[i][1].text_is_1byte = true;
	smsublabel[i][1].text_in_resource = true;
	smsubgcd[i][1].gd.label = &smsublabel[i][1];
	smsubgcd[i][1].gd.cid = CID_SMNew+i*100;
	smsubgcd[i][1].gd.handle_controlevent = GFI_SMNew;
	smsubgcd[i][1].creator = GButtonCreate;

	smsubgcd[i][2].gd.pos.x = 10+(smsubgcd[i][0].gd.pos.width-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor))/2; smsubgcd[i][2].gd.pos.y = smsubgcd[i][1].gd.pos.y;
	smsubgcd[i][2].gd.pos.width = -1;
	smsubgcd[i][2].gd.flags = gg_visible;
	smsublabel[i][2].text = (unichar_t *) _("_Delete");
	smsublabel[i][2].text_in_resource = true;
	smsublabel[i][2].text_is_1byte = true;
	smsubgcd[i][2].gd.label = &smsublabel[i][2];
	smsubgcd[i][2].gd.cid = CID_SMDel+i*100;
	smsubgcd[i][2].gd.handle_controlevent = GFI_SMDel;
	smsubgcd[i][2].creator = GButtonCreate;

	smsubgcd[i][3].gd.pos.x = 10+(smsubgcd[i][0].gd.pos.width-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)); smsubgcd[i][3].gd.pos.y = smsubgcd[i][1].gd.pos.y;
	smsubgcd[i][3].gd.pos.width = -1;
	smsubgcd[i][3].gd.flags = gg_visible;
	smsublabel[i][3].text = (unichar_t *) _("_Edit...");
	smsublabel[i][3].text_in_resource = true;
	smsublabel[i][3].text_is_1byte = true;
	smsubgcd[i][3].gd.label = &smsublabel[i][3];
	smsubgcd[i][3].gd.cid = CID_SMEdit+i*100;
	smsubgcd[i][3].gd.handle_controlevent = GFI_SMEdit;
	smsubgcd[i][3].creator = GButtonCreate;

	if ( i==1 ) {
	    smsubgcd[i][4].gd.pos.x = 10; smsubgcd[i][4].gd.pos.y = smsubgcd[i][1].gd.pos.y+30;
	    smsubgcd[i][4].gd.flags = SFAnyConvertableSM(sf) ? gg_visible | gg_enabled : gg_visible;
	    smsublabel[i][4].text = (unichar_t *) _("Convert from OpenType...");
	    smsublabel[i][4].text_is_1byte = true;
	    smsubgcd[i][4].gd.label = &smsublabel[i][4];
	    smsubgcd[i][4].gd.handle_controlevent = GFI_SMConvert;
	    smsubgcd[i][4].creator = GButtonCreate;
	}
    }

    smaspects[1].selected = true;	/* Contextual glyph subs is most likely to be used, so select it by default */

    smgcd[0].gd.pos.x = 4; smgcd[0].gd.pos.y = 10;
    smgcd[0].gd.pos.width = 250;
    smgcd[0].gd.pos.height = 260;
    smgcd[0].gd.u.tabs = smaspects;
    smgcd[0].gd.flags = gg_visible | gg_enabled;
    smgcd[0].gd.cid = CID_StateMachine;
    /*smgcd[0].gd.handle_controlevent = GFI_AspectChange;*/
    smgcd[0].creator = GTabSetCreate;

/******************************************************************************/
    memset(&mfgcd,0,sizeof(mfgcd));
    memset(&mflabel,'\0',sizeof(mflabel));

    GCDFillMacFeat(mfgcd,mflabel,250,sf->features, false);

/******************************************************************************/

    memset(&mlabel,0,sizeof(mlabel));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&aspects,'\0',sizeof(aspects));

    i = 0;

    aspects[i].text = (unichar_t *) _("Names");
    d->old_aspect = 0;
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = ngcd;

    aspects[i].text = (unichar_t *) _("General");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = psgcd;

    aspects[i].text = (unichar_t *) _("PS UID");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = xugcd;

    d->private_aspect = i;
    aspects[i].text = (unichar_t *) _("PS Private");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = pgcd;

    d->ttfv_aspect = i;
    aspects[i].text = (unichar_t *) _("OS/2");
    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = vagcd;

    d->tn_aspect = i;
    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _("TTF Names");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = tngcd;

    d->tx_aspect = i;
/* xgettext won't use non-ASCII messages */
    aspects[i].text = (unichar_t *) U_("ΤεΧ");	/* Tau epsilon Chi, in greek */
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = txgcd;

    aspects[i].text = (unichar_t *) _("Size");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = szgcd;

    aspects[i].text = (unichar_t *) _("Comment");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = comgcd;

    aspects[i].text = (unichar_t *) _("Mark Classes");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = mkgcd;

    aspects[i].text = (unichar_t *) _("Anchor Classes");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = atgcd;

    aspects[i].text = (unichar_t *) _("Contextual");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = congcd;

    aspects[i].text = (unichar_t *) _("Mac");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = mcgcd;

    aspects[i].text = (unichar_t *) _("Mac Features");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = mfgcd;

/* GT: Mac State Machines. Please make this very short */
    aspects[i].text = (unichar_t *) _("Mac SM");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = smgcd;

    aspects[defaspect].selected = true;

    mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
    mgcd[0].gd.u.tabs = aspects;
#ifndef FONTFORGE_CONFIG_INFO_HORIZONTAL
    mgcd[0].gd.flags = gg_visible | gg_enabled | gg_tabset_vert;
    mgcd[0].gd.pos.width = 260+85;
    mgcd[0].gd.pos.height = 325;
#else
    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].gd.pos.width = 260;
    mgcd[0].gd.pos.height = 325;
#endif
    mgcd[0].gd.handle_controlevent = GFI_AspectChange;
    mgcd[0].gd.cid = CID_Tabs;
    mgcd[0].creator = GTabSetCreate;

    mgcd[1].gd.pos.x = 30-3; mgcd[1].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
    mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _("_OK");
    mlabel[1].text_is_1byte = true;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = GFI_OK;
    mgcd[1].gd.cid = CID_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.pos.x = -30; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y+3;
    mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _("_Cancel");
    mlabel[2].text_is_1byte = true;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = GFI_Cancel;
    mgcd[2].gd.cid = CID_Cancel;
    mgcd[2].creator = GButtonCreate;

    mgcd[3].gd.pos.x = 2; mgcd[3].gd.pos.y = 2;
    mgcd[3].gd.pos.width = pos.width-4; mgcd[3].gd.pos.height = pos.height-4;
    mgcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    mgcd[3].gd.cid = CID_MainGroup;
    mgcd[3].creator = GGroupCreate;

    GGadgetsCreate(gw,mgcd);
    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = 12;
    rq.weight = 400;
    font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GGadgetSetFont(atgcd[0].ret,font);
    GTextInfoListFree(namelistnames);
    GTextInfoListFree(pgcd[0].gd.u.list);
    GTextInfoListFree(atgcd[0].gd.u.list);
    if ( GTabSetGetTabLines(mgcd[0].ret)>3 ) {
	int offset = (GTabSetGetTabLines(mgcd[0].ret)-2)*GDrawPointsToPixels(NULL,20);
	GDrawResize(gw,pos.width,pos.height+offset);
    }

    wattrs.mask = wam_events;
    GGadgetGetSize(tngcd[1].ret,&pos);
    pos.y += pos.height + GDrawPointsToPixels(NULL,3);
    pos.x = 5; pos.width = pos.height = d->tn_height = d->tn_width = 100;
    d->tn_v = GWidgetCreateSubWindow(GGadgetGetWindow(tngcd[1].ret),&pos,tn_e_h,d,&wattrs);	/* Will be resized later */
    d->tn_smallactive = -1;
    rq.family_name = sans;
    rq.point_size = 10;
    d->tn_font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    {
	int as, ds, ld;
	GDrawFontMetrics(d->tn_font,&as,&ds,&ld);
	d->tn_as = as; d->tn_fh = as+ds;
    }
    {
	static GBox small = { 0 };
	GGadgetData gd;
	GTextInfo label;
	static unichar_t nullstr[1] = { 0 };

	small.main_background = small.main_foreground = COLOR_DEFAULT;
	small.main_foreground = 0x0000ff;
	memset(&gd,'\0',sizeof(gd));
	memset(&label,'\0',sizeof(label));

	label.text = nullstr;
	label.font = d->tn_font;
	gd.pos.height = d->tn_fh;
	gd.pos.width = 40;
	gd.label = &label;
	gd.box = &small;
	gd.flags = gg_enabled | gg_pos_in_pixels | gg_dontcopybox | gg_text_xim;
	d->tn_smalledit = GTextFieldCreate(d->tn_v,&gd,d);
    }

    GWidgetIndicateFocusGadget(ngcd[1].ret);
    ProcessListSel(d);
    GFI_AspectChange(mgcd[0].ret,NULL);
    GFI_InitMarkClasses(d);
    GGadgetSetList(GWidgetGetControl(gw,CID_StyleName),StyleNames(sf->fontstyle_name),false);
    GDrawSetVisible(d->tn_v,true);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);

    if ( sync ) {
	while ( !d->done )
	    GDrawProcessOneEvent(NULL);
    }
}

void FontMenuFontInfo(void *_fv) {
    FontInfo( ((FontView *) _fv)->sf,-1,false);
}

void FontInfoDestroy(FontView *fv) {
    if ( fv->fontinfo )
	GFI_CancelClose( (struct gfi_data *) (fv->fontinfo) );
}

void FontInfoInit(void) {
    static int done = false;
    int i, j;
    static GTextInfo *needswork[] = {
	macstyles, widthclass, weightclass, fstype, pfmfamily, ibmfamily,
	panfamily, panserifs, panweight, panprop, pancontrast, panstrokevar,
	panarmstyle, panletterform, panmidline, panxheight, mslanguages,
	ttfnameids, interpretations,
	NULL
    };
    static char **needswork2[] = { texparams, texpopups,
	mathparams, mathpopups, extparams, extpopups,
    NULL };

    if ( done )
return;
    done = true;
    for ( j=0; needswork[j]!=NULL; ++j ) {
	for ( i=0; needswork[j][i].text!=NULL; ++i )
	    needswork[j][i].text = (unichar_t *) S_((char *) needswork[j][i].text);
    }
    for ( j=0; needswork2[j]!=NULL; ++j ) {
	for ( i=0; needswork2[j][i]!=NULL; ++i )
	    needswork2[j][i] = _(needswork2[j][i]);
    }
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
