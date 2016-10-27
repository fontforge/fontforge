/* -*- coding: utf-8 -*- */
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
#include "autowidth.h"
#include "bitmapchar.h"
#include "dumppfa.h"
#include "encoding.h"
#include "featurefile.h"
#include "fontforgeui.h"
#include "lookups.h"
#include "namelist.h"
#include "ofl.h"
#include "parsepfa.h"
#include "parsettf.h"
#include <ustring.h>
#include <chardata.h>
#include <utype.h>
#include "unicoderange.h"
#include <locale.h>
#include "sfundo.h"
#include "collabclientui.h"

extern int _GScrollBar_Width;
extern GBox _ggadget_Default_Box;
#define ACTIVE_BORDER   (_ggadget_Default_Box.active_border)
#define MAIN_FOREGROUND (_ggadget_Default_Box.main_foreground)

#include <gkeysym.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

static int last_aspect=0;

GTextInfo emsizes[] = {
    { (unichar_t *) "1000", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "1024", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "2048", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "4096", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};

/* Note that we are storing integer data in a pointer field, hence the casts. They are not to be dereferenced. */
GTextInfo interpretations[] = {
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "None", ignore "Interpretation|" */
/* GT: In french this could be "Aucun" or "Aucune" depending on the gender */
/* GT:  of "Interpretation" */
    { (unichar_t *) N_("Interpretation|None"), NULL, 0, 0, (void *) ui_none, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
/*  { (unichar_t *) N_("Adobe Public Use Defs."), NULL, 0, 0, (void *) ui_adobe, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'}, */
/*  { (unichar_t *) N_("Greek"), NULL, 0, 0, (void *) ui_greek, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'}, */
    { (unichar_t *) N_("Japanese"), NULL, 0, 0, (void *) ui_japanese, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Traditional Chinese"), NULL, 0, 0, (void *) ui_trad_chinese, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Simplified Chinese"), NULL, 0, 0, (void *) ui_simp_chinese, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Korean"), NULL, 0, 0, (void *) ui_korean, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
/*  { (unichar_t *) N_("AMS Public Use"), NULL, 0, 0, (void *) ui_ams, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'}, */
    GTEXTINFO_EMPTY
};
GTextInfo macstyles[] = {
    { (unichar_t *) N_("MacStyles|Bold"), NULL, 0, 0, (void *) sf_bold, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("MacStyles|Italic"), NULL, 0, 0, (void *) sf_italic, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("MacStyles|Condense"), NULL, 0, 0, (void *) sf_condense, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("MacStyles|Expand"), NULL, 0, 0, (void *) sf_extend, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("MacStyles|Underline"), NULL, 0, 0, (void *) sf_underline, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("MacStyles|Outline"), NULL, 0, 0, (void *) sf_outline, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("MacStyles|Shadow"), NULL, 0, 0, (void *) sf_shadow, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo widthclass[] = {
    { (unichar_t *) N_("Ultra-Condensed (50%)"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Extra-Condensed (62.5%)"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Condensed (75%)"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Semi-Condensed (87.5%)"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Medium (100%)"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Semi-Expanded (112.5%)"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Expanded (125%)"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Extra-Expanded (150%)"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ultra-Expanded (200%)"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo weightclass[] = {
    { (unichar_t *) N_("100 Thin"), NULL, 0, 0, (void *) 100, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("200 Extra-Light"), NULL, 0, 0, (void *) 200, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("300 Light"), NULL, 0, 0, (void *) 300, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("400 Regular"), NULL, 0, 0, (void *) 400, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("500 Medium"), NULL, 0, 0, (void *) 500, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("600 Semi-Bold"), NULL, 0, 0, (void *) 600, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("700 Bold"), NULL, 0, 0, (void *) 700, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("800 Extra-Bold"), NULL, 0, 0, (void *) 800, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("900 Black"), NULL, 0, 0, (void *) 900, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo fstype[] = {
    { (unichar_t *) N_("Never Embed/No Editing"), NULL, 0, 0, (void *) 0x02, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Printable Document"), NULL, 0, 0, (void *) 0x04, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Editable Document"), NULL, 0, 0, (void *) 0x08, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Installable Font"), NULL, 0, 0, (void *) 0x00, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo pfmfamily[] = {
    { (unichar_t *) N_("Serif"), NULL, 0, 0, (void *) 0x11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sans-Serif"), NULL, 0, 0, (void *) 0x21, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Monospace"), NULL, 0, 0, (void *) 0x31, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Script", ignore "cursive|" */
/* GT: English uses "script" to me a general writing style (latin, greek, kanji) */
/* GT: and the cursive handwriting style. Here we mean cursive handwriting. */
    { (unichar_t *) N_("cursive|Script"), NULL, 0, 0, (void *) 0x41, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Decorative"), NULL, 0, 0, (void *) 0x51, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo ibmfamily[] = {
    { (unichar_t *) N_("No Classification"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Old Style Serifs"), NULL, 0, 0, (void *) 0x100, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("OSS Rounded Legibility"), NULL, 0, 0, (void *) 0x101, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("OSS Geralde"), NULL, 0, 0, (void *) 0x102, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("OSS Venetian"), NULL, 0, 0, (void *) 0x103, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("OSS Modified Venetian"), NULL, 0, 0, (void *) 0x104, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("OSS Dutch Modern"), NULL, 0, 0, (void *) 0x105, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("OSS Dutch Trad"), NULL, 0, 0, (void *) 0x106, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("OSS Contemporary"), NULL, 0, 0, (void *) 0x107, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("OSS Calligraphic"), NULL, 0, 0, (void *) 0x108, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("OSS Miscellaneous"), NULL, 0, 0, (void *) 0x10f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Transitional Serifs"), NULL, 0, 0, (void *) 0x200, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("TS Direct Line"), NULL, 0, 0, (void *) 0x201, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("TS Script"), NULL, 0, 0, (void *) 0x202, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("TS Miscellaneous"), NULL, 0, 0, (void *) 0x20f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Modern Serifs"), NULL, 0, 0, (void *) 0x300, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("MS Italian"), NULL, 0, 0, (void *) 0x301, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("MS Script"), NULL, 0, 0, (void *) 0x302, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("MS Miscellaneous"), NULL, 0, 0, (void *) 0x30f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Clarendon Serifs"), NULL, 0, 0, (void *) 0x400, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CS Clarendon"), NULL, 0, 0, (void *) 0x401, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CS Modern"), NULL, 0, 0, (void *) 0x402, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CS Traditional"), NULL, 0, 0, (void *) 0x403, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CS Newspaper"), NULL, 0, 0, (void *) 0x404, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CS Stub Serif"), NULL, 0, 0, (void *) 0x405, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CS Monotone"), NULL, 0, 0, (void *) 0x406, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CS Typewriter"), NULL, 0, 0, (void *) 0x407, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CS Miscellaneous"), NULL, 0, 0, (void *) 0x40f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Slab Serifs"), NULL, 0, 0, (void *) 0x500, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Slab Serifs|SS Monotone"), NULL, 0, 0, (void *) 0x501, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Slab Serifs|SS Humanist"), NULL, 0, 0, (void *) 0x502, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Slab Serifs|SS Geometric"), NULL, 0, 0, (void *) 0x503, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Slab Serifs|SS Swiss"), NULL, 0, 0, (void *) 0x504, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Slab Serifs|SS Typewriter"), NULL, 0, 0, (void *) 0x505, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Slab Serifs|SS Miscellaneous"), NULL, 0, 0, (void *) 0x50f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Freeform Serifs"), NULL, 0, 0, (void *) 0x700, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("FS Modern"), NULL, 0, 0, (void *) 0x701, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("FS Miscellaneous"), NULL, 0, 0, (void *) 0x70f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sans-Serif"), NULL, 0, 0, (void *) 0x800, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sans-Serif|SS IBM NeoGrotesque Gothic"), NULL, 0, 0, (void *) 0x801, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sans-Serif|SS Humanist"), NULL, 0, 0, (void *) 0x802, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sans-Serif|SS Low-x Round Geometric"), NULL, 0, 0, (void *) 0x803, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sans-Serif|SS High-x Round Geometric"), NULL, 0, 0, (void *) 0x804, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sans-Serif|SS NeoGrotesque Gothic"), NULL, 0, 0, (void *) 0x805, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sans-Serif|SS Modified Grotesque Gothic"), NULL, 0, 0, (void *) 0x806, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sans-Serif|SS Typewriter Gothic"), NULL, 0, 0, (void *) 0x809, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sans-Serif|SS Matrix"), NULL, 0, 0, (void *) 0x80a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sans-Serif|SS Miscellaneous"), NULL, 0, 0, (void *) 0x80f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ornamentals"), NULL, 0, 0, (void *) 0x900, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("O Engraver"), NULL, 0, 0, (void *) 0x901, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("O Black Letter"), NULL, 0, 0, (void *) 0x902, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("O Decorative"), NULL, 0, 0, (void *) 0x903, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("O Three Dimensional"), NULL, 0, 0, (void *) 0x904, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("O Miscellaneous"), NULL, 0, 0, (void *) 0x90f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Scripts"), NULL, 0, 0, (void *) 0xa00, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("S Uncial"), NULL, 0, 0, (void *) 0xa01, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("S Brush Joined"), NULL, 0, 0, (void *) 0xa02, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("S Formal Joined"), NULL, 0, 0, (void *) 0xa03, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("S Monotone Joined"), NULL, 0, 0, (void *) 0xa04, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("S Calligraphic"), NULL, 0, 0, (void *) 0xa05, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("S Brush Unjoined"), NULL, 0, 0, (void *) 0xa06, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("S Formal Unjoined"), NULL, 0, 0, (void *) 0xa07, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("S Monotone Unjoined"), NULL, 0, 0, (void *) 0xa08, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("S Miscellaneous"), NULL, 0, 0, (void *) 0xa0f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Symbolic"), NULL, 0, 0, (void *) 0xc00, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sy Mixed Serif"), NULL, 0, 0, (void *) 0xc03, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sy Old Style Serif"), NULL, 0, 0, (void *) 0xc06, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sy Neo-grotesque Sans Serif"), NULL, 0, 0, (void *) 0xc07, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sy Miscellaneous"), NULL, 0, 0, (void *) 0xc0f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo stylemap[] = {
    { (unichar_t *) N_("Automatic"), NULL, 0, 0, (void *) -1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("None"), NULL, 0, 0, (void *) 0x00, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Regular"), NULL, 0, 0, (void *) 0x40, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Italic"), NULL, 0, 0, (void *) 0x01, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Bold"), NULL, 0, 0, (void *) 0x20, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Bold Italic"), NULL, 0, 0, (void *) 0x21, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo os2versions[] = {
    { (unichar_t *) N_("OS2Version|Automatic"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("1"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("2"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("3"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("4"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo gaspversions[] = {
    { (unichar_t *) N_("0"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("1"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panfamily[] = {
    { (unichar_t *) N_("PanoseFamily|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseFamily|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Latin: Text & Display"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Script", ignore "cursive|" */
/* GT: English uses "script" to me a general writing style (latin, greek, kanji) */
/* GT: and the cursive handwriting style. Here we mean cursive handwriting. */
    { (unichar_t *) N_("cursive|Latin: Handwritten"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Latin: Decorative"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Latin: Pictorial/Symbol"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "6", NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "7", NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panunknown[] = {
    { (unichar_t *) "Any", NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "No Fit", NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "2", NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "3", NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "4", NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "5", NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "6", NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "7", NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panserifs[] = {
    { (unichar_t *) N_("PanoseSerifs|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseSerifs|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cove"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Obtuse Cove"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Square Cove"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Obtuse Square Cove"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseSerivfs|Square"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseSerifs|Thin"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Bone"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Exaggerated"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Triangle"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal Sans"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Obtuse Sans"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Perpendicular Sans"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Flared"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseSerivfs|Rounded"), NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panweight[] = {
    { (unichar_t *) N_("PanoseWeight|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseWeight|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very Light"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Light"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseWeight|Thin"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Book"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Medium"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Demi"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Bold"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Heavy"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Black"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Nord"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panprop[] = {
    { (unichar_t *) N_("PanoseProportion|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseProportion|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Old Style"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Modern"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Even Width"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Expanded"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Condensed"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very Expanded"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very Condensed"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Monospaced"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo pancontrast[] = {
    { (unichar_t *) N_("PanoseContrast|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|None"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|Very Low"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|Low"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|Medium Low"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|Medium"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|Medium High"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|High"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|Very High"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panstrokevar[] = {
    { (unichar_t *) N_("PanoseStrokeVariation|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseStrokeVariation|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("No Variation"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Gradual/Diagonal"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Gradual/Transitional"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Gradual/Vertical"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Gradual/Horizontal"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Rapid/Vertical"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Rapid/Horizontal"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Instant/Vertical"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Instant/Horizontal"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panarmstyle[] = {
    { (unichar_t *) N_("PanoseArmStyle|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseArmStyle|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Straight Arms/Horizontal"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Straight Arms/Wedge"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Straight Arms/Vertical"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Straight Arms/Single Serif"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Straight Arms/Double Serif"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Non-Straight Arms/Horizontal"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Non-Straight Arms/Wedge"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Non-Straight Arms/Vertical"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Non-Straight Arms/Single Serif"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Non-Straight Arms/Double Serif"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panletterform[] = {
    { (unichar_t *) N_("PanoseLetterform|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseLetterform|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal/Contact"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal/Weighted"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal/Boxed"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal/Flattened"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal/Rounded"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal/Off-Center"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal/Square"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oblique/Contact"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oblique/Weighted"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oblique/Boxed"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oblique/Flattened"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oblique/Rounded"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oblique/Off-Center"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oblique/Square"), NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panmidline[] = {
    { (unichar_t *) N_("PanoseMidline|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|Standard/Trimmed"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|Standard/Pointed"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|Standard/Serifed"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|High/Trimmed"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|High/Pointed"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|High/Serifed"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|Constant/Trimmed"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|Constant/Pointed"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|Constant/Serifed"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|Low/Trimmed"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|Low/Pointed"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseMidline|Low/Serifed"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panxheight[] = {
    { (unichar_t *) N_("PanoseXHeight|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXHeight|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXHeight|Constant/Small"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXHeight|Constant/Standard"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXHeight|Constant/Large"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXHeight|Ducking/Small"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXHeight|Ducking/Standard"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXHeight|Ducking/Large"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
/* Latin: Hand written */
static GTextInfo pantool[] = {
    { (unichar_t *) N_("PanoseTool|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseTool|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Flat Nib"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Pressure Point"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Engraved"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ball (Round Cap)"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Brush"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Rough"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Felt Pen/Brush Tip"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Wild Brush - Drips a lot"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panspacing[] = {
    { (unichar_t *) N_("PanoseSpacing|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseSpacing|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Proportional Spaced"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Monospaced"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "4", NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "5", NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "6", NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "7", NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panasprat[] = {
    { (unichar_t *) N_("PanoseAspectRatio|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseAspectRatio|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very Condensed"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Condensed"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Expanded"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very Expanded"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "7", NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo pancontrast2[] = {
    { (unichar_t *) N_("PanoseContrast|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|None"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very Low"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Low"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Medium Low"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Medium"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Medium High"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("High"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very High"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo pantopology[] = {
    { (unichar_t *) N_("PanoseTopology|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseTopology|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Roman Disconnected"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Roman Trailing"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Roman Connected"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cursive Disconnected"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cursive Trailing"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cursive Connected"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Blackletter Disconnected"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Blackletter Trailing"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Blackletter Connected"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panform[] = {
    { (unichar_t *) N_("PanoseForm|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseForm|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Upright/No Wrapping"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Upright/Some Wrapping"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Upright/More Wrapping"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Upright/Extreme Wrapping"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oblique/No Wrapping"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oblique/Some Wrapping"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oblique/More Wrapping"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oblique/Extreme Wrapping"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Exaggerated/No Wrapping"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Exaggerated/Some Wrapping"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Exaggerated/More Wrapping"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Exaggerated/Extreme Wrapping"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panfinials[] = {
    { (unichar_t *) N_("PanoseFinials|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseFinials|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("None/No loops"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("None/Closed loops"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("None/Open loops"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sharp/No loops"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sharp/Closed loops"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sharp/Open loops"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tapered/No loops"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tapered/Closed loops"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tapered/Open loops"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Round/No loops"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Round/Closed loops"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Round/Open loops"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panxascent[] = {
    { (unichar_t *) N_("PanoseXAscent|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXAscent|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXAscent|Very Low"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXAscent|Low"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXAscent|Medium"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXAscent|High"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseXAscent|Very High"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "7", NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
/* Latin: Decorative */
static GTextInfo panclass[] = {
    { (unichar_t *) N_("PanoseClass|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseClass|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Derivative"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Non-standard Topology"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Non-standard Elements"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Non-standard Aspect"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Initials"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cartoon"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Picture Stems"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ornamented"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Text and Background"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Collage"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Montage"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panaspect[] = {
    { (unichar_t *) N_("PanoseAspect|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseAspect|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Super Condensed"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very Condensed"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Condensed"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Extended"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very Extended"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Super Extended"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo pancontrast3[] = {
    { (unichar_t *) N_("PanoseContrast|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseContrast|None"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very Low"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Low"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Medium Low"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Medium"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Medium High"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("High"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very High"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Horizontal Low"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Horizontal Medium"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Horizontal High"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Broken"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panserifvar[] = {
    { (unichar_t *) N_("PanoseSerifVariant|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseSerifVariant|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cove"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Obtuse Cove"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Square Cove"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Obtuse Square Cove"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseSerivfs|Square"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseSerifs|Thin"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
/*!*/{ (unichar_t *) N_("Oval"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Exaggerated"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Triangle"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal Sans"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Obtuse Sans"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Perpendicular Sans"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Flared"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseSerivfs|Rounded"), NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
/* Um, these guys are only supposed to go up to 15, so why does this have a 16? */
/*!*/{ (unichar_t *) N_("PanoseSerivfs|Script"), NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo pantreatment[] = {
    { (unichar_t *) N_("PanoseTreatment|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseTreatment|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Solid Fill"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("No Fill"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Patterned Fill"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Complex Fill"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Shaped Fill"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Drawn/Distressed"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panlining[] = {
    { (unichar_t *) N_("PanoseLining|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseLining|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseLining|None"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseLining|Inline"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseLining|Outline"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseLining|Engraved (Multiple Lines)"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseLining|Shadow"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseLining|Relief"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseLining|Backdrop"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo pantopology2[] = {
    { (unichar_t *) N_("PanoseTopology|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseTopology|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Standard"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Square"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Multiple Segment"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Deco (E,M,S) Waco midline"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Uneven Weighting"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Diverse Arms"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Diverse Forms"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lombardic Forms"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Upper Case in Lower Case"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Implied Topology"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Horseshoe E and A"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cursive"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Blackletter"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Swash Variance"), NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo pancharrange[] = {
    { (unichar_t *) N_("PanoseCharRange|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseCharRange|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Extended Collection"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Literals"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("No Lower Case"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Small Caps"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "6", NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "7", NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "8", NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "9", NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
/* Latin: Symbol */
static GTextInfo pankind[] = {
    { (unichar_t *) N_("PanoseKind|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseKind|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Montages"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Pictures"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Shapes"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Scientific"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Music"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Expert"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Patterns"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Borders"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Icons"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Logos"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Industry specific"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo panasprat2[] = {
    { (unichar_t *) N_("PanoseAspect|Any"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("PanoseAspect|No Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("No Width"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Exceptionally Wide"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Super Wide"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very Wide"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Wide"), NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Normal"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Narrow"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Very Narrow"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "10", NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "11", NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "12", NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "13", NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "14", NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "15", NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};

static struct titlelist { char *name; GTextInfo *variants; } panoses[][9] = {
    /*Any*/    {{ N_("Class2"), panunknown }, { N_("Class3"), panunknown }, { N_("Class4"), panunknown }, { N_("Class5"), panunknown }, { N_("Class6"), panunknown }, { N_("Class7"), panunknown }, { N_("Class8"), panunknown }, { N_("Class9"), panunknown }, { N_("Class10"), panunknown }},
    /*NoFit*/  {{ N_("Class2"), panunknown }, { N_("Class3"), panunknown }, { N_("Class4"), panunknown }, { N_("Class5"), panunknown }, { N_("Class6"), panunknown }, { N_("Class7"), panunknown }, { N_("Class8"), panunknown }, { N_("Class9"), panunknown }, { N_("Class10"), panunknown }},
    /*LtnTxt*/ {{ N_("_Serifs"), panserifs }, { N_("Panose|_Weight"), panweight }, { N_("_Proportion"), panprop }, { N_("_Contrast"), pancontrast }, { N_("Stroke _Variation"), panstrokevar }, { N_("_Arm Style"), panarmstyle }, { N_("_Letterform"), panletterform }, { N_("_Midline"), panmidline }, { N_("_X-Height"), panxheight }},
    /*LtnHnd*/ {{ N_("_Tool"), pantool }, { N_("Panose|_Weight"), panweight }, { N_("_Spacing"), panspacing }, { N_("_Aspect Ratio"), panasprat }, { N_("_Contrast"), pancontrast2 }, { N_("_Topology"), pantopology }, { N_("F_orm"), panform }, { N_("F_inials"), panfinials }, { N_("_X-Ascent"), panxascent }},
    /*LtnDcr*/ {{ N_("_Class"), panclass }, { N_("Panose|_Weight"), panweight }, { N_("_Aspect"), panaspect }, { N_("C_ontrast"), pancontrast3 }, { N_("_Serif Variant"), panserifvar }, { N_("T_reatment"), pantreatment }, { N_("_Lining"), panlining }, { N_("_Topology"), pantopology2 }, { N_("Char. _Range"), pancharrange }},
					    /* Yup, really should be "unknown", no weights permitted, nor aspect ratios */
    /*LtnSym*/ {{ N_("_Kind"), pankind }, { N_("Panose|_Weight"), panunknown }, { N_("_Spacing"), panspacing }, { N_("_Aspect Ratio"), panunknown }, { N_("AR: Char 94"), panasprat2 }, { N_("AR: Char 119"), panasprat2 }, { N_("AR: Char 157"), panasprat2 }, { N_("AR: Char 163"), panasprat2 }, { N_("AR: Char 211"), panasprat2 }},
    /* 6 */    {{ N_("Class2"), panunknown }, { N_("Class3"), panunknown }, { N_("Class4"), panunknown }, { N_("Class5"), panunknown }, { N_("Class6"), panunknown }, { N_("Class7"), panunknown }, { N_("Class8"), panunknown }, { N_("Class9"), panunknown }, { N_("Class10"), panunknown }},
    {{NULL, NULL}}
};

/* There is a similar list at the end of python.c, untranslated strings for */
/*  scripting use */
static GTextInfo mslanguages[] = {
    { (unichar_t *) N_("Afrikaans"), NULL, 0, 0, (void *) 0x436, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Albanian"), NULL, 0, 0, (void *) 0x41c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Malayalam", ignore "Lang|" */
    { (unichar_t *) N_("Lang|Amharic"), NULL, 0, 0, (void *) 0x45e, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Saudi Arabia)"), NULL, 0, 0, (void *) 0x401, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Iraq)"), NULL, 0, 0, (void *) 0x801, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Egypt)"), NULL, 0, 0, (void *) 0xc01, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Libya)"), NULL, 0, 0, (void *) 0x1001, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Algeria)"), NULL, 0, 0, (void *) 0x1401, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Morocco)"), NULL, 0, 0, (void *) 0x1801, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Tunisia)"), NULL, 0, 0, (void *) 0x1C01, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Oman)"), NULL, 0, 0, (void *) 0x2001, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Yemen)"), NULL, 0, 0, (void *) 0x2401, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Syria)"), NULL, 0, 0, (void *) 0x2801, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Jordan)"), NULL, 0, 0, (void *) 0x2c01, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Lebanon)"), NULL, 0, 0, (void *) 0x3001, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Kuwait)"), NULL, 0, 0, (void *) 0x3401, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (U.A.E.)"), NULL, 0, 0, (void *) 0x3801, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Bahrain)"), NULL, 0, 0, (void *) 0x3c01, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic (Qatar)"), NULL, 0, 0, (void *) 0x4001, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Armenian"), NULL, 0, 0, (void *) 0x42b, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Assamese"), NULL, 0, 0, (void *) 0x44d, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Azeri (Latin)"), NULL, 0, 0, (void *) 0x42c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Azeri (Cyrillic)"), NULL, 0, 0, (void *) 0x82c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Basque"), NULL, 0, 0, (void *) 0x42d, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Byelorussian"), NULL, 0, 0, (void *) 0x423, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Bengali"), NULL, 0, 0, (void *) 0x445, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Bengali Bangladesh"), NULL, 0, 0, (void *) 0x845, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Bulgarian"), NULL, 0, 0, (void *) 0x402, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Burmese"), NULL, 0, 0, (void *) 0x455, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Catalan"), NULL, 0, 0, (void *) 0x403, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cambodian"), NULL, 0, 0, (void *) 0x453, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Cherokee"), NULL, 0, 0, (void *) 0x45c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Chinese (Taiwan)"), NULL, 0, 0, (void *) 0x404, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Chinese (PRC)"), NULL, 0, 0, (void *) 0x804, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Chinese (Hong Kong)"), NULL, 0, 0, (void *) 0xc04, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Chinese (Singapore)"), NULL, 0, 0, (void *) 0x1004, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Chinese (Macau)"), NULL, 0, 0, (void *) 0x1404, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Croatian"), NULL, 0, 0, (void *) 0x41a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Croatian Bosnia/Herzegovina"), NULL, 0, 0, (void *) 0x101a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Czech"), NULL, 0, 0, (void *) 0x405, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Danish"), NULL, 0, 0, (void *) 0x406, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Divehi"), NULL, 0, 0, (void *) 0x465, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Dutch"), NULL, 0, 0, (void *) 0x413, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Flemish (Belgian Dutch)"), NULL, 0, 0, (void *) 0x813, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Edo"), NULL, 0, 0, (void *) 0x466, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (British)"), NULL, 0, 0, (void *) 0x809, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (US)"), NULL, 0, 0, (void *) 0x409, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Canada)"), NULL, 0, 0, (void *) 0x1009, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Australian)"), NULL, 0, 0, (void *) 0xc09, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (New Zealand)"), NULL, 0, 0, (void *) 0x1409, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Irish)"), NULL, 0, 0, (void *) 0x1809, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (South Africa)"), NULL, 0, 0, (void *) 0x1c09, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Jamaica)"), NULL, 0, 0, (void *) 0x2009, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Caribbean)"), NULL, 0, 0, (void *) 0x2409, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Belize)"), NULL, 0, 0, (void *) 0x2809, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Trinidad)"), NULL, 0, 0, (void *) 0x2c09, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Zimbabwe)"), NULL, 0, 0, (void *) 0x3009, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Philippines)"), NULL, 0, 0, (void *) 0x3409, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Indonesia)"), NULL, 0, 0, (void *) 0x3809, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Hong Kong)"), NULL, 0, 0, (void *) 0x3c09, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (India)"), NULL, 0, 0, (void *) 0x4009, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("English (Malaysia)"), NULL, 0, 0, (void *) 0x4409, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Estonian"), NULL, 0, 0, (void *) 0x425, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Faeroese"), NULL, 0, 0, (void *) 0x438, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Farsi"), NULL, 0, 0, (void *) 0x429, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Filipino"), NULL, 0, 0, (void *) 0x464, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Finnish"), NULL, 0, 0, (void *) 0x40b, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French French"), NULL, 0, 0, (void *) 0x40c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French Belgium"), NULL, 0, 0, (void *) 0x80c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French Canadian"), NULL, 0, 0, (void *) 0xc0c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French Swiss"), NULL, 0, 0, (void *) 0x100c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French Luxembourg"), NULL, 0, 0, (void *) 0x140c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French Monaco"), NULL, 0, 0, (void *) 0x180c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French West Indies"), NULL, 0, 0, (void *) 0x1c0c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) NU_("French Réunion"), NULL, 0, 0, (void *) 0x200c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French D.R. Congo"), NULL, 0, 0, (void *) 0x240c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French Senegal"), NULL, 0, 0, (void *) 0x280c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French Camaroon"), NULL, 0, 0, (void *) 0x2c0c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) NU_("French Côte d'Ivoire"), NULL, 0, 0, (void *) 0x300c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French Mali"), NULL, 0, 0, (void *) 0x340c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French Morocco"), NULL, 0, 0, (void *) 0x380c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French Haiti"), NULL, 0, 0, (void *) 0x3c0c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("French North Africa"), NULL, 0, 0, (void *) 0xe40c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Frisian"), NULL, 0, 0, (void *) 0x462, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Fulfulde"), NULL, 0, 0, (void *) 0x467, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Gaelic (Scottish)"), NULL, 0, 0, (void *) 0x43c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Gaelic (Irish)"), NULL, 0, 0, (void *) 0x83c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Galician"), NULL, 0, 0, (void *) 0x467, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Georgian"), NULL, 0, 0, (void *) 0x437, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("German German"), NULL, 0, 0, (void *) 0x407, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("German Swiss"), NULL, 0, 0, (void *) 0x807, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("German Austrian"), NULL, 0, 0, (void *) 0xc07, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("German Luxembourg"), NULL, 0, 0, (void *) 0x1007, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("German Liechtenstein"), NULL, 0, 0, (void *) 0x1407, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Greek"), NULL, 0, 0, (void *) 0x408, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Guarani"), NULL, 0, 0, (void *) 0x474, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Gujarati"), NULL, 0, 0, (void *) 0x447, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Hausa"), NULL, 0, 0, (void *) 0x468, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Hawaiian"), NULL, 0, 0, (void *) 0x475, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Hebrew"), NULL, 0, 0, (void *) 0x40d, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Hindi"), NULL, 0, 0, (void *) 0x439, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Hungarian"), NULL, 0, 0, (void *) 0x40e, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ibibio"), NULL, 0, 0, (void *) 0x469, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Icelandic"), NULL, 0, 0, (void *) 0x40f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Igbo"), NULL, 0, 0, (void *) 0x470, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Indonesian"), NULL, 0, 0, (void *) 0x421, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Inuktitut"), NULL, 0, 0, (void *) 0x45d, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Italian"), NULL, 0, 0, (void *) 0x410, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Italian Swiss"), NULL, 0, 0, (void *) 0x810, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Japanese"), NULL, 0, 0, (void *) 0x411, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Kannada"), NULL, 0, 0, (void *) 0x44b, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Kanuri"), NULL, 0, 0, (void *) 0x471, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Kashmiri (India)"), NULL, 0, 0, (void *) 0x860, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Kazakh"), NULL, 0, 0, (void *) 0x43f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Khmer"), NULL, 0, 0, (void *) 0x453, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Kirghiz"), NULL, 0, 0, (void *) 0x440, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Konkani"), NULL, 0, 0, (void *) 0x457, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Korean"), NULL, 0, 0, (void *) 0x412, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Korean (Johab)"), NULL, 0, 0, (void *) 0x812, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lao"), NULL, 0, 0, (void *) 0x454, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Latvian"), NULL, 0, 0, (void *) 0x426, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Latin"), NULL, 0, 0, (void *) 0x476, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lithuanian"), NULL, 0, 0, (void *) 0x427, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lithuanian (Classic)"), NULL, 0, 0, (void *) 0x827, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Macedonian"), NULL, 0, 0, (void *) 0x42f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Malay"), NULL, 0, 0, (void *) 0x43e, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Malay (Brunei)"), NULL, 0, 0, (void *) 0x83e, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Malayalam"), NULL, 0, 0, (void *) 0x44c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Maltese"), NULL, 0, 0, (void *) 0x43a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Manipuri"), NULL, 0, 0, (void *) 0x458, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Maori"), NULL, 0, 0, (void *) 0x481, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Marathi"), NULL, 0, 0, (void *) 0x44e, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Mongolian (Cyrillic)"), NULL, 0, 0, (void *) 0x450, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Mongolian (Mongolian)"), NULL, 0, 0, (void *) 0x850, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Nepali"), NULL, 0, 0, (void *) 0x461, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Nepali (India)"), NULL, 0, 0, (void *) 0x861, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Norwegian (Bokmal)"), NULL, 0, 0, (void *) 0x414, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Norwegian (Nynorsk)"), NULL, 0, 0, (void *) 0x814, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Oriya"), NULL, 0, 0, (void *) 0x448, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oromo"), NULL, 0, 0, (void *) 0x472, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Papiamentu"), NULL, 0, 0, (void *) 0x479, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Pashto"), NULL, 0, 0, (void *) 0x463, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Polish"), NULL, 0, 0, (void *) 0x415, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Portuguese (Portugal)"), NULL, 0, 0, (void *) 0x416, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Portuguese (Brasil)"), NULL, 0, 0, (void *) 0x816, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Punjabi (India)"), NULL, 0, 0, (void *) 0x446, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Punjabi (Pakistan)"), NULL, 0, 0, (void *) 0x846, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Quecha (Bolivia)"), NULL, 0, 0, (void *) 0x46b, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Quecha (Ecuador)"), NULL, 0, 0, (void *) 0x86b, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Quecha (Peru)"), NULL, 0, 0, (void *) 0xc6b, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Rhaeto-Romanic"), NULL, 0, 0, (void *) 0x417, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Romanian"), NULL, 0, 0, (void *) 0x418, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Romanian (Moldova)"), NULL, 0, 0, (void *) 0x818, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Russian"), NULL, 0, 0, (void *) 0x419, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Russian (Moldova)"), NULL, 0, 0, (void *) 0x819, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sami (Lappish)"), NULL, 0, 0, (void *) 0x43b, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sanskrit"), NULL, 0, 0, (void *) 0x43b, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sepedi"), NULL, 0, 0, (void *) 0x46c, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Serbian (Cyrillic)"), NULL, 0, 0, (void *) 0xc1a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Serbian (Latin)"), NULL, 0, 0, (void *) 0x81a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sindhi India"), NULL, 0, 0, (void *) 0x459, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sindhi Pakistan"), NULL, 0, 0, (void *) 0x859, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Sinhalese"), NULL, 0, 0, (void *) 0x45b, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Slovak"), NULL, 0, 0, (void *) 0x41b, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Slovenian"), NULL, 0, 0, (void *) 0x424, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sorbian"), NULL, 0, 0, (void *) 0x42e, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Traditional)"), NULL, 0, 0, (void *) 0x40a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish Mexico"), NULL, 0, 0, (void *) 0x80a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Modern)"), NULL, 0, 0, (void *) 0xc0a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Guatemala)"), NULL, 0, 0, (void *) 0x100a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Costa Rica)"), NULL, 0, 0, (void *) 0x140a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Panama)"), NULL, 0, 0, (void *) 0x180a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Dominican Republic)"), NULL, 0, 0, (void *) 0x1c0a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Venezuela)"), NULL, 0, 0, (void *) 0x200a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Colombia)"), NULL, 0, 0, (void *) 0x240a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Peru)"), NULL, 0, 0, (void *) 0x280a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Argentina)"), NULL, 0, 0, (void *) 0x2c0a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Ecuador)"), NULL, 0, 0, (void *) 0x300a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Chile)"), NULL, 0, 0, (void *) 0x340a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Uruguay)"), NULL, 0, 0, (void *) 0x380a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Paraguay)"), NULL, 0, 0, (void *) 0x3c0a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Bolivia)"), NULL, 0, 0, (void *) 0x400a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (El Salvador)"), NULL, 0, 0, (void *) 0x440a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Honduras)"), NULL, 0, 0, (void *) 0x480a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Nicaragua)"), NULL, 0, 0, (void *) 0x4c0a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Puerto Rico)"), NULL, 0, 0, (void *) 0x500a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (United States)"), NULL, 0, 0, (void *) 0x540a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spanish (Latin America)"), NULL, 0, 0, (void *) 0xe40a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sutu"), NULL, 0, 0, (void *) 0x430, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Swahili (Kenyan)"), NULL, 0, 0, (void *) 0x441, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Swedish (Sweden)"), NULL, 0, 0, (void *) 0x41d, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Swedish (Finland)"), NULL, 0, 0, (void *) 0x81d, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Syriac"), NULL, 0, 0, (void *) 0x45a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Tagalog"), NULL, 0, 0, (void *) 0x464, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tajik"), NULL, 0, 0, (void *) 0x428, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tamazight (Arabic)"), NULL, 0, 0, (void *) 0x45f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tamazight (Latin)"), NULL, 0, 0, (void *) 0x85f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Tamil"), NULL, 0, 0, (void *) 0x449, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tatar (Tatarstan)"), NULL, 0, 0, (void *) 0x444, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Telugu"), NULL, 0, 0, (void *) 0x44a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Thai"), NULL, 0, 0, (void *) 0x41e, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tibetan (PRC)"), NULL, 0, 0, (void *) 0x451, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tibetan Bhutan"), NULL, 0, 0, (void *) 0x851, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tigrinya Ethiopia"), NULL, 0, 0, (void *) 0x473, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tigrinyan Eritrea"), NULL, 0, 0, (void *) 0x873, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tsonga"), NULL, 0, 0, (void *) 0x431, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tswana"), NULL, 0, 0, (void *) 0x432, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Turkish"), NULL, 0, 0, (void *) 0x41f, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Turkmen"), NULL, 0, 0, (void *) 0x442, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Uighur"), NULL, 0, 0, (void *) 0x480, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ukrainian"), NULL, 0, 0, (void *) 0x422, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Urdu (Pakistan)"), NULL, 0, 0, (void *) 0x420, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Urdu (India)"), NULL, 0, 0, (void *) 0x820, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Uzbek (Latin)"), NULL, 0, 0, (void *) 0x443, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Uzbek (Cyrillic)"), NULL, 0, 0, (void *) 0x843, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Venda"), NULL, 0, 0, (void *) 0x433, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Vietnamese"), NULL, 0, 0, (void *) 0x42a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Welsh"), NULL, 0, 0, (void *) 0x452, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Xhosa"), NULL, 0, 0, (void *) 0x434, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lang|Yi"), NULL, 0, 0, (void *) 0x478, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Yiddish"), NULL, 0, 0, (void *) 0x43d, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Yoruba"), NULL, 0, 0, (void *) 0x46a, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Zulu"), NULL, 0, 0, (void *) 0x435, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
/* There is a similar list at the end of python.c, untranslated strings for */
/*  scripting use */
static GTextInfo ttfnameids[] = {
/* Put styles (docs call it subfamily) first because it is most likely to change */
    { (unichar_t *) N_("Styles (SubFamily)"), NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Copyright"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Family"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Fullname"), NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("UniqueID"), NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Version"), NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
/* Don't give user access to PostScriptName, we set that elsewhere */
    { (unichar_t *) N_("Trademark"), NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Manufacturer"), NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Designer"), NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Descriptor"), NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Vendor URL"), NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Designer URL"), NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("License"), NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("License URL"), NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
/* slot 15 is reserved */
    { (unichar_t *) N_("Preferred Family"), NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Preferred Styles"), NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Compatible Full"), NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sample Text"), NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CID findfont Name"), NULL, 0, 0, (void *) 20, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("WWS Family"), NULL, 0, 0, (void *) 21, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("WWS Subfamily"), NULL, 0, 0, (void *) 22, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo otfssfeattags[] = {
/* These should not be translated. They are tags */
    { (unichar_t *) "ss01", NULL, 0, 0, (void *) (intpt) CHR('s','s','0','1'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss02", NULL, 0, 0, (void *) (intpt) CHR('s','s','0','2'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss03", NULL, 0, 0, (void *) (intpt) CHR('s','s','0','3'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss04", NULL, 0, 0, (void *) (intpt) CHR('s','s','0','4'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss05", NULL, 0, 0, (void *) (intpt) CHR('s','s','0','5'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss06", NULL, 0, 0, (void *) (intpt) CHR('s','s','0','6'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss07", NULL, 0, 0, (void *) (intpt) CHR('s','s','0','7'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss08", NULL, 0, 0, (void *) (intpt) CHR('s','s','0','8'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss09", NULL, 0, 0, (void *) (intpt) CHR('s','s','0','9'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss10", NULL, 0, 0, (void *) (intpt) CHR('s','s','1','0'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss11", NULL, 0, 0, (void *) (intpt) CHR('s','s','1','1'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss12", NULL, 0, 0, (void *) (intpt) CHR('s','s','1','2'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss13", NULL, 0, 0, (void *) (intpt) CHR('s','s','1','3'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss14", NULL, 0, 0, (void *) (intpt) CHR('s','s','1','4'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss15", NULL, 0, 0, (void *) (intpt) CHR('s','s','1','5'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss16", NULL, 0, 0, (void *) (intpt) CHR('s','s','1','6'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss17", NULL, 0, 0, (void *) (intpt) CHR('s','s','1','7'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss18", NULL, 0, 0, (void *) (intpt) CHR('s','s','1','8'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss19", NULL, 0, 0, (void *) (intpt) CHR('s','s','1','9'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ss20", NULL, 0, 0, (void *) (intpt) CHR('s','s','2','0'), NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
/* Put styles (docs call it subfamily) first because it is most likely to change */
static GTextInfo unicoderangenames[] = {
    { (unichar_t *) N_("Basic Latin"),				NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Latin-1 Supplement"),			NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Latin Extended-A"),			NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Latin Extended-B"),			NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("IPA Extensions"),			NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Spacing Modifier Letters"),		NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Combining Diacritical Marks"),		NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Greek and Coptic"),			NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Coptic"),				NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cyrillic & Supplement"),		NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Armenian"),				NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Hebrew"),				NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Vai"),					NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic"),				NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("N'Ko"),					NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Devanagari"),				NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Bengali"),				NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Gurmukhi"),				NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Gujarati"),				NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Oriya"),				NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tamil"),				NULL, 0, 0, (void *) 20, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Telugu"),				NULL, 0, 0, (void *) 21, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Kannada"),				NULL, 0, 0, (void *) 22, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Malayalam"),				NULL, 0, 0, (void *) 23, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Thai"),					NULL, 0, 0, (void *) 24, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lao"),					NULL, 0, 0, (void *) 25, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Georgian"),				NULL, 0, 0, (void *) 26, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Balinese"),				NULL, 0, 0, (void *) 27, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Hangul Jamo"),				NULL, 0, 0, (void *) 28, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Latin Extended Additional"),		NULL, 0, 0, (void *) 29, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Greek Extended"),			NULL, 0, 0, (void *) 30, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("General Punctuation"),			NULL, 0, 0, (void *) 31, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Subscripts and Superscripts"),		NULL, 0, 0, (void *) 32, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Currency Symbols"),			NULL, 0, 0, (void *) 33, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Combining Diacritical Marks for Symbols"), NULL, 0, 0, (void *) 34, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Letterlike Symbols"),			NULL, 0, 0, (void *) 35, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Numeric Forms"),			NULL, 0, 0, (void *) 36, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arrows (& Supplements A&B)"),		NULL, 0, 0, (void *) 37, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Mathematical Operators (Suppl. & Misc.)"), NULL, 0, 0, (void *) 38, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Miscellaneous Technical"),		NULL, 0, 0, (void *) 39, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Control Pictures"),			NULL, 0, 0, (void *) 40, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Optical Character Recognition"),	NULL, 0, 0, (void *) 41, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Enclosed Alphanumerics"),		NULL, 0, 0, (void *) 42, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Box Drawing"),				NULL, 0, 0, (void *) 43, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Block Elements"),			NULL, 0, 0, (void *) 44, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Geometric Shapes"),			NULL, 0, 0, (void *) 45, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Miscellaneous Symbols"),		NULL, 0, 0, (void *) 46, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Dingbats"),				NULL, 0, 0, (void *) 47, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CJK Symbols and Punctuation"),		NULL, 0, 0, (void *) 48, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Hiragana"),				NULL, 0, 0, (void *) 49, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Katakana & Phonetic Extensions"),	NULL, 0, 0, (void *) 50, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Bopomofo & Extended"),			NULL, 0, 0, (void *) 51, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Hangul Compatibility Jamo"),		NULL, 0, 0, (void *) 52, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Phags-pa"),				NULL, 0, 0, (void *) 53, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Enclosed CJK Letters and Months"),	NULL, 0, 0, (void *) 54, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CJK Compatibility"),			NULL, 0, 0, (void *) 55, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Hangul Syllables"),			NULL, 0, 0, (void *) 56, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Non-Basic Multilingual Plane"),		NULL, 0, 0, (void *) 57, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Phoenician"),				NULL, 0, 0, (void *) 58, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CJK Unified Ideographs"),		NULL, 0, 0, (void *) 59, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
		/* And too much other stuff to put in this string */
    { (unichar_t *) N_("Private Use Area"),			NULL, 0, 0, (void *) 60, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CJK Compatibility Ideographs"),		NULL, 0, 0, (void *) 61, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Alphabetic Presentation Forms"),	NULL, 0, 0, (void *) 62, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic Presentation Forms-A"),		NULL, 0, 0, (void *) 63, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Combining Half Marks"),			NULL, 0, 0, (void *) 64, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("CJK Compatibility Forms"),		NULL, 0, 0, (void *) 65, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Small Form Variants"),			NULL, 0, 0, (void *) 66, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Arabic Presentation Forms-B"),		NULL, 0, 0, (void *) 67, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Halfwidth and Fullwidth Forms"),	NULL, 0, 0, (void *) 68, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Specials"),				NULL, 0, 0, (void *) 69, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tibetan"),				NULL, 0, 0, (void *) 70, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Syriac"),				NULL, 0, 0, (void *) 71, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Thaana"),				NULL, 0, 0, (void *) 72, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sinhala"),				NULL, 0, 0, (void *) 73, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Myanmar"),				NULL, 0, 0, (void *) 74, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ethiopic"),				NULL, 0, 0, (void *) 75, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cherokee"),				NULL, 0, 0, (void *) 76, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Unified Canadian Aboriginal Syllabics"),NULL, 0, 0, (void *) 77, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ogham"),				NULL, 0, 0, (void *) 78, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Runic"),				NULL, 0, 0, (void *) 79, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Khmer"),				NULL, 0, 0, (void *) 80, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Mongolian"),				NULL, 0, 0, (void *) 81, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Braille Patterns"),			NULL, 0, 0, (void *) 82, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Yi Syllables/Radicals"),		NULL, 0, 0, (void *) 83, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tagalog/Hanunno/Buhid/Tagbanwa"),	NULL, 0, 0, (void *) 84, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Old Italic"),				NULL, 0, 0, (void *) 85, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Gothic"),				NULL, 0, 0, (void *) 86, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Deseret"),				NULL, 0, 0, (void *) 87, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Musical Symbols, Byzantine & Western"), NULL, 0, 0, (void *) 88, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Mathematical Alphanumeric Symbols"),	NULL, 0, 0, (void *) 89, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Private Use (planes 15&16)"),		NULL, 0, 0, (void *) 90, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Variation Selectors"),			NULL, 0, 0, (void *) 91, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tags"),					NULL, 0, 0, (void *) 92, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Limbu"),				NULL, 0, 0, (void *) 93, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tai Le"),				NULL, 0, 0, (void *) 94, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("New Tai Lue"),				NULL, 0, 0, (void *) 95, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Buginese"),				NULL, 0, 0, (void *) 96, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Glagolitic"),				NULL, 0, 0, (void *) 97, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tifinagh"),				NULL, 0, 0, (void *) 98, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Yijing Hexagram Symbols"),		NULL, 0, 0, (void *) 99, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Syloti Nagri"),				NULL, 0, 0, (void *) 100, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Linear B Syllabary/Ideograms, Agean Num."),	NULL, 0, 0, (void *) 101, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ancient Greek Numbers"),		NULL, 0, 0, (void *) 102, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ugaritic"),				NULL, 0, 0, (void *) 103, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Old Persian"),				NULL, 0, 0, (void *) 104, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Shavian"),				NULL, 0, 0, (void *) 105, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Osmanya"),				NULL, 0, 0, (void *) 106, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cypriot Syllabary"),			NULL, 0, 0, (void *) 107, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Kharoshthi"),				NULL, 0, 0, (void *) 108, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Tai Xuan Jing Symbols"),		NULL, 0, 0, (void *) 109, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cuneiform Numbers & Punctuation"),	NULL, 0, 0, (void *) 110, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Counting Rod Numerals"),		NULL, 0, 0, (void *) 111, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Sudanese"),				NULL, 0, 0, (void *) 112, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Lepcha"),				NULL, 0, 0, (void *) 113, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ol Chiki"),				NULL, 0, 0, (void *) 114, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Saurashtra"),				NULL, 0, 0, (void *) 115, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Kayah Li"),				NULL, 0, 0, (void *) 116, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Rejang"),				NULL, 0, 0, (void *) 117, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Cham"),					NULL, 0, 0, (void *) 118, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Ancient Symbols"),			NULL, 0, 0, (void *) 119, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Phaistos Disc"),			NULL, 0, 0, (void *) 120, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Carian/Lycian/Lydian"),			NULL, 0, 0, (void *) 121, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Mahjong & Domino Tiles"),		NULL, 0, 0, (void *) 122, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Unassigned Bit 123"),			NULL, 0, 0, (void *) 123, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Unassigned Bit 124"),			NULL, 0, 0, (void *) 124, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Unassigned Bit 125"),			NULL, 0, 0, (void *) 125, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Unassigned Bit 126"),			NULL, 0, 0, (void *) 126, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Unassigned Bit 127"),			NULL, 0, 0, (void *) 127, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo codepagenames[] = {
    { (unichar_t *) N_("1252, Latin-1"),			NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("1250, Latin-2 (Eastern Europe)"),	NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("1251, Cyrillic"),			NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("1253, Greek"),				NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("1254, Turkish"),			NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("1255, Hebrew"),				NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("1256, Arabic"),				NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("1257, Windows Baltic"),			NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("1258, Vietnamese"),			NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 9"),			NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 10"),			NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 11"),			NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 12"),			NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 13"),			NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 14"),			NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 15"),			NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("874, Thai"),				NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("932, JIS/Japan"),			NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("936, Simplified Chinese"),		NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("949, Korean Wansung"),			NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("950, Traditional Chinese"),		NULL, 0, 0, (void *) 20, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("1361, Korean Johab"),			NULL, 0, 0, (void *) 21, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 22"),			NULL, 0, 0, (void *) 22, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 23"),			NULL, 0, 0, (void *) 23, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 24"),			NULL, 0, 0, (void *) 24, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 25"),			NULL, 0, 0, (void *) 25, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 26"),			NULL, 0, 0, (void *) 26, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 27"),			NULL, 0, 0, (void *) 27, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 28"),			NULL, 0, 0, (void *) 28, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Mac Roman"),				NULL, 0, 0, (void *) 29, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("OEM Charset"),				NULL, 0, 0, (void *) 30, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Symbol Charset"),			NULL, 0, 0, (void *) 31, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 32"),			NULL, 0, 0, (void *) 32, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 33"),			NULL, 0, 0, (void *) 33, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 34"), 			NULL, 0, 0, (void *) 34, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 35"),			NULL, 0, 0, (void *) 35, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 36"),			NULL, 0, 0, (void *) 36, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 37"),			NULL, 0, 0, (void *) 37, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 38"), 			NULL, 0, 0, (void *) 38, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 39"),			NULL, 0, 0, (void *) 39, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 40"),			NULL, 0, 0, (void *) 40, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 41"),			NULL, 0, 0, (void *) 41, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 42"),			NULL, 0, 0, (void *) 42, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 43"),			NULL, 0, 0, (void *) 43, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 44"),			NULL, 0, 0, (void *) 44, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 45"),			NULL, 0, 0, (void *) 45, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 46"),			NULL, 0, 0, (void *) 46, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Reserved Bit 47"),			NULL, 0, 0, (void *) 47, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("869, IBM Greek"),			NULL, 0, 0, (void *) 48, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("866, MS-DOS Russian"),			NULL, 0, 0, (void *) 49, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("865, MS_DOS Nordic"),			NULL, 0, 0, (void *) 50, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("864, Arabic"),				NULL, 0, 0, (void *) 51, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("863, MS-DOS Canadian French"),		NULL, 0, 0, (void *) 52, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("862, Hebrew"),				NULL, 0, 0, (void *) 53, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("861, MS-DOS Icelandic"),		NULL, 0, 0, (void *) 54, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("860, MS-DOS Portuguese"),		NULL, 0, 0, (void *) 55, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("857, IBM Turkish"),			NULL, 0, 0, (void *) 56, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("855, IBM Cyrillic; primarily Russian"),	NULL, 0, 0, (void *) 57, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("852, Latin 2"),				NULL, 0, 0, (void *) 58, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("775, MS-DOS Baltic"),			NULL, 0, 0, (void *) 59, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("737, Greek; former 437 G"),		NULL, 0, 0, (void *) 60, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("708, Arabic ASMO 708"),			NULL, 0, 0, (void *) 61, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("850, WE/Latin 1"),			NULL, 0, 0, (void *) 62, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("437, US"),				NULL, 0, 0, (void *) 63, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static char *TN_DefaultName(GGadget *g, int r, int c);
static void TN_StrIDEnable(GGadget *g,GMenuItem *mi, int r, int c);
static void TN_LangEnable(GGadget *g,GMenuItem *mi, int r, int c);
static struct col_init ci[] = {
    { me_enum, NULL, mslanguages, TN_LangEnable, N_("Language") },
    { me_enum, NULL, ttfnameids, TN_StrIDEnable, N_("String ID") },
    { me_func, TN_DefaultName, NULL, NULL, N_("String") }
};
static struct col_init ssci[] = {
    { me_enum, NULL, mslanguages, NULL, N_("Language") },
    { me_enum, NULL, otfssfeattags, NULL, N_("Feature Tags") },
    { me_string, NULL, NULL, NULL, N_("Friendly Name") }
};
static struct col_init sizeci[] = {
    { me_enum, NULL, mslanguages, NULL, N_("Language") },
    { me_string, NULL, NULL, NULL, N_("Name") }
};
static GTextInfo gridfit[] = {
    { (unichar_t *) N_("No Grid Fit"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Grid Fit"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo antialias[] = {
    { (unichar_t *) N_("No Anti-Alias"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Anti-Alias"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo symsmooth[] = {
    { (unichar_t *) N_("No Symmetric-Smooth"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Symmetric-Smoothing"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo gfsymsmooth[] = {
    { (unichar_t *) N_("No Grid Fit w/ Sym-Smooth"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Grid Fit w/ Sym-Smooth"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static struct col_init gaspci[] = {
    { me_int , NULL, NULL, NULL, N_("Gasp|For Pixels Per EM <= Value") },
    { me_enum, NULL, gridfit, NULL, N_("Gasp|Grid Fit") },			/* 1 */
    { me_enum, NULL, antialias, NULL, N_("Gasp|Anti-Alias") },			/* 2 */
    { me_enum, NULL, symsmooth, NULL, N_("Gasp|Symmetric Smoothing") },		/* 8 */
    { me_enum, NULL, gfsymsmooth, NULL, N_("Gasp|Grid Fit w/ Sym Smooth") }	/* 4 */
};
static GTextInfo splineorder[] = {
    { (unichar_t *) N_("Cubic"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Quadratic"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static GTextInfo layertype[] = {
    { (unichar_t *) N_("Layer|Foreground"), NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 1, 0, 1, 0, 0, '\0'},
    { (unichar_t *) N_("Layer|Background"), NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};
static void Layers_BackgroundEnable(GGadget *g,GMenuItem *mi, int r, int c);
static struct col_init layersci[] = {
    { me_string, NULL, NULL, NULL, N_("Layer Name") },
    { me_enum  , NULL, splineorder, NULL, N_("Curve Type") },
    { me_enum  , NULL, layertype, Layers_BackgroundEnable, N_("Type") },
    { me_int   , NULL, NULL, NULL, N_("Orig layer") }
};
static char *GFI_Mark_PickGlyphsForClass(GGadget *g,int r, int c);
static struct col_init marks_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Set Name") },
    { me_funcedit, GFI_Mark_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the set") },
};
static struct col_init markc_ci[] = {
    { me_string, NULL, NULL, NULL, N_("Class Name") },
    { me_funcedit, GFI_Mark_PickGlyphsForClass, NULL, NULL, N_("Glyphs in the class") },
};

struct langstyle { int lang; const char *str; };
#define LANGSTYLE_EMPTY { 0, NULL }

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

static const char regulardutch[] = "Normaal";
static const char mediumdutch[] = "Gemiddeld";
static const char bookdutch[] = "Boek";
static const char bolddutch[] = "Vet";
static const char demibolddutch[] = "Dik";
/*static const char demibolddutch[] = "Halfvet";*/
static const char lightdutch[] = "Licht";
/*static const char lightdutch[] = "Licht mager";*/
static const char blackdutch[] = "Extra vet";
static const char italicdutch[] = "Cursief";
static const char italicdutch2[] = "italiek";
static const char obliquedutch2[] = "oblique";
static const char obliquedutch[] = "Schuin";
static const char condenseddutch[] = "Smal";
static const char expandeddutch[] = "Breed";
static const char outlinedutch[] = "Uitlijning";
/*static const char outlinedutch[] = "Contour";*/

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

static const char regularpl[] = "podstawowa";
static const char demiboldpl[] = "półgruba";
static const char boldpl[] = "pogrubiona";
static const char thinpl[] = "cienka";
static const char lightpl[] = "bardzo cienka";
static const char heavypl[] = "bardzo gruba";
static const char italicpl[] = "pochyła";
static const char obliquepl[] = "pochyła";
static const char condensedpl[] = "wąska";
static const char expandedpl[] = "szeroka";
static const char outlinepl[] = "konturowa";
static const char mediumpl[] = "zwykła";
static const char bookpl[] = "zwykła";

static struct langstyle regs[] = { {0x409, regulareng}, { 0x40c, regularfren }, { 0x410, regularital }, { 0x407, regulargerm }, { 0x40a, regularspan }, { 0x419, regularru }, { 0x40e, regularhu },
	{ 0x413, regulardutch}, { 0x41d, regularswed }, { 0x414, regularnor },
	{ 0x406, regulardanish}, {0x415, regularpl }, { 0x804, "正常"},
	{ 0x408, "κανονική"}, { 0x42a, "Chuẩn"}, { 0x418, "Normal"}, LANGSTYLE_EMPTY};
static struct langstyle meds[] = { {0x409, mediumeng}, { 0x410, mediumital },
	{ 0x40c, mediumfren }, { 0x407, mediumgerm }, { 0x40e, mediumhu },
	{ 0x406, mediumdanish}, {0x415, mediumpl }, { 0x804, "中等"},
	{ 0x408, "µεσαία"}, { 0x42a, "Vừa"}, { 0x413, mediumdutch},
        { 0x418, "Mediu"}, LANGSTYLE_EMPTY};
static struct langstyle books[] = { {0x409, bookeng}, { 0x410, bookital },
	{ 0x407, bookgerm }, { 0x40e, bookhu }, { 0x40e, bookhu2 },
	{ 0x415, bookpl}, { 0x804, "书体"}, { 0x408, "ßιßλίου"},
	{ 0x42a, "Sách"}, { 0x413, bookdutch}, { 0x418, "Carte"}, LANGSTYLE_EMPTY};
static struct langstyle bolds[] = { {0x409, boldeng}, { 0x410, boldital }, { 0x40c, boldfren }, { 0x407, boldgerm }, { 0x407, boldgerm2 }, { 0x40a, boldspan}, { 0x419, boldru }, { 0x40e, boldhu }, { 0x40e, boldhu2 },
	{ 0x413, bolddutch}, { 0x41d, boldswed }, { 0x414, boldnor },
	{ 0x406, bolddanish}, { 0x415, boldpl}, { 0x804, "粗体"},
	{ 0x408, "έντονη"}, { 0x42a, "Đậm"}, {0x418,"Aldin"}, LANGSTYLE_EMPTY};
static struct langstyle italics[] = { {0x409, italiceng}, { 0x410, italicital }, { 0x40c, italicfren }, { 0x407, italicgerm }, { 0x40a, italicspan}, { 0x419, italicru }, { 0x40e, italichu },
	{ 0x413, italicdutch}, { 0x413, italicdutch2}, { 0x41d, italicswed }, { 0x414, italicnor },
	{ 0x406, italicdanish}, { 0x415, italicpl}, { 0x804, "斜体"},
	{ 0x408, "Λειψίας"}, { 0x42a, "Nghiêng" }, { 0x418, "Cursiv"},  LANGSTYLE_EMPTY};
static struct langstyle obliques[] = { {0x409, obliqueeng}, { 0x410, obliqueital },
	{ 0x40c, obliquefren }, { 0x407, obliquegerm }, { 0x419, obliqueru },
	{ 0x40e, obliquehu }, { 0x40e, obliquehu2 }, {0x415, obliquepl},
	{ 0x804, "斜体"}, { 0x408, "πλάγια"},
	{ 0x42a, "Xiên" }, { 0x413, obliquedutch}, { 0x413, obliquedutch2},
        { 0x418, "Înclinat"}, LANGSTYLE_EMPTY};
static struct langstyle demibolds[] = { {0x409, demiboldeng}, {0x409, demiboldeng3}, {0x409, demiboldeng5},
	{ 0x410, demiboldital }, { 0x40c, demiboldfren }, { 0x40c, demiboldfren2 }, { 0x407, demiboldgerm }, { 0x407, demiboldgerm2 },
	{ 0x419, demiboldru }, { 0x40e, demiboldhu }, { 0x40e, demiboldhu2 },
	{ 0x406, demibolddanish}, { 0x415, demiboldpl },
	{ 0x804, "略粗"}, { 0x408, "ηµιέντονη"},
	{ 0x42a, "Nửa đậm"}, { 0x413, demibolddutch},
        {0x418, "Semialdin"}, LANGSTYLE_EMPTY};
static struct langstyle heavys[] = { {0x409, heavyeng}, { 0x410, heavyital },
	{ 0x419, heavyru }, { 0x40e, heavyhu }, { 0x40e, heavyhu2 },
	{ 0x415, heavypl }, { 0x804, "粗"}, LANGSTYLE_EMPTY};
static struct langstyle blacks[] = { {0x409, blackeng}, { 0x410, blackital }, { 0x40c, blackfren }, { 0x407, blackgerm }, { 0x419, blackru }, { 0x40e, blackhu }, { 0x40e, blackhu2 }, { 0x40a, blackspan },
	{ 0x413, blackdutch}, { 0x41d, blackswed }, { 0x414, blacknor }, { 0x406, blackdanish},
	{ 0x415, heavypl }, { 0x804, "黑"},  { 0x408, "µαύρα"},
	{ 0x42a, "Đen"}, { 0x418, "Negru"}, LANGSTYLE_EMPTY };
static struct langstyle thins[] = { {0x409, thineng}, { 0x410, thinital },
	{ 0x419, thinru }, { 0x40e, thinhu }, { 0x415, thinpl},
	{ 0x804, "细"}, LANGSTYLE_EMPTY};
static struct langstyle extralights[] = { {0x409, extralighteng}, {0x409, extralighteng2},
	{ 0x804, "极细"}, LANGSTYLE_EMPTY};
static struct langstyle lights[] = { {0x409, lighteng}, {0x410, lightital}, {0x40c, lightfren}, {0x407, lightgerm}, { 0x419, lightru }, { 0x40e, lighthu }, { 0x40a, lightspan },
	{ 0x413, lightdutch}, { 0x41d, lightswed }, { 0x414, lightnor },
	{ 0x406, lightdanish}, { 0x415, lightpl}, { 0x804, "细"},
	{ 0x408, "λεπτή"}, { 0x42a, "Nhẹ" }, { 0x418, "Subțire"}, LANGSTYLE_EMPTY};
static struct langstyle condenseds[] = { {0x409, condensedeng}, {0x410, condensedital}, {0x40c, condensedfren}, {0x407, condensedgerm}, { 0x419, condensedru }, { 0x40e, condensedhu }, { 0x40a, condensedspan },
	{ 0x413, condenseddutch}, { 0x41d, condensedswed },
	{ 0x414, condensednor }, { 0x406, condenseddanish},
	{ 0x415, condensedpl }, { 0x804, "压缩"},
	{ 0x408, "πυκνή"}, { 0x42a, "Hẹp" }, { 0x418, "Îngust"}, LANGSTYLE_EMPTY};
static struct langstyle expandeds[] = { {0x409, expandedeng}, {0x410, expandedital}, {0x40c, expandedfren}, {0x407, expandedgerm}, { 0x419, expandedru }, { 0x40e, expandedhu }, { 0x40a, expandedspan },
	{ 0x413, expandeddutch}, { 0x41d, expandedswed }, { 0x414, expandednor },
	{ 0x406, expandeddanish}, { 0x415, expandedpl }, { 0x804, "加宽"},
	{ 0x408, "αραιή"}, { 0x42a, "Rộng" }, { 0x418, "Expandat"}, LANGSTYLE_EMPTY};
static struct langstyle outlines[] = { {0x409, outlineeng}, {0x40c, outlinefren},
	{0x407, outlinegerm}, {0x40e, outlinehu}, { 0x406, outlinedanish},
	{0x415, outlinepl}, { 0x804, "轮廓"}, { 0x408, "περιγράμματος"},
	{0x42a, "Nét ngoài" }, { 0x413, outlinedutch}, {0x418, "Contur"}, LANGSTYLE_EMPTY};
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
#define CID_Fontname	1016
#define CID_Em		1017
#define CID_Scale	1018
#define CID_Interpretation	1021
#define CID_Namelist	1024
#define CID_Revision	1025

#define CID_WoffMajor	1050
#define CID_WoffMinor	1051
#define CID_WoffMetadata	1052

#define CID_XUID	1113
#define CID_Human	1114
#define CID_SameAsFontname	1115
#define CID_HasDefBase	1116
#define CID_DefBaseName	1117
#define CID_UseXUID	1118
#define CID_UseUniqueID	1119

#define CID_GuideOrder2		1200
#define CID_IsMixed		1217
#define CID_IsOrder3		1218
#define CID_IsOrder2		1219
#define CID_IsMultiLayer	1220
#define CID_IsStrokedFont	1222
#define CID_StrokeWidth		1223
#define CID_Backgrounds		1226

#define CID_Private		2001
#define CID_Guess		2004
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
#define CID_OS2Version		3031
#define CID_UseTypoMetrics	3032
#define CID_WeightWidthSlopeOnly	3033
// Maybe we could insert these above and change the numbering.
#define CID_CapHeight		3034
#define CID_CapHeightLab		3035
#define CID_XHeight		3036
#define CID_XHeightLab		3037
#define CID_StyleMap		3038

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
#define CID_TNames		5005

#define CID_Language		5006	/* Used by AskForLangNames */

#define CID_SSNames		5050

#define CID_Gasp		5100
#define CID_GaspVersion		5101
#define CID_HeadClearType	5102

#define CID_Comment		6001
#define CID_FontLog		6002

#define CID_MarkClasses		7101

#define CID_MarkSets		7201

#define CID_TeXText		8001
#define CID_TeXMathSym		8002
#define CID_TeXMathExt		8003
#define CID_MoreParams		8005
#define CID_TeXExtraSpLabel	8006
#define CID_TeX			8007	/* through 8014 */
#define CID_TeXBox		8030

#define CID_DesignSize		8301
#define CID_DesignBottom	8302
#define CID_DesignTop		8303
#define CID_StyleID		8304
#define CID_StyleName		8305

#define CID_Tabs		10001
#define CID_OK			10002
#define CID_Cancel		10003
#define CID_MainGroup		10004
#define CID_TopBox		10005

#define CID_Lookups		11000
#define CID_LookupTop		11001
#define CID_LookupUp		11002
#define CID_LookupDown		11003
#define CID_LookupBottom	11004
#define CID_AddLookup		11005
#define CID_AddSubtable		11006
#define CID_EditMetadata	11007
#define CID_EditSubtable	11008
#define CID_DeleteLookup	11009
#define CID_MergeLookup		11010
#define CID_RevertLookups	11011
#define CID_LookupSort		11012
#define CID_ImportLookups	11013
#define CID_LookupWin		11020		/* (GSUB, add 1 for GPOS) */
#define CID_LookupVSB		11022		/* (GSUB, add 1 for GPOS) */
#define CID_LookupHSB		11024		/* (GSUB, add 1 for GPOS) */
#define CID_SaveLookup		11026
#define CID_SaveFeat		11027
#define CID_AddAllAlternates	11028
#define CID_AddDFLT		11029
#define CID_AddLanguage		11030
#define CID_RmLanguage		11031

#define CID_MacAutomatic	16000
#define CID_MacStyles		16001
#define CID_MacFOND		16002

#define CID_Unicode		16100
#define CID_UnicodeEmpties	16101
#define CID_UnicodeTab		16102
#define CID_URangesDefault	16110
#define CID_UnicodeRanges	16111
#define CID_UnicodeList		16112
#define CID_CPageDefault	16120
#define CID_CodePageRanges	16121
#define CID_CodePageList	16122

const char *UI_TTFNameIds(int id) {
    int i;

    FontInfoInit();
    for ( i=0; ttfnameids[i].text!=NULL; ++i )
	if ( ttfnameids[i].userdata == (void *) (intpt) id )
return( (char *) ttfnameids[i].text );

    if ( id==6 )
return( "PostScript" );

return( _("Unknown") );
}

const char *UI_MSLangString(int language) {
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

/* These are PostScript names, and as such should not be translated */
enum { pt_number, pt_boolean, pt_array, pt_code };
static struct { const char *name; short type, arr_size, present; } KnownPrivates[] = {
    { "BlueValues", pt_array, 14, 0 },
    { "OtherBlues", pt_array, 10, 0 },
    { "BlueFuzz", pt_number, 0, 0 },
    { "FamilyBlues", pt_array, 14, 0 },
    { "FamilyOtherBlues", pt_array, 10, 0 },
    { "BlueScale", pt_number, 0, 0 },
    { "BlueShift", pt_number, 0, 0 },
    { "StdHW", pt_array, 1, 0 },
    { "StdVW", pt_array, 1, 0 },
    { "StemSnapH", pt_array, 12, 0 },
    { "StemSnapV", pt_array, 12, 0 },
    { "ForceBold", pt_boolean, 0, 0 },
    { "LanguageGroup", pt_number, 0, 0 },
    { "RndStemUp", pt_number, 0, 0 },
    { "lenIV", pt_number, 0, 0 },
    { "ExpansionFactor", pt_number, 0, 0 },
    { "Erode", pt_code, 0, 0 },
/* I am deliberately not including Subrs and OtherSubrs */
/* The first could not be entered (because it's a set of binary strings) */
/* And the second has special meaning to us and must be handled with care */
    { NULL, 0, 0, 0 }
};

static GTextInfo psprivate_nameids[] = {
/* Don't translate these here either */
    { (unichar_t *) "BlueValues", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "OtherBlues", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "BlueFuzz", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "FamilyBlues", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "FamilyOtherBlues", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "BlueScale", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "BlueShift", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "StdHW", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "StdVW", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "StemSnapH", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "StemSnapV", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ForceBold", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "LanguageGroup", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "RndStemUp", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "lenIV", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "ExpansionFactor", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    { (unichar_t *) "Erode", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0'},
    GTEXTINFO_EMPTY
};

static void PI_KeyEnable(GGadget *g,GMenuItem *mi, int r, int c) {
    int i,j, rows;
    struct matrix_data *strings = _GMatrixEditGet(g, &rows);
    int cols = GMatrixEditGetColCnt(g);
    int found;

    for ( i=0; mi[i].ti.text!=NULL; ++i ) {
	found = 0;
	for ( j=0; j<rows; ++j ) {
	    if ( strcmp((char *) mi[i].ti.text,strings[j*cols+0].u.md_str)==0 ) {
		found=1;
	break;
	    }
	}
	if ( j==r && found ) {
	    mi[i].ti.disabled = false;
	    mi[i].ti.selected = true;
	} else {
	    mi[i].ti.disabled = found;
	    mi[i].ti.selected = false;
	}
    }
}

static struct col_init psprivate_ci[] = {
    { me_stringchoice, NULL, psprivate_nameids, PI_KeyEnable, N_("Key") },
    { me_string, NULL, NULL, NULL, N_("Value") }
    };

static char *rpldecimal(const char *orig,const char *decimal_point, locale_t tmplocale) {
    char *end;
    char *new, *npt; const char *pt;
    int dlen;
    double dval;
    char buffer[40];
    locale_t oldlocale;

    /* if the current locale uses a "." for a decimal point the we don't need */
    /*  to translate the number, just check that it is valid */
    if ( strcmp(decimal_point,".")==0 ) {
	strtod(orig,&end);
	while ( isspace(*end)) ++end;
	if ( *end!='\0' )
return( NULL );
return( copy(orig));
    }

    npt = new = malloc(2*strlen(orig)+10);
    dlen = strlen(decimal_point);
    /* now I want to change the number as little as possible. So if they use */
    /*  arabic numerals we can get by with just switching the decimal point */
    /*  If they use some other number convention, then parse in the old locale*/
    /*  and sprintf in the PostScript ("C") locale. This might lose some */
    /*  precision but the basic idea will get across */
    for ( pt=orig; *pt; ) {
	if ( strncmp(pt,decimal_point,dlen)==0 ) {
	    pt += dlen;
	    *npt++ = '.';
	} else
	    *npt++ = *pt++;
    }
    *npt = '\0';

    oldlocale = uselocale_hack(tmplocale);
    strtod(new,&end);
    uselocale_hack(oldlocale);
    while ( isspace(*end)) ++end;
    if ( *end=='\0' ) {
	char *ret = copy(new);
	free(new);
return( ret );
    }

    /* OK, can we parse the number in the original local? */
    dval = strtod(new,&end);
    while ( isspace(*end)) ++end;
    if ( *end!='\0' ) {
	free(new);
return( NULL );
    }
    free(new);
    oldlocale = uselocale_hack(tmplocale);
    sprintf( buffer, "%g", dval );
    uselocale_hack(oldlocale);
return( copy(buffer));
}

static char *rplarraydecimal(const char *orig,const char *decimal_point, locale_t tmplocale) {
    char *new, *npt, *rpl;
    int nlen;
    const char *start, *pt; int ch;

    nlen = 2*strlen(orig)+10;
    npt = new = calloc(1,nlen+1);
    *npt++ = '[';

    for ( pt=orig; isspace(*pt) || *pt=='['; ++pt );
    while ( *pt!=']' && *pt!='\0' ) {
	start=pt;
	while ( *pt!=']' && *pt!=' ' && *pt!='\0' ) ++pt;
	ch = *pt; *(char *) pt = '\0';
	rpl = rpldecimal(start,decimal_point,tmplocale);
	*(char *) pt = ch;
	if ( rpl==NULL ) {
	    gwwv_post_notice(_("Bad type"),_("Expected array of numbers.\nFailed to parse \"%.*s\" as a number."),
		    pt-start, start);
	    free(new);
return( NULL );
	}
	if ( npt-new + strlen(rpl) + 2 >nlen ) {
	    int noff = npt-new;
	    new = realloc(new,nlen += strlen(rpl)+100);
	    npt = new+noff;
	}
	if ( npt[-1]!='[' )
	    *npt++ = ' ';
	strcpy(npt,rpl);
	free(rpl);
	npt += strlen(npt);
	while ( isspace(*pt)) ++pt;
    }
    *npt++ =']';
    *npt = '\0';
    rpl = copy(new);
    free(new);
return( rpl );
}

static void PSPrivate_FinishEdit(GGadget *g,int r, int c, int wasnew) {
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *strings = _GMatrixEditGet(g, &rows);
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    char *key = strings[r*cols+0].u.md_str;
    char *val = strings[r*cols+1].u.md_str;
    char *pt, *ept, *newval;
    locale_t tmplocale;
    struct psdict *tempdict;

    if ( key==NULL )
return;

    if ( c==0 && (wasnew || val==NULL || *val=='\0')) {
	tempdict = calloc(1,sizeof(*tempdict));
	SFPrivateGuess(d->sf,ly_fore,tempdict,key,true);
	strings[r*cols+1].u.md_str = copy(PSDictHasEntry(tempdict,key));
	PSDictFree(tempdict);
    } else if ( c==1 && val!=NULL ) {
	struct lconv *loc = localeconv();
	int i;
	for ( i=0; KnownPrivates[i].name!=NULL; ++i )
	    if ( strcmp(KnownPrivates[i].name,key)==0 )
	break;
	if ( KnownPrivates[i].name==NULL )	/* If we don't recognize it, leave it be */
return;

	tmplocale = newlocale_hack(LC_NUMERIC_MASK, "C", NULL);
	if (tmplocale == NULL) fprintf(stderr, "Locale error.\n");

	for ( pt=val; isspace(*pt); ++pt );
	for ( ept = val+strlen(val-1); ept>pt && isspace(*ept); --ept );
	if ( KnownPrivates[i].type==pt_boolean ) {
	    if ( strcasecmp(val,"true")==0 || strcasecmp(val,"t")==0 || strtol(val,NULL,10)!=0 ) {
		/* If they make a mistake about case, correct it */
		strings[r*cols+1].u.md_str = copy("true");
		free(val);
		GGadgetRedraw(g);
	    } else if ( strcasecmp(val,"false")==0 || strcasecmp(val,"f")==0 || (*val=='0' && strtol(val,NULL,10)==0) ) {
		strings[r*cols+1].u.md_str = copy("false");
		free(val);
		GGadgetRedraw(g);
	    } else
/* GT: The words "true" and "false" should be left untranslated. We are restricted */
/* GT: here by what PostScript understands, and it only understands the English */
/* GT: words. You may, of course, change it to something like ("true" (vrai) ou "false" (faux)) */
		gwwv_post_notice(_("Bad type"),_("Expected boolean value.\n(\"true\" or \"false\")"));
	} else if ( KnownPrivates[i].type==pt_code ) {
	    if ( *pt!='\0' && (*pt!='{' || (ept>=pt && *ept!='}')) )
		gwwv_post_notice(_("Bad type"),_("Expected PostScript code.\nWhich usually begins with a \"{\" and ends with a \"}\"."));
	} else if ( KnownPrivates[i].type==pt_number ) {
	    newval = rpldecimal(val,loc->decimal_point,tmplocale);
	    if ( newval==NULL )
		gwwv_post_notice(_("Bad type"),_("Expected number."));
	    else if ( strcmp(newval,val)==0 )
		free(newval);		/* No change */
	    else {
		strings[r*cols+1].u.md_str = newval;
		free(val);
		GGadgetRedraw(g);
	    }
	} else if ( KnownPrivates[i].type==pt_array ) {
	    newval = rplarraydecimal(val,loc->decimal_point,tmplocale);
	    if ( newval==NULL )
		gwwv_post_notice(_("Bad type"),_("Expected number."));
	    else if ( strcmp(newval,val)==0 )
		free(newval);		/* No change */
	    else {
		strings[r*cols+1].u.md_str = newval;
		free(val);
		GGadgetRedraw(g);
	    }
	}
	if (tmplocale != NULL) { freelocale_hack(tmplocale); tmplocale = NULL; }
    }
}

static void PSPrivate_MatrixInit(struct matrixinit *mi,struct gfi_data *d) {
    SplineFont *sf = d->sf;
    int i,j;
    struct matrix_data *md;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 2;
    mi->col_init = psprivate_ci;

    mi->initial_row_cnt = sf->private==NULL?0:sf->private->next;
    md = calloc(2*(mi->initial_row_cnt+1),sizeof(struct matrix_data));
    if ( sf->private!=NULL ) {
	for ( i=j=0; i<sf->private->next; ++i ) {
	    md[2*j  ].u.md_str = copy(sf->private->keys[i]);
	    md[2*j+1].u.md_str = copy(sf->private->values[i]);
	    ++j;
	}
    }
    mi->matrix_data = md;

    mi->finishedit = PSPrivate_FinishEdit;
}

static struct psdict *GFI_ParsePrivate(struct gfi_data *d) {
    struct psdict *ret = calloc(1,sizeof(struct psdict));
    GGadget *private = GWidgetGetControl(d->gw,CID_Private);
    int rows, cols = GMatrixEditGetColCnt(private);
    struct matrix_data *strings = GMatrixEditGet(private, &rows);
    int i,j;

    ret->cnt = rows;
    ret->keys = malloc(rows*sizeof(char *));
    ret->values = malloc(rows*sizeof(char *));
    for ( i=j=0; i<rows; ++i ) {
	if ( strings[i*cols+0].u.md_str!=NULL && strings[i*cols+1].u.md_str!=NULL ) {
	    ret->keys[j] = copy(strings[i*cols+0].u.md_str);
	    ret->values[j++] = copy(strings[i*cols+1].u.md_str);
	}
    }
    ret->next = j;
return( ret );
}

static void PSPrivate_EnableButtons(GGadget *g, int r, int c) {
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *strings = _GMatrixEditGet(g, &rows);
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    char *key = r>=0 && r<rows ? strings[r*cols+0].u.md_str : NULL;
    int has_hist = key!=NULL && (
		    strcmp(key,"BlueValues")==0 ||
		    strcmp(key,"OtherBlues")==0 ||
		    strcmp(key,"StdHW")==0 ||
		    strcmp(key,"StemSnapH")==0 ||
		    strcmp(key,"StdVW")==0 ||
		    strcmp(key,"StemSnapV")==0);
    int has_guess = key!=NULL && (has_hist ||
		    strcmp(key,"BlueScale")==0 ||
		    strcmp(key,"BlueShift")==0 ||
		    strcmp(key,"BlueFuzz")==0 ||
		    strcmp(key,"ForceBold")==0 ||
		    strcmp(key,"LanguageGroup")==0 ||
		    strcmp(key,"ExpansionFactor")==0);
    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess), has_guess);
    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Hist ), has_hist );
}

static int PI_Guess(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *private;
    struct psdict *tempdict;
    struct matrix_data *strings;
    int rows, cols, r;
    char *key, *ret;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	private = GWidgetGetControl(gw,CID_Private);
	strings = GMatrixEditGet(private, &rows);	/* Commit any changes made thus far */
	cols = GMatrixEditGetColCnt(private);
	r = GMatrixEditGetActiveRow(private);
	key = strings[r*cols+0].u.md_str;
	tempdict = calloc(1,sizeof(*tempdict));
	SFPrivateGuess(d->sf,ly_fore,tempdict,key,true);
	ret = copy(PSDictHasEntry(tempdict,key));
	if ( ret!=NULL ) {
	    free(strings[r*cols+1].u.md_str);
	    strings[r*cols+1].u.md_str = ret;
	    GGadgetRedraw(private);
	}
	PSDictFree(tempdict);
    }
return( true );
}

static int PI_Hist(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *private;
    struct psdict *tempdict;
    enum hist_type h;
    struct matrix_data *strings;
    int rows, cols, r;
    char *key, *ret;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	private = GWidgetGetControl(gw,CID_Private);
	strings = _GMatrixEditGet(private, &rows);
	cols = GMatrixEditGetColCnt(private);
	r = GMatrixEditGetActiveRow(private);
	key = strings[r*cols+0].u.md_str;
	if ( strcmp(key,"BlueValues")==0 ||
		strcmp(key,"OtherBlues")==0 )
	    h = hist_blues;
	else if ( strcmp(key,"StdHW")==0 ||
		strcmp(key,"StemSnapH")==0 )
	    h = hist_hstem;
	else if ( strcmp(key,"StdVW")==0 ||
		strcmp(key,"StemSnapV")==0 )
	    h = hist_vstem;
	else
return( true );		/* can't happen */
	tempdict = GFI_ParsePrivate(d);
	SFHistogram(d->sf,ly_fore,tempdict,NULL,NULL,h);
	ret = copy(PSDictHasEntry(tempdict,key));
	if ( ret!=NULL ) {
	    free(strings[r*cols+1].u.md_str);
	    strings[r*cols+1].u.md_str = ret;
	    GGadgetRedraw(private);
	}
	PSDictFree(tempdict);
    }
return( true );
}

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

	/* If the user didn't set the full name yet, we guess it from the
	 * postrscript name */
	if ( gfi->human_untitled ) {
	    unichar_t *cp = u_copy(uname);
	    int i=0;
	    /* replace the last hyphen with space */
	    for( i=u_strlen(cp); i>=0; i-- ) {
		if( cp[i] == '-' ) {
		    cp[i] = ' ';
		    break;
		}
	    }
	    /* If the postscript name ends with "Regular" it is recommended not
	     * to include it in the full name */
	    if(u_endswith(cp,c_to_u(" Regular")) || u_endswith(cp,c_to_u(" regular"))) {
		cp[u_strlen(cp) - strlen(" Regular")] ='\0';
	    }
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_Human),cp);
	    free(cp);
	}
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

static int GFI_DefBaseChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	GGadgetSetChecked(GWidgetGetControl(gfi->gw,*_GGadgetGetTitle(g)!='\0'?CID_HasDefBase:CID_SameAsFontname),
		true);
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
	int checked = GGadgetIsChecked(g);
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
	double val = SFGuessItalicAngle(d->sf);
	char buf[30]; unichar_t ubuf[30];
	sprintf( buf, "%.1f", val);
	uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_ItalicAngle),ubuf);
    }
return( true );
}

static void GFI_Close(struct gfi_data *d) {

    GDrawDestroyWindow(d->gw);
    if ( d->sf->fontinfo == d )
	d->sf->fontinfo = NULL;
    FVRefreshAll(d->sf);
    d->done = true;
    /* d will be freed by destroy event */;
}

static void GFI_Mark_FinishEdit(GGadget *g,int r, int c, int wasnew) {
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    int is_markclass = GGadgetGetCid(g) == CID_MarkClasses;

    if ( c==0 ) {
	/* Name can't be null. No spaces. No duplicates */
	int rows, cols = GMatrixEditGetColCnt(g);
	struct matrix_data *classes = _GMatrixEditGet(g,&rows);
	char *pt;
	int i;

	if ( classes[r*cols+c].u.md_str==NULL || *classes[r*cols+c].u.md_str=='\0' ) {
	    ff_post_error(_("No Name"), _("Please specify a name for this mark class or set"));
return;
	}
	for ( pt = classes[r*cols+c].u.md_str; *pt!='\0'; ++pt ) {
	    if ( *pt==' ' ) {
		ff_post_error(_("Bad Name"), _("Mark class/set names should not contain spaces."));
return;
	    }
	}
	for ( i=0; i<rows; ++i ) if ( i!=r ) {
	    if ( strcmp(classes[r*cols+c].u.md_str,classes[i*cols+c].u.md_str)==0 ) {
		ff_post_error(_("Duplicate Name"), _("This name was previously used to identify mark class/set #%d."), i+1);
return;
	    }
	}
    } else {
	if ( is_markclass )
	    ME_ClassCheckUnique(g, r, c, d->sf);
	else
	    ME_SetCheckUnique(g, r, c, d->sf);
    }
}

static char *GFI_Mark_PickGlyphsForClass(GGadget *g,int r, int c) {
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *classes = _GMatrixEditGet(g,&rows);
    char *new = GlyphSetFromSelection(d->sf,d->def_layer,classes[r*cols+c].u.md_str);
return( new );
}

static void GFI_Mark_DeleteClass(GGadget *g,int whichclass) {
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    int rows/*, cols = GMatrixEditGetColCnt(g)*/;
    struct matrix_data *classes = _GMatrixEditGet(g,&rows);
    int is_markclass = GGadgetGetCid(g) == CID_MarkClasses;
    SplineFont *sf = d->sf;
    OTLookup *otl;
    int isgpos;

    whichclass += is_markclass;		/* Classes begin at 1, Sets begin at 0 */
    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next ) {
	    if ( is_markclass ) {
		int old = (otl->lookup_flags & pst_markclass)>>8;
		if ( old==whichclass ) {
		    ff_post_notice(_("Mark Class was in use"),_("This mark class (%s) was used in lookup %s"),
			    classes[2*whichclass-2].u.md_str,otl->lookup_name);
		    otl->lookup_flags &= ~ pst_markclass;
		} else if ( old>whichclass ) {
		    otl->lookup_flags &= ~ pst_markclass;
		    otl->lookup_flags |= (old-1)<<8;
		}
	    } else if ( otl->lookup_flags&pst_usemarkfilteringset ) {
		int old = (otl->lookup_flags & pst_markset)>>16;
		if ( old==whichclass ) {
		    ff_post_notice(_("Mark Set was in use"),_("This mark set (%s) was used in lookup %s"),
			    classes[2*whichclass+0].u.md_str,otl->lookup_name);
		    otl->lookup_flags &= ~ pst_markset;
		    otl->lookup_flags &= ~pst_usemarkfilteringset;
		} else if ( old>whichclass ) {
		    otl->lookup_flags &= ~ pst_markset;
		    otl->lookup_flags |= (old-1)<<16;
		}
	    }
	}
    }
}

static unichar_t **GFI_GlyphListCompletion(GGadget *t,int from_tab) {
    struct gfi_data *d = GDrawGetUserData(GDrawGetParentWindow(GGadgetGetWindow(t)));
    SplineFont *sf = d->sf;

return( SFGlyphNameCompletion(sf,t,from_tab,true));
}

static void GFI_CancelClose(struct gfi_data *d) {
    int isgpos,i,j;

    MacFeatListFree(GGadgetGetUserData((GWidgetGetControl(
	    d->gw,CID_Features))));
    for ( isgpos=0; isgpos<2; ++isgpos ) {
	struct lkdata *lk = &d->tables[isgpos];
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].new )
		SFRemoveLookup(d->sf,lk->all[i].lookup,0);
	    else for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].new )
		    SFRemoveLookupSubTable(d->sf,lk->all[i].subtables[j].subtable,0);
	    }
	    free(lk->all[i].subtables);
	}
	free(lk->all);
    }
    GFI_Close(d);
}

static struct otfname *OtfNameFromStyleNames(GGadget *me) {
    int i;
    int rows;
    struct matrix_data *strings = GMatrixEditGet(me, &rows);
    struct otfname *head=NULL, *last, *cur;

    for ( i=0; i<rows; ++i ) {
	cur = chunkalloc(sizeof(struct otfname));
	cur->lang = strings[2*i  ].u.md_ival;
	cur->name = copy(strings[2*i+1].u.md_str);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
return( head );
}

void GListMoveSelected(GGadget *list,int offset) {
    int32 len; int i,j;
    GTextInfo **old, **new;

    old = GGadgetGetList(list,&len);
    new = calloc(len+1,sizeof(GTextInfo *));
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
	new[j] = malloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	if ( offset<0 ) ++j; else --j;
    }
    for ( i=j=0; i<len; ++i ) if ( !old[i]->selected ) {
	while ( new[j] ) ++j;
	new[j] = malloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	++j;
    }
    new[len] = calloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
}

void GListDelSelected(GGadget *list) {
    int32 len; int i,j;
    GTextInfo **old, **new;

    old = GGadgetGetList(list,&len);
    new = calloc(len+1,sizeof(GTextInfo *));
    for ( i=j=0; i<len; ++i ) if ( !old[i]->selected ) {
	new[j] = malloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	++j;
    }
    new[j] = calloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
}

GTextInfo *GListChangeLine(GGadget *list,int pos, const unichar_t *line) {
    GTextInfo **old, **new;
    int32 i,len;

    old = GGadgetGetList(list,&len);
    new = calloc(len+1,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = malloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	if ( i!=pos )
	    new[i]->text = u_copy(new[i]->text);
	else
	    new[i]->text = u_copy(line);
    }
    new[i] = calloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,pos);
return( new[pos]);
}

GTextInfo *GListAppendLine(GGadget *list,const unichar_t *line,int select) {
    GTextInfo **old, **new;
    int32 i,len;

    old = GGadgetGetList(list,&len);
    new = calloc(len+2,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = malloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	new[i]->text = u_copy(new[i]->text);
	if ( select ) new[i]->selected = false;
    }
    new[i] = calloc(1,sizeof(GTextInfo));
    new[i]->fg = new[i]->bg = COLOR_DEFAULT;
    new[i]->userdata = NULL;
    new[i]->text = u_copy(line);
    new[i]->selected = select;
    new[i+1] = calloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,i);
return( new[i]);
}

GTextInfo *GListChangeLine8(GGadget *list,int pos, const char *line) {
    GTextInfo **old, **new;
    int32 i,len;

    old = GGadgetGetList(list,&len);
    new = calloc(len+1,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = malloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	if ( i!=pos )
	    new[i]->text = u_copy(new[i]->text);
	else
	    new[i]->text = utf82u_copy(line);
    }
    new[i] = calloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,pos);
return( new[pos]);
}

GTextInfo *GListAppendLine8(GGadget *list,const char *line,int select) {
    GTextInfo **old, **new;
    int32 i,len;

    old = GGadgetGetList(list,&len);
    new = calloc(len+2,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = malloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	new[i]->text = u_copy(new[i]->text);
	if ( select ) new[i]->selected = false;
    }
    new[i] = calloc(1,sizeof(GTextInfo));
    new[i]->fg = new[i]->bg = COLOR_DEFAULT;
    new[i]->userdata = NULL;
    new[i]->text = utf82u_copy(line);
    new[i]->selected = select;
    new[i+1] = calloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
    GGadgetScrollListToPos(list,i);
return( new[i]);
}

static int GFI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GFI_CancelClose(d);
    }
return( true );
}

static void BadFamily() {
    ff_post_error(_("Bad Family Name"),_("Bad Family Name, must begin with an alphabetic character."));
}

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
    char *buts[3];
    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;

    if ( u_strlen(ufont)>63 ) {
	ff_post_error(_("Bad Font Name"),_("A PostScript name should be ASCII\nand must not contain (){}[]<>%%/ or space\nand must be shorter than 63 characters"));
return( false );
    }

    if ( *ufamily=='\0' ) {
	ff_post_error(_("A Font Family name is required"),_("A Font Family name is required"));
return( false );
    }
    /* A postscript name cannot be a number. There are two ways it can be a */
    /*  number, it can be a real (which we can check for with strtod) or */
    /*  it can be a "radix number" which is <intval>'#'<intval>. I'll only */
    /*  do a cursory test for that */
    u_strtod(ufamily,&end);
    if ( *end=='\0' || (isdigit(ufamily[0]) && u_strchr(ufamily,'#')!=NULL) ) {
	ff_post_error(_("Bad Font Family Name"),_("A PostScript name may not be a number"));
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
	    if ( gwwv_ask(_("Bad Font Name"),(const char **) buts,0,1,_("Adobe's fontname spec (5088.FontNames.pdf) says that fontnames should not be longer than 29 characters. Do you want to continue anyway?"))==1 )
return( false );
	}
    }
    while ( *ufamily ) {
	if ( *ufamily<' ' || *ufamily>=0x7f ) {
	    ff_post_error(_("Bad Font Family Name"),_("A PostScript name should be ASCII\nand must not contain (){}[]<>%%/ or space"));
return( false );
	}
	++ufamily;
    }

    u_strtod(ufont,&end);
    if ( (*end=='\0' || (isdigit(ufont[0]) && u_strchr(ufont,'#')!=NULL)) &&
	    *ufont!='\0' ) {
	ff_post_error(_("Bad Font Name"),_("A PostScript name may not be a number"));
return( false );
    }
    for ( pt=ufont; *pt; ++pt ) {
	if ( *pt<=' ' || *pt>=0x7f ||
		*pt=='(' || *pt=='[' || *pt=='{' || *pt=='<' ||
		*pt==')' || *pt==']' || *pt=='}' || *pt=='>' ||
		*pt=='%' || *pt=='/' ) {
	    ff_post_error(_("Bad Font Name"),_("A PostScript name should be ASCII\nand must not contain (){}[]<>%%/ or space"));
return( false );
	}
    }
return( true );
}

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

static char *TN_DefaultName(GGadget *g, int r, int c) {
    struct gfi_data *d = GGadgetGetUserData(g);
    int rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);

    if ( strings==NULL || !strings[3*r+2].user_bits )
return( NULL );

return( tn_recalculatedef(d,strings[3*r+1].u.md_ival ));
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
    const struct matrix_data *n1 = pt1, *n2 = pt2;
    char buf1[20], buf2[20];
    const char *l1, *l2;

    if ( n1[1].u.md_ival!=n2[1].u.md_ival )
return( n1[1].u.md_ival - n2[1].u.md_ival );

    l1 = langname(n1[0].u.md_ival,buf1);
    l2 = langname(n2[0].u.md_ival,buf2);
return( strcoll(l1,l2));
}

static int lang_sorter(const void *pt1, const void *pt2) {
    const struct matrix_data *n1 = pt1, *n2 = pt2;
    char buf1[20], buf2[20];
    const char *l1, *l2;

    if ( n1[0].u.md_ival==n2[0].u.md_ival )
return( n1[1].u.md_ival - n2[1].u.md_ival );

    l1 = langname(n1[0].u.md_ival,buf1);
    l2 = langname(n2[0].u.md_ival,buf2);
return( strcoll(l1,l2));
}

static int ms_thislocale = 0;
static int specialvals(const struct matrix_data *n) {
    if ( n[0].u.md_ival == ms_thislocale )
return( -10000000 );
    else if ( (n[0].u.md_ival&0x3ff) == (ms_thislocale&0x3ff) )
return( -10000000 + (n[0].u.md_ival&~0x3ff) );
    if ( n[0].u.md_ival == 0x409 )	/* English */
return( -1000000 );
    else if ( (n[0].u.md_ival&0x3ff) == 9 )
return( -1000000 + (n[0].u.md_ival&~0x3ff) );

return( 1 );
}

static int speciallang_sorter(const void *pt1, const void *pt2) {
    const struct matrix_data *n1 = pt1, *n2 = pt2;
    char buf1[20], buf2[20];
    const char *l1, *l2;
    int pos1=1, pos2=1;

    /* sort so that entries for the current language are first, then English */
    /*  then alphabetical order */
    if ( n1[0].u.md_ival==n2[0].u.md_ival )
return( n1[1].u.md_ival - n2[1].u.md_ival );

    pos1 = specialvals(n1); pos2 = specialvals(n2);
    if ( pos1<0 || pos2<0 )
return( pos1-pos2 );
    l1 = langname(n1[0].u.md_ival,buf1);
    l2 = langname(n2[0].u.md_ival,buf2);
return( strcoll(l1,l2));
}

static void TTFNames_Resort(struct gfi_data *d) {
    int(*compar)(const void *, const void *);
    GGadget *edit = GWidgetGetControl(d->gw,CID_TNames);
    int rows;
    struct matrix_data *strings = GMatrixEditGet(edit, &rows);

    if ( strings==NULL )
return;

    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TNLangSort)) )
	compar = lang_sorter;
    else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TNStringSort)) )
	compar = strid_sorter;
    else
	compar = speciallang_sorter;
    ms_thislocale = d->langlocalecode;
    qsort(strings,rows,3*sizeof(struct matrix_data),compar);
}

static int GFI_Char(struct gfi_data *d,GEvent *event) {
    if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	help("fontinfo.html");
return( true );
    } else if ( GMenuIsCommand(event,H_("Save All|Alt+Ctl+S") )) {
	MenuSaveAll(NULL,NULL,NULL);
return( true );
    } else if ( GMenuIsCommand(event,H_("Quit|Ctl+Q") )) {
	MenuExit(NULL,NULL,NULL);
return( true );
    } else if ( GMenuIsCommand(event,H_("Close|Ctl+Shft+Q") )) {
	GFI_CancelClose(d);
return( true );
    }
return( false );
}

static int CheckActiveStyleTranslation(struct gfi_data *d,
	struct matrix_data *strings,int r, int rows, int iswws ) {
    int i,j, eng_pos, other_pos;
    char *english, *new=NULL, *temp, *pt;
    int other_lang = strings[3*r].u.md_ival;
    int changed = false;
    int search_sid = iswws ? ttf_wwssubfamily : ttf_subfamily;

    for ( i=rows-1; i>=0 ; --i )
	if ( strings[3*i+1].u.md_ival==search_sid &&
		strings[3*i].u.md_ival == 0x409 )
    break;
    if ( i<0 && iswws ) {
	for ( i=rows-1; i>=0; --i )
	    if ( strings[3*i+1].u.md_ival==ttf_subfamily &&
		    strings[3*i].u.md_ival == other_lang ) {
		new = copy(strings[3*i+2].u.md_str);
		changed = true;
	break;
	    }
    } else {
	if ( i<0 || (english = strings[3*i+2].u.md_str)==NULL )
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
			free(strings[3*r+2].u.md_str);
			if ( other_lang==0x415 ) {
			    /* polish needs a word before the translation */
			    strings[3*r+2].u.md_str = malloc(strlen("odmiana ")+strlen(stylelist[i][other_pos].str)+1);
			    strcpy(strings[3*r+2].u.md_str,"odmiana ");
			    strcat(strings[3*r+2].u.md_str,stylelist[i][other_pos].str);
			} else
			    strings[3*r+2].u.md_str = copy(stylelist[i][other_pos].str);
    return( true );
		    }
		    temp = malloc((strlen(new)
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
    }
    if ( changed ) {
	free(strings[3*r+2].u.md_str);
	if ( other_lang==0x415 ) {
	    /* polish needs a word before the translation */
	    strings[3*r+2].u.md_str = malloc(strlen("odmiana ")+strlen(new)+1);
	    strcpy(strings[3*r+2].u.md_str,"odmiana ");
	    strcat(strings[3*r+2].u.md_str,new);
	    free(new);
	} else
	    strings[3*r+2].u.md_str = new;
    } else
	free(new);
return( changed );
}

#define MID_ToggleBase	1
#define MID_MultiEdit	2
#define MID_Delete	3

static void TN_StrPopupDispatch(GWindow gw, GMenuItem *mi, GEvent *e) {
    struct gfi_data *d = GDrawGetUserData(GDrawGetParentWindow(gw));
    GGadget *g = GWidgetGetControl(d->gw,CID_TNames);

    switch ( mi->mid ) {
      case MID_ToggleBase: {
	int rows;
	struct matrix_data *strings = GMatrixEditGet(g, &rows);
	strings[3*d->tn_active+2].frozen = !strings[3*d->tn_active+2].frozen;
	if ( strings[3*d->tn_active+2].frozen ) {
	    free( strings[3*d->tn_active+2].u.md_str );
	    strings[3*d->tn_active+2].u.md_str = NULL;
	} else {
	    strings[3*d->tn_active+2].u.md_str = tn_recalculatedef(d,strings[3*d->tn_active+1].u.md_ival);
	}
	GGadgetRedraw(g);
      } break;
      case MID_MultiEdit:
	GMatrixEditStringDlg(g,d->tn_active,2);
      break;
      case MID_Delete:
	GMatrixEditDeleteRow(g,d->tn_active);
      break;
    }
}

static int menusort(const void *m1, const void *m2) {
    const GMenuItem *mi1 = m1, *mi2 = m2;

    /* Should do a strcoll here, but I never wrote one */
    if ( mi1->ti.text_is_1byte && mi2->ti.text_is_1byte )
return( strcoll( (char *) (mi1->ti.text), (char *) (mi2->ti.text)) );
    else
return( u_strcmp(mi1->ti.text,mi2->ti.text));
}

static void TN_StrIDEnable(GGadget *g,GMenuItem *mi, int r, int c) {
    int rows, i, j;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);

    for ( i=0; mi[i].ti.text!=NULL; ++i ) {
	int strid = (intpt) mi[i].ti.userdata;
	for ( j=0; j<rows; ++j ) if ( j!=r )
	    if ( strings[3*j].u.md_ival == strings[3*r].u.md_ival &&
		    strings[3*j+1].u.md_ival == strid ) {
		mi[i].ti.disabled = true;
	break;
	    }
    }
    qsort(mi,i,sizeof(mi[0]),menusort);
}

static void TN_LangEnable(GGadget *g,GMenuItem *mi, int r, int c) {
    int i;

    for ( i=0; mi[i].ti.text!=NULL; ++i );
    qsort(mi,i,sizeof(mi[0]),menusort);
}

static void TN_NewName(GGadget *g,int row) {
    int rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);

    strings[3*row+1].u.md_ival = ttf_subfamily;
}

static void TN_FinishEdit(GGadget *g,int row,int col,int wasnew) {
    int i,rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);
    uint8 found[ttf_namemax];
    struct gfi_data *d = (struct gfi_data *) GGadgetGetUserData(g);
    int ret = false;

    if ( col==2 ) {
	if ( strings[3*row+2].u.md_str==NULL || *strings[3*row+2].u.md_str=='\0' ) {
	    GMatrixEditDeleteRow(g,row);
	    ret = true;
	}
    } else {
	if ( col==0 ) {
	    memset(found,0,sizeof(found));
	    found[ttf_idontknow] = true;	/* reserved name id */
	    for ( i=0; i<rows; ++i ) if ( i!=row ) {
		if ( strings[3*i].u.md_ival == strings[3*row].u.md_ival )	/* Same language */
		    found[strings[3*i+1].u.md_ival] = true;
	    }
	    if ( found[ strings[3*row+1].u.md_ival ] ) {
		/* This language already has an entry for this strid */
		/* pick another */
		if ( !found[ttf_subfamily] ) {
		    strings[3*row+1].u.md_ival = ttf_subfamily;
		    ret = true;
		} else {
		    for ( i=0; i<ttf_namemax; ++i )
			if ( !found[i] ) {
			    strings[3*row+1].u.md_ival = i;
			    ret = true;
		    break;
			}
		}
	    }
	}
	if ( (strings[3*row+2].u.md_str==NULL || *strings[3*row+2].u.md_str=='\0') ) {
	    for ( i=0; i<rows; ++i ) if ( i!=row ) {
		if ( strings[3*row+1].u.md_ival == strings[3*i+1].u.md_ival &&
			(strings[3*row].u.md_ival&0xff) == (strings[3*i].u.md_ival&0xff)) {
		    /* Same string, same language, different locale */
		    /* first guess is the same as the other string. */
		    if ( strings[3*i+2].u.md_str==NULL )
			strings[3*row+2].u.md_str = tn_recalculatedef(d,strings[3*row+1].u.md_ival );
		    else
			strings[3*row+2].u.md_str = copy(strings[3*i+2].u.md_str);
		    ret = true;
	    break;
		}
	    }
	    if ( i==rows ) {
	    /* If we didn't find anything above, and if we've got a style */
	    /*  (subfamily) see if we can guess a translation from the english */
		if ( strings[3*row+1].u.md_ival == ttf_subfamily )
		    ret |= CheckActiveStyleTranslation(d,strings,row,rows,false);
		else if ( strings[3*row+1].u.md_ival == ttf_wwssubfamily ) {
		    /* First see if there is a wwssubfamily we can translate */
		    /* then see if there is a subfamily we can copy */
		    ret |= CheckActiveStyleTranslation(d,strings,row,rows,true);
		} else if ( strings[3*row+1].u.md_ival == ttf_wwsfamily ) {
		    /* Copy the normal family */
		    for ( i=rows-1; i>=0; --i )
			if ( strings[3*i+1].u.md_ival==ttf_family &&
				strings[3*i].u.md_ival == strings[3*row].u.md_ival ) {
			    strings[3*row+2].u.md_str = copy(strings[3*i+2].u.md_str);
			    ret = true;
		    break;
			}
		    if ( (i<0 || strings[3*i+2].u.md_str==NULL) &&
			    strings[3*row].u.md_ival == 0x409 ) {
			strings[3*row+2].u.md_str = tn_recalculatedef(d,ttf_family);
			ret = true;
		    }
		}
	    }
	}
    }
    if ( ret )
	GGadgetRedraw(g);
}

static int TN_CanDelete(GGadget *g,int row) {
    int rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);
    if ( strings==NULL )
return( false );

return( !strings[3*row+2].user_bits );
}

static void TN_PopupMenu(GGadget *g,GEvent *event,int r,int c) {
    struct gfi_data *d = (struct gfi_data *) GGadgetGetUserData(g);
    int rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);
    GMenuItem mi[5];
    int i;

    if ( strings==NULL )
return;

    d->tn_active = r;

    memset(mi,'\0',sizeof(mi));
    for ( i=0; i<3; ++i ) {
	mi[i].ti.fg = COLOR_DEFAULT;
	mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].mid = i+1;
	mi[i].invoke = TN_StrPopupDispatch;
	mi[i].ti.text_is_1byte = true;
    }
    mi[MID_Delete-1].ti.disabled = strings[3*r+2].user_bits;
    mi[MID_ToggleBase-1].ti.disabled = !strings[3*r+2].user_bits;
    if ( strings[3*r+2].frozen ) {
	mi[MID_MultiEdit-1].ti.disabled = true;
	mi[MID_ToggleBase-1].ti.text = (unichar_t *) _("Detach from PostScript Names");
    } else {
	char *temp;
	mi[MID_ToggleBase-1].ti.text = (unichar_t *) _("Same as PostScript Names");
	temp = tn_recalculatedef(d,strings[3*r+1].u.md_ival);
	mi[MID_ToggleBase-1].ti.disabled = (temp==NULL);
	free(temp);
    }
    if ( c!=2 )
	mi[MID_MultiEdit-1].ti.disabled = true;
    mi[MID_MultiEdit-1].ti.text = (unichar_t *) _("Multi-line edit");
    mi[MID_Delete-1].ti.text = (unichar_t *) _("Delete");
    GMenuCreatePopupMenu(event->w,event, mi);
}

static int TN_PassChar(GGadget *g,GEvent *e) {
return( GFI_Char(GGadgetGetUserData(g),e));
}

static char *TN_BigEditTitle(GGadget *g,int r, int c) {
    char buf[100], buf2[20];
    const char *lang;
    int k;
    int rows;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);

    lang = langname(strings[3*r].u.md_ival,buf2);
    for ( k=0; ttfnameids[k].text!=NULL && ttfnameids[k].userdata!=(void *) (intpt) strings[3*r+1].u.md_ival;
	    ++k );
    snprintf(buf,sizeof(buf),_("%1$.30s string for %2$.30s"),
	    lang, (char *) ttfnameids[k].text );
return( copy( buf ));
}

static void TNMatrixInit(struct matrixinit *mi,struct gfi_data *d) {
    SplineFont *sf = d->sf;
    int i,j,k,cnt;
    uint8 sawEnglishUS[ttf_namemax];
    struct ttflangname *tln;
    struct matrix_data *md;

    d->langlocalecode = MSLanguageFromLocale();

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 3;
    mi->col_init = ci;

    md = NULL;
    for ( k=0; k<2; ++k ) {
	memset(sawEnglishUS,0,sizeof(sawEnglishUS));
	cnt = 0;
	for ( tln = sf->names; tln!=NULL; tln = tln->next ) {
	    for ( i=0; i<ttf_namemax; ++i ) if ( i!=ttf_postscriptname && tln->names[i]!=NULL ) {
		if ( md!=NULL ) {
		    md[3*cnt  ].u.md_ival = tln->lang;
		    md[3*cnt+1].u.md_ival = i;
		    md[3*cnt+2].u.md_str = copy(tln->names[i]);
		}
		++cnt;
		if ( tln->lang==0x409 )
		    sawEnglishUS[i] = true;
	    }
	}
	for ( i=0; ttfspecials[i]!=-1; ++i ) if ( !sawEnglishUS[ttfspecials[i]] ) {
	    if ( md!=NULL ) {
		md[3*cnt  ].u.md_ival = 0x409;
		md[3*cnt+1].u.md_ival = ttfspecials[i];
		md[3*cnt+2].u.md_str = NULL;
/* if frozen is set then can't remove or edit. (old basedon bit) */
		md[3*cnt].frozen = md[3*cnt+1].frozen = md[3*cnt+2].frozen = true;
/* if user_bits is set then can't remove. (old cantremove bit) */
		md[3*cnt].user_bits = md[3*cnt+1].user_bits = md[3*cnt+2].user_bits = true;
	    }
	    ++cnt;
	}
	if ( md==NULL )
	    md = calloc(3*(cnt+10),sizeof(struct matrix_data));
    }
    for ( i=0; i<cnt; ++i ) if ( md[3*cnt].u.md_ival==0x409 ) {
	for ( j=0; ttfspecials[j]!=-1 && ttfspecials[j]!=md[3*cnt+1].u.md_ival; ++j );
	md[3*i].user_bits = md[3*i+1].user_bits = md[3*i+2].user_bits =
		( ttfspecials[j]!=-1 );
    }
    mi->matrix_data = md;
    mi->initial_row_cnt = cnt;

    mi->initrow = TN_NewName;
    mi->finishedit = TN_FinishEdit;
    mi->candelete = TN_CanDelete;
    mi->popupmenu = TN_PopupMenu;
    mi->handle_key = TN_PassChar;
    mi->bigedittitle = TN_BigEditTitle;
}

static int GFI_SortBy(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = (struct gfi_data *) GDrawGetUserData(GGadgetGetWindow(g));
	TTFNames_Resort(d);
	GGadgetRedraw(GWidgetGetControl(d->gw,CID_TNames));
    }
return( true );
}

static int GFI_HelpOFL(GGadget *g, GEvent *e) {
/* F1 Help to open a browser to sil.org Open Source License and FAQ */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	help("http://scripts.sil.org/OFL");
    }
return( true );
}

static int GFI_AddOFL(GGadget *g, GEvent *e) {
/* Add sil.org Open Source License (see ofl.c), and modify with current date */
/* Author, and Font Family Name for rows[0,1] of the license. You can access */
/* this routine from GUI at Element->Font_Info->TTF_Names. info at PS_Names. */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *tng = GWidgetGetControl(GGadgetGetWindow(g),CID_TNames);
	int rows;
	struct matrix_data *tns, *newtns;
	int i,j,k,l,m, extras, len;
	char *all, *pt, **data;
	char buffer[1024], *bpt;
	const char *author = GetAuthor();
	char *reservedname, *fallback;
	time_t now;
	struct tm *tm;

	time(&now);
	tm = localtime(&now);

	tns = GMatrixEditGet(tng, &rows); newtns = NULL;
	for ( k=0; k<2; ++k ) {
	    extras = 0;
	    for ( i=0; ofl_str_lang_data[i].data!=NULL; ++i ) {
		for ( j=rows-1; j>=0; --j ) {
		    if ( tns[j*3+1].u.md_ival==ofl_str_lang_data[i].strid &&
			    tns[j*3+0].u.md_ival==ofl_str_lang_data[i].lang ) {
			if ( k ) {
			    free(newtns[j*3+2].u.md_str);
			    newtns[j*3+2].u.md_str = NULL;
			}
		break;
		    }
		}
		if ( j<0 )
		    j = rows + extras++;
		if ( k ) {
		    newtns[j*3+1].u.md_ival = ofl_str_lang_data[i].strid;
		    newtns[j*3+0].u.md_ival = ofl_str_lang_data[i].lang;
		    data = ofl_str_lang_data[i].data;
		    reservedname = fallback = NULL;
		    for ( m=0; m<rows; ++m ) {
			if ( newtns[j*3+1].u.md_ival==ttf_family ) {
			    if ( newtns[j*3+0].u.md_ival==0x409 )
				fallback = newtns[3*j+2].u.md_str;
			    else if ( newtns[j*3+0].u.md_ival==ofl_str_lang_data[i].lang )
				reservedname = newtns[3*j+2].u.md_str;
			}
		    }
		    if ( reservedname==NULL )
			reservedname = fallback;
		    if ( reservedname==NULL )
			reservedname = d->sf->familyname;
		    for ( m=0; m<2; ++m ) {
			len = 0;
			for ( l=0; data[l]!=NULL; ++l ) {
			    if ( l==0 ) {
				sprintf( buffer, data[l], tm->tm_year+1900, author );
			        bpt = buffer;
			    } else if ( l==1 ) {
				sprintf( buffer, data[l], reservedname );
			        bpt = buffer;
			    } else
				bpt = data[l];
			    if ( m ) {
				strcpy( pt, bpt );
			        pt += strlen( bpt );
			        *pt++ = '\n';
			    } else
				len += strlen( bpt ) + 1;		/* for a new line */
			}
			if ( !m )
			    newtns[j*3+2].u.md_str = all = pt = malloc(len+2);
		    }
		    if ( pt>all ) pt[-1] = '\0';
		    else *pt = '\0';
		}
	    }
	    if ( !k ) {
		newtns = calloc((rows+extras)*3,sizeof(struct matrix_data));
		memcpy(newtns,tns,rows*3*sizeof(struct matrix_data));
		for ( i=0; i<rows; ++i )
		    newtns[3*i+2].u.md_str = copy(newtns[3*i+2].u.md_str);
	    }
	}
	GMatrixEditSet(tng, newtns, rows+extras, false);
	ff_post_notice(_("Using the OFL for your open fonts"),_(
	    "The OFL is a community-approved software license designed for libre/open font projects. \n"
	    "Fonts under the OFL can be used, studied, copied, modified, embedded, merged and redistributed while giving authors enough control and artistic integrity. For more details including an FAQ see http://scripts.sil.org/OFL. \n\n"
	    "This font metadata will help users, designers and distribution channels to know who you are, how to contact you and what rights you are granting. \n" 
        "When releasing modified versions, remember to add your additional notice, including any extra Reserved Font Name(s). \n\n" 
	    "Have fun designing open fonts!" ));
    }
return( true );
}

static int ss_cmp(const void *_md1, const void *_md2) {
    const struct matrix_data *md1 = _md1, *md2 = _md2;

    char buf1[20], buf2[20];
    const char *l1, *l2;

    if ( md1[1].u.md_ival == md2[1].u.md_ival ) {
       l1 = langname(md1[0].u.md_ival,buf1);
       l2 = langname(md2[0].u.md_ival,buf2);
return( strcoll(l1,l2));
    }
return( md1[1].u.md_ival - md2[1].u.md_ival );
}

static void SSMatrixInit(struct matrixinit *mi,struct gfi_data *d) {
    SplineFont *sf = d->sf;
    struct matrix_data *md;
    struct otffeatname *fn;
    struct otfname *on;
    int cnt;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 3;
    mi->col_init = ssci;

    for ( cnt=0, fn=sf->feat_names; fn!=NULL; fn=fn->next ) {
	for ( on=fn->names; on!=NULL; on=on->next, ++cnt );
    }
    md = calloc(3*(cnt+10),sizeof(struct matrix_data));
    for ( cnt=0, fn=sf->feat_names; fn!=NULL; fn=fn->next ) {
	for ( on=fn->names; on!=NULL; on=on->next, ++cnt ) {
	    md[3*cnt  ].u.md_ival = on->lang;
	    md[3*cnt+1].u.md_ival = fn->tag;
	    md[3*cnt+2].u.md_str = copy(on->name);
	}
    }
    qsort( md, cnt, 3*sizeof(struct matrix_data), ss_cmp );
    mi->matrix_data = md;
    mi->initial_row_cnt = cnt;
}

static int size_cmp(const void *_md1, const void *_md2) {
    const struct matrix_data *md1 = _md1, *md2 = _md2;

    char buf1[20], buf2[20];
    const char *l1, *l2;

   l1 = langname(md1[0].u.md_ival,buf1);
   l2 = langname(md2[0].u.md_ival,buf2);
return( strcoll(l1,l2));
}

static void SizeMatrixInit(struct matrixinit *mi,struct gfi_data *d) {
    SplineFont *sf = d->sf;
    struct matrix_data *md;
    struct otfname *on;
    int cnt;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 2;
    mi->col_init = sizeci;

    for ( cnt=0, on=sf->fontstyle_name; on!=NULL; on=on->next )
	++cnt;
    md = calloc(2*(cnt+10),sizeof(struct matrix_data));
    for ( cnt=0, on=sf->fontstyle_name; on!=NULL; on=on->next, ++cnt ) {
	md[2*cnt  ].u.md_ival = on->lang;
	md[2*cnt+1].u.md_str = copy(on->name);
    }
    qsort( md, cnt, 2*sizeof(struct matrix_data), size_cmp );
    mi->matrix_data = md;
    mi->initial_row_cnt = cnt;
}

static int Gasp_Default(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *gg = GWidgetGetControl(GGadgetGetWindow(g),CID_Gasp);
	int rows;
	struct matrix_data *gasp;

	if ( !SFHasInstructions(d->sf)) {
	    rows = 1;
	    gasp = calloc(rows*5,sizeof(struct matrix_data));
	    gasp[0].u.md_ival = 65535;
	    gasp[1].u.md_ival = 0;	/* no grid fit (we have no instructions, we can't grid fit) */
	    gasp[2].u.md_ival = 1;	/* do anti-alias */
	    gasp[3].u.md_ival = 0;	/* do symmetric smoothing */
	    gasp[4].u.md_ival = 0;	/* do no grid fit w/ sym smooth */
	} else {
	    rows = 3;
	    gasp = calloc(rows*5,sizeof(struct matrix_data));
	    gasp[0].u.md_ival = 8;     gasp[1].u.md_ival = 0; gasp[2].u.md_ival = 1;
		    gasp[3].u.md_ival = 0; gasp[4].u.md_ival = 0;
	    gasp[5].u.md_ival = 16;    gasp[6].u.md_ival = 1; gasp[7].u.md_ival = 0;
		    gasp[8].u.md_ival = 0; gasp[9].u.md_ival = 0;
	    gasp[10].u.md_ival = 65535; gasp[11].u.md_ival = 1; gasp[12].u.md_ival = 1;
		    gasp[13].u.md_ival = 0; gasp[14].u.md_ival = 0;
	}
	GMatrixEditSet(gg,gasp,rows,false);
    }
return( true );
}

static int Gasp_CanDelete(GGadget *g,int row) {
    int rows;
    struct matrix_data *gasp = GMatrixEditGet(g, &rows);
    if ( gasp==NULL )
return( false );

    /* Only allow them to delete the sentinal entry if that would give us an */
    /* empty gasp table */
return( gasp[5*row].u.md_ival!=0xffff || rows==1 );
}

static int gasp_comp(const void *_md1, const void *_md2) {
    const struct matrix_data *md1 = _md1, *md2 = _md2;
return( md1->u.md_ival - md2->u.md_ival );
}

static void Gasp_FinishEdit(GGadget *g,int row,int col,int wasnew) {
    int rows;
    struct matrix_data *gasp = GMatrixEditGet(g, &rows);

    if ( col==0 ) {
	qsort(gasp,rows,5*sizeof(struct matrix_data),gasp_comp);
	GGadgetRedraw(g);
    }
}

static void GaspMatrixInit(struct matrixinit *mi,struct gfi_data *d) {
    SplineFont *sf = d->sf;
    int i;
    struct matrix_data *md;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 5;
    mi->col_init = gaspci;

    if ( sf->gasp_cnt==0 ) {
	md = calloc(5,sizeof(struct matrix_data));
	mi->initial_row_cnt = 0;
    } else {
	md = calloc(5*sf->gasp_cnt,sizeof(struct matrix_data));
	for ( i=0; i<sf->gasp_cnt; ++i ) {
	    md[5*i  ].u.md_ival = sf->gasp[i].ppem;
	    md[5*i+1].u.md_ival = (sf->gasp[i].flags&1)?1:0;
	    md[5*i+2].u.md_ival = (sf->gasp[i].flags&2)?1:0;
	    md[5*i+3].u.md_ival = (sf->gasp[i].flags&8)?1:0;
	    md[5*i+4].u.md_ival = (sf->gasp[i].flags&4)?1:0;
	}
	mi->initial_row_cnt = sf->gasp_cnt;
    }
    mi->matrix_data = md;

    mi->finishedit = Gasp_FinishEdit;
    mi->candelete = Gasp_CanDelete;
    mi->handle_key = TN_PassChar;
}

static int GFI_GaspVersion(GGadget *g, GEvent *e) {
    if ( e->u.control.subtype == et_listselected ) {
	int version = GGadgetGetFirstListSelectedItem(g);
	GGadget *gasp = GWidgetGetControl(GGadgetGetWindow(g),CID_Gasp);
	if ( version == 0 ) {
	    GMatrixEditShowColumn(gasp,3,false);
	    GMatrixEditShowColumn(gasp,4,false);
	} else {
	    GMatrixEditShowColumn(gasp,3,true);
	    GMatrixEditShowColumn(gasp,4,true);
	}
	GGadgetRedraw(gasp);
    }
return( true );
}

static void Layers_BackgroundEnable(GGadget *g,GMenuItem *mi, int r, int c) {
    int disable = r<=ly_fore;
    mi[0].ti.disabled = disable;
    mi[1].ti.disabled = disable;
}

static int Layers_CanDelete(GGadget *g,int row) {
return( row>ly_fore );
}

static void Layers_InitRow(GGadget *g,int row) {
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *layers = GMatrixEditGet(g, &rows);
    int isquadratic = GGadgetIsChecked(GWidgetGetControl(GGadgetGetWindow(g),CID_IsOrder2));

    layers[row*cols+1].u.md_ival = isquadratic;
}

static void LayersMatrixInit(struct matrixinit *mi,struct gfi_data *d) {
    SplineFont *sf = d->sf;
    int i,j;
    struct matrix_data *md;

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 4;
    mi->col_init = layersci;

    md = calloc(4*(sf->layer_cnt+1),sizeof(struct matrix_data));
    for ( i=j=0; i<sf->layer_cnt; ++i ) {
	md[4*j  ].u.md_str  = copy(sf->layers[i].name);
	md[4*j+1].u.md_ival = sf->layers[i].order2;
	md[4*j+2].u.md_ival = sf->layers[i].background;
	md[4*j+3].u.md_ival = i+1;
	++j;
    }
    mi->initial_row_cnt = sf->layer_cnt;
    mi->matrix_data = md;

    mi->initrow   = Layers_InitRow;
    mi->candelete = Layers_CanDelete;
}

static int GFI_Type3Change(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	int type3 = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsMultiLayer));
	int mixed = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsMixed));
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_IsMixed), !type3);
	if ( type3 )
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_IsMixed), false );
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_IsOrder2), !type3);
	if ( type3 )
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_IsOrder2), false );
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_IsOrder3), !type3);
	if ( type3 )
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_IsOrder3), true );
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_GuideOrder2), !type3 && mixed);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_Backgrounds), !type3);
    }
return( true );
}

static int GFI_OrderChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadget *backs = GWidgetGetControl(gw,CID_Backgrounds);
	int mixed = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsMixed));
	int cubic = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsOrder3));
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_IsMultiLayer), cubic);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_GuideOrder2), mixed);
	if ( !mixed ) {
	    GGadgetSetChecked(GWidgetGetControl(gw,CID_GuideOrder2), !cubic );
	}
	GGadgetSetEnabled(backs, true);
	GMatrixEditEnableColumn(backs, 1, mixed);
	if ( !mixed ) {
	    int col = GMatrixEditGetColCnt(backs), rows, i;
	    struct matrix_data *md = GMatrixEditGet(backs, &rows);
	    for ( i=0; i<rows; ++i )
		md[i*col+1].u.md_ival = !cubic;
	}
	GGadgetRedraw(backs);
    }
return( true );
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
    N_("The amount of stretchable space between words when using this font"),
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
    else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMathSym)) )
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
	GGadget *edit = GWidgetGetControl(d->gw,CID_TNames);
	int rows;
	struct matrix_data *strings = GMatrixEditGet(edit, &rows);
	for ( i=0; i<rows; ++i )
	    if ( strings[3*i+1].u.md_ival==ttf_uniqueid ) {
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
	GGadget *edit = GWidgetGetControl(d->gw,CID_TNames);
	int rows;
	struct matrix_data *strings = GMatrixEditGet(edit, &rows);

	for ( tln = sf->names; tln!=NULL; tln=tln->next ) {
	    if ( tln->names[ttf_uniqueid]==NULL )
	continue;		/* Not set, so if it has been given a new value */
				/*  that's a change, and if it hasn't that's ok */
	    for ( i=0; i<rows; ++i )
		if ( strings[3*i+1].u.md_ival==ttf_uniqueid && strings[3*i].u.md_ival==tln->lang )
	    break;
	    if ( i==rows )
	continue;		/* removed. That's a change */
	    if ( strcmp(tln->names[ttf_uniqueid],strings[3*i+2].u.md_str )==0 )
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
	GGadget *edit = GWidgetGetControl(d->gw,CID_TNames);
	int rows;
	struct matrix_data *strings = GMatrixEditGet(edit, &rows);

	/* see if any instances of the name have changed */
	for ( tln = sf->names; tln!=NULL; tln=tln->next ) {
	    if ( tln->names[ttf_uniqueid]==NULL )
	continue;
	    for ( i=0; i<rows; ++i )
		if ( strings[3*i+1].u.md_ival==ttf_uniqueid && strings[3*i].u.md_ival==tln->lang )
	    break;
	    if ( i==rows )
	continue;
	    if ( strcmp(tln->names[ttf_uniqueid],strings[3*i+2].u.md_str )!=0 )
		changed = copy(strings[3*i+2].u.md_str );
	break;
	}
	/* All unique ids should be the same, if any changed set the unchanged */
	/*  ones to the one that did (or the first of many if several changed) */
	for ( tln = sf->names; tln!=NULL; tln=tln->next ) {
	    if ( tln->names[ttf_uniqueid]==NULL )
	continue;
	    for ( i=0; i<rows; ++i )
		if ( strings[3*i+1].u.md_ival==ttf_uniqueid && strings[3*i].u.md_ival==tln->lang )
	    break;
	    if ( i==rows )
	continue;
	    if ( strcmp(tln->names[ttf_uniqueid],strings[3*i+2].u.md_str)==0 ) {
		free(strings[3*i+2].u.md_str);
		strings[3*i+2].u.md_str = changed!=NULL
			? copy( changed )
			: NULL;
	    }
	}
    }
}

static void StoreTTFNames(struct gfi_data *d) {
    struct ttflangname *tln;
    SplineFont *sf = d->sf;
    int i;
    GGadget *edit = GWidgetGetControl(d->gw,CID_TNames);
    int rows;
    struct matrix_data *strings = GMatrixEditGet(edit, &rows);

    TTFLangNamesFree(sf->names); sf->names = NULL;

    for ( i=0; i<rows; ++i ) {
	for ( tln=sf->names; tln!=NULL && tln->lang!=strings[3*i].u.md_ival; tln=tln->next );
	if ( tln==NULL ) {
	    tln = chunkalloc(sizeof(struct ttflangname));
	    tln->lang = strings[3*i].u.md_ival;
	    tln->next = sf->names;
	    sf->names = tln;
	}
	tln->names[strings[3*i+1].u.md_ival] = copy(strings[3*i+2].u.md_str );
    }

    TTF_PSDupsDefault(sf);
}

static int SSNameValidate(struct gfi_data *d) {
    GGadget *edit = GWidgetGetControl(d->gw,CID_SSNames);
    int rows;
    struct matrix_data *strings = GMatrixEditGet(edit, &rows);
    int i, j, k;

    for ( i=0; i<rows; ++i ) {
	if ( strings[3*i+2].u.md_str == NULL )
    continue;
	for ( j=i+1; j<rows; ++j ) {
	    if ( strings[3*j+2].u.md_str == NULL )
	continue;
	    if ( strings[3*i  ].u.md_ival == strings[3*j  ].u.md_ival &&
		    strings[3*i+1].u.md_ival == strings[3*j+1].u.md_ival ) {
		uint32 tag = strings[3*i+1].u.md_ival;
		for ( k=0; mslanguages[k].text!=NULL &&
			((intpt) mslanguages[k].userdata)!=strings[3*i].u.md_ival; ++k );
		if ( mslanguages[k].text==NULL ) k=0;
		ff_post_error(_("Duplicate StyleSet Name"),_("The feature '%c%c%c%c' is named twice in language %s\n%.80s\n%.80s"),
			tag>>24, tag>>16, tag>>8, tag,
			mslanguages[k].text,
			strings[3*i+2].u.md_str,
			strings[3*j+2].u.md_str
			);
return( false );
	    }
	}
    }
return( true );
}

static void StoreSSNames(struct gfi_data *d) {
    GGadget *edit = GWidgetGetControl(d->gw,CID_SSNames);
    int rows;
    struct matrix_data *strings = GMatrixEditGet(edit, &rows);
    int i;
    uint32 tag, lang;
    struct otffeatname *fn;
    struct otfname *on;
    SplineFont *sf = d->sf;

    OtfFeatNameListFree(sf->feat_names);
    sf->feat_names = NULL;

    qsort( strings, rows, 3*sizeof(struct matrix_data), ss_cmp );
    for ( i=rows-1; i>=0; --i ) {
	if ( strings[3*i+2].u.md_str == NULL )
    continue;
	tag = strings[3*i+1].u.md_ival;
	lang = strings[3*i].u.md_ival;
	for ( fn=sf->feat_names; fn!=NULL && fn->tag!=tag; fn=fn->next );
	if ( fn==NULL ) {
	    fn = chunkalloc(sizeof(*fn));
	    fn->tag = tag;
	    fn->next = sf->feat_names;
	    sf->feat_names = fn;
	}
	on = chunkalloc(sizeof(*on));
	on->next = fn->names;
	fn->names = on;
	on->lang = lang;
	on->name = copy(strings[3*i+2].u.md_str );
    }
}

static void GFI_ApplyLookupChanges(struct gfi_data *gfi) {
    int i,j, isgpos;
    OTLookup *last;
    SplineFont *sf = gfi->sf;
    struct lookup_subtable *sublast;

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	struct lkdata *lk = &gfi->tables[isgpos];
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
		SFRemoveLookup(gfi->sf,lk->all[i].lookup,0);
	    else for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
		    SFRemoveLookupSubTable(gfi->sf,lk->all[i].subtables[j].subtable,0);
	    }
	}
	last = NULL;
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( !lk->all[i].deleted ) {
		if ( last!=NULL )
		    last->next = lk->all[i].lookup;
		else if ( isgpos )
		    sf->gpos_lookups = lk->all[i].lookup;
		else
		    sf->gsub_lookups = lk->all[i].lookup;
		last = lk->all[i].lookup;
		sublast = NULL;
		for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		    if ( !lk->all[i].subtables[j].deleted ) {
			if ( sublast!=NULL )
			    sublast->next = lk->all[i].subtables[j].subtable;
			else
			    last->subtables = lk->all[i].subtables[j].subtable;
			sublast = lk->all[i].subtables[j].subtable;
			sublast->lookup = last;
		    }
		}
		if ( sublast!=NULL )
		    sublast->next = NULL;
		else
		    last->subtables = NULL;
	    }
	    free(lk->all[i].subtables);
	}
	if ( last!=NULL )
	    last->next = NULL;
	else if ( isgpos )
	    sf->gpos_lookups = NULL;
	else
	    sf->gsub_lookups = NULL;
	free(lk->all);
    }
}

static void hexparse(GWindow gw, int cid, char *name, uint32 *data, int len, int *err) {
    int i;
    const unichar_t *ret;
    unichar_t *end;

    ret = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    end = (unichar_t *) ret;
    for ( i=0; i<len; ++i ) {
	if ( i!=0 ) {
	    if ( *end=='.' )
		++end;
	    else {
		*err = true;
		ff_post_error(_("Bad hex number"), _("Bad hex number in %s"), name);
return;
	    }
	}
	data[len-1-i] = u_strtoul(end,&end,16);
    }
    if ( *end!='\0' ) {
	*err = true;
	ff_post_error(_("Bad hex number"), _("Bad hex number in %s"), name);
return;
    }
}

static int GFI_AnyLayersRemoved(struct gfi_data *d) {
    GGadget *backs = GWidgetGetControl(d->gw,CID_Backgrounds);
    int rows, cols = GMatrixEditGetColCnt(backs), r, origr, l;
    struct matrix_data *layers = GMatrixEditGet(backs, &rows);
    SplineFont *sf = d->sf;
    int anytrue_before, anytrue_after;

    for ( l=0; l<sf->layer_cnt; ++l )
	sf->layers[l].ticked = false;

    anytrue_before = anytrue_after = false;
    for ( r=ly_fore; r<sf->layer_cnt; ++r ) {
	if ( sf->layers[r].order2 )
	    anytrue_before = true;
    }
    for ( r=0; r<rows; ++r ) if ( (origr = layers[r*cols+3].u.md_ival)!=0 ) {
	--origr;
	sf->layers[origr].ticked = true;
	if ( origr>=ly_fore && layers[r*cols+1].u.md_ival )
	    anytrue_after = true;
    }

    for ( l=sf->layer_cnt-1; l>ly_fore; --l ) if ( !sf->layers[l].ticked )
return( 1 | (anytrue_before && !anytrue_after? 2 : 0) );

return( 0 | (anytrue_before && !anytrue_after? 2 : 0) );
}

static int GFI_SetLayers(struct gfi_data *d) {
    GGadget *backs = GWidgetGetControl(d->gw,CID_Backgrounds);
    int rows, cols = GMatrixEditGetColCnt(backs), r, origr, l;
    struct matrix_data *layers = GMatrixEditGet(backs, &rows);
    SplineFont *sf = d->sf;
    int changed = false;

    for ( l=0; l<sf->layer_cnt; ++l )
	sf->layers[l].ticked = false;

    for ( r=0; r<rows; ++r ) if ( (origr = layers[r*cols+3].u.md_ival)!=0 ) {
	/* It's an old layer. Do we need to change anything? */
	--origr;
	sf->layers[origr].ticked = true;
	if ( sf->layers[origr].order2 != layers[r*cols+1].u.md_ival ) {
	    if ( layers[r*cols+1].u.md_ival )
		SFConvertLayerToOrder2(sf,origr);
	    else
		SFConvertLayerToOrder3(sf,origr);
	    changed = true;
	}
	if ( sf->layers[origr].background != layers[r*cols+2].u.md_ival ) {
	    SFLayerSetBackground(sf,origr,layers[r*cols+2].u.md_ival);
	    changed = true;
	}
	if ( layers[r*cols+0].u.md_str!=NULL && *layers[r*cols+0].u.md_str!='\0' &&
		strcmp( sf->layers[origr].name, layers[r*cols+0].u.md_str)!=0 ) {
	    free( sf->layers[origr].name );
	    sf->layers[origr].name = copy( layers[r*cols+0].u.md_str );
	    changed = true;
	}
    }

    for ( l=sf->layer_cnt-1; l>ly_fore; --l ) if ( !sf->layers[l].ticked ) {
	SFRemoveLayer(sf,l);
	changed = true;
    }

    l = 0;
    for ( r=0; r<rows; ++r ) if ( layers[r*cols+3].u.md_ival==0 ) {
	SFAddLayer(sf,layers[r*cols+0].u.md_str,layers[r*cols+1].u.md_ival, layers[r*cols+2].u.md_ival);
	changed = true;
    }
    if ( changed )
	CVLayerPaletteCheck(sf);
return( changed );
}


char* DumpSplineFontMetadata(SplineFont *sf) {
    FILE *sfd;
    if ( (sfd=MakeTemporaryFile()) ) {
	SFD_DumpSplineFontMetadata( sfd, sf );
	char *str = FileToAllocatedString( sfd );
	fclose(sfd);
	return( str );
    }
    return( 0 );
}

static int GFI_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
    {
	GWindow gw = GGadgetGetWindow(g);
	struct gfi_data *d = GDrawGetUserData(gw);
	SplineFont *sf = d->sf, *_sf;

	printf("at top of GFI_OK\n");

	char* sfdfrag  = DumpSplineFontMetadata( sf );
	SFUndoes* undo = SFUndoCreateSFD( sfut_fontinfo,
					  _("Font Information Dialog"),
					  sfdfrag );
	SFUndoPushFront( &sf->undoes, undo );



	int interp;
	int reformat_fv=0, retitle_fv=false;
	int upos, uwid, as, des, err = false, weight=0;
	int uniqueid, linegap=0, vlinegap=0, winascent=0, windescent=0;
	int tlinegap=0, tascent=0, tdescent=0, hascent=0, hdescent=0;
	int capheight=0, xheight=0;
	int winaoff=true, windoff=true;
	int taoff=true, tdoff=true, haoff = true, hdoff = true;
	real ia, cidversion;
	const unichar_t *txt, *fond; unichar_t *end;
	int i,j, mcs;
	int vmetrics, namechange, guideorder2;
	int xuidchanged = false, usexuid, useuniqueid;
	GTextInfo *pfmfam, *ibmfam, *fstype, *nlitem, *stylemap;
	int32 len;
	GTextInfo **ti;
	int subs[4], super[4], strike[2];
	struct otfname *fontstyle_name;
	int design_size, size_top, size_bottom, styleid;
	int strokedfont = false;
	real strokewidth;
	int multilayer = false;
	char os2_vendor[4];
	NameList *nl;
	extern int allow_utf8_glyphnames;
	int os2version;
	int rows, gasprows, mc_rows, ms_rows;
	struct matrix_data *strings     = GMatrixEditGet(GWidgetGetControl(d->gw,CID_TNames), &rows);
	struct matrix_data *gasp        = GMatrixEditGet(GWidgetGetControl(d->gw,CID_Gasp), &gasprows);
	struct matrix_data *markclasses = GMatrixEditGet(GWidgetGetControl(d->gw,CID_MarkClasses), &mc_rows);
	struct matrix_data *marksets    = GMatrixEditGet(GWidgetGetControl(d->gw,CID_MarkSets), &ms_rows);
	int was_ml = sf->multilayer, was_stroke = sf->strokedfont;
	uint32 codepages[2], uranges[4];
	int layer_cnt;
	struct matrix_data *layers = GMatrixEditGet(GWidgetGetControl(d->gw,CID_Backgrounds), &layer_cnt);
	int layer_flags;
	int sfntRevision=sfntRevisionUnset, woffMajor=woffUnset, woffMinor=woffUnset;

	if ( strings==NULL || gasp==NULL || layers==NULL )
return( true );
	if ( gasprows>0 && gasp[5*gasprows-5].u.md_ival!=65535 ) {
	    ff_post_error(_("Bad Grid Fitting table"),_("The 'gasp' (Grid Fit) table must end with a pixel entry of 65535"));
return( true );
	}
	{
	    int i;
	    static struct {
		int cid;
		char *tit, *msg;
	    } msgs[] = {
		{ CID_Notice, N_("Bad Copyright"), NU_("Copyright text (in the Names pane) must be entirely ASCII. So, use (c) instead of ©.")},
		{ CID_Human, N_("Bad Human Fontname"), N_("The human-readable fontname text (in the Names pane) must be entirely ASCII.")},
		{ CID_Weight, N_("Bad Weight"), N_("The weight text (in the Names pane) must be entirely ASCII.")},
		{ CID_Version, N_("Bad Version"), N_("The version text (in the Names pane) must be entirely ASCII.")},
		{ 0, NULL, NULL }
	    };
	    for ( i = 0; msgs[i].cid!=0 ; ++i )
		    if ( !uAllAscii(_GGadgetGetTitle(GWidgetGetControl(d->gw,msgs[i].cid))) ) {
		ff_post_error(_(msgs[i].tit),_(msgs[i].msg));
return( true );
	    }
	}
	if ( (layer_flags = GFI_AnyLayersRemoved(d))!=0 ) {
	    char *buts[3];
	    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
	    if ( layer_flags & 1 ) {
		if ( gwwv_ask(_("Deleting a layer cannot be UNDONE!"),(const char **) buts,0,1,_(
			"You are about to delete a layer.\n"
			"This will lose all contours in that layer.\n"
			"If this is the last quadratic layer it will\n"
			"lose all truetype instructions.\n\n"
			"Deleting a layer cannot be undone.\n\n"
			"Is this really your intent?"))==1 )
return( true );
	    } else if ( layer_flags & 2 ) {
		if ( gwwv_ask(_("Removing instructions cannot be UNDONE!"),(const char **) buts,0,1,_(
			"You are about to change the last quadratic\n"
			"layer to cubic. When this happens FontForge\n"
			"will remove all truetype instructions.\n\n"
			"This cannot be undone.\n\n"
			"Is this really your intent?"))==1 )
return( true );
	    }
	}
	if ( layer_cnt>=BACK_LAYER_MAX-2 ) {
	    ff_post_error(_("Too many layers"),_("FontForge supports at most %d layers"),BACK_LAYER_MAX-2);
	    /* This can be increased in configure-fontforge.h */
return( true );
	}
	if ( !CheckNames(d))
return( true );

	if ( ttfmultuniqueids(sf,d)) {
	    char *buts[3];
	    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2]=NULL;
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
	    GGadgetProtest8(_("_Italic Angle:"));
return(true);
	}
	guideorder2 = GGadgetIsChecked(GWidgetGetControl(gw,CID_GuideOrder2));
	strokedfont = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsStrokedFont));
	strokewidth = GetReal8(gw,CID_StrokeWidth,_("Stroke _Width:"),&err);
	multilayer = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsMultiLayer));
	vmetrics = GGadgetIsChecked(GWidgetGetControl(gw,CID_HasVerticalMetrics));
	upos = GetReal8(gw,CID_UPos, _("Underline _Position:"),&err);
	uwid = GetReal8(gw,CID_UWidth,S_("Underline|_Height:"),&err);
	GetInt8(gw,CID_Em,_("_Em Size:"),&err);	/* just check for errors. redundant info */
	as = GetInt8(gw,CID_Ascent,_("_Ascent:"),&err);
	des = GetInt8(gw,CID_Descent,_("_Descent:"),&err);
	uniqueid = GetInt8(gw,CID_UniqueID,_("UniqueID"),&err);
	design_size = rint(10*GetReal8(gw,CID_DesignSize,_("De_sign Size:"),&err));
	size_bottom = rint(10*GetReal8(gw,CID_DesignBottom,_("_Bottom"),&err));
	size_top = rint(10*GetReal8(gw,CID_DesignTop,_("_Top"),&err));
	styleid = GetInt8(gw,CID_StyleID,_("Style _ID:"),&err);
	fontstyle_name = OtfNameFromStyleNames(GWidgetGetControl(gw,CID_StyleName));
	OtfNameListFree(fontstyle_name);
	if ( design_size==0 && ( size_bottom!=0 || size_top!=0 || styleid!=0 || fontstyle_name!=NULL )) {
	    ff_post_error(_("Bad Design Size Info"),_("If the design size is 0, then all other fields on that pane must be zero (or unspecified) too."));
return( true );
	} else if ( styleid!=0 && fontstyle_name==NULL ) {
	    ff_post_error(_("Bad Design Size Info"),_("If you specify a style id for the design size, then you must specify a style name"));
return( true );
	} else if ( fontstyle_name!=NULL && styleid==0 ) {
	    ff_post_error(_("Bad Design Size Info"),_("If you specify a style name for the design size, then you must specify a style id"));
return( true );
	} else if ( design_size<0 ) {
	    ff_post_error(_("Bad Design Size Info"),_("If you specify a design size, it must be positive"));
return( true );
	} else if ( size_bottom!=0 && size_bottom>design_size ) {
	    ff_post_error(_("Bad Design Size Info"),_("In the design size range, the bottom field must be less than the design size."));
return( true );
	} else if ( size_top!=0 && size_top<design_size ) {
	    ff_post_error(_("Bad Design Size Info"),_("In the design size range, the bottom top must be more than the design size."));
return( true );
	} else if ( styleid!=0 && size_top==0 ) {
	    ff_post_error(_("Bad Design Size Info"),_("If you specify a style id for the design size, then you must specify a size range"));
return( true );
	} else if ( size_top!=0 && styleid==0 ) {
	    ff_post_notice(_("Bad Design Size Info"),_("If you specify a design size range, then you are supposed to specify a style id and style name too. FontForge will allow you to leave those fields blank, but other applications may not."));
	    /* no return, this is just a warning */
	}

	if ( *_GGadgetGetTitle(GWidgetGetControl(gw,CID_Revision))!='\0' )
	    sfntRevision = rint(65536.*GetReal8(gw,CID_Revision,_("sfnt Revision:"),&err));
	if ( *_GGadgetGetTitle(GWidgetGetControl(gw,CID_WoffMajor))!='\0' ) {
	    woffMajor = GetInt8(gw,CID_WoffMajor,_("Woff Major Version:"),&err);
	    woffMinor = 0;
	    if ( *_GGadgetGetTitle(GWidgetGetControl(gw,CID_WoffMinor))!='\0' )
		woffMinor = GetInt8(gw,CID_WoffMinor,_("Woff Minor Version:"),&err);
	}
	if ( err )
return(true);

	memset(codepages,0,sizeof(codepages));
	memset(uranges,0,sizeof(uranges));
	if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_CPageDefault)) )
	    hexparse(gw,CID_CodePageRanges,_("MS Code Pages"),codepages,2,&err );
	if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_URangesDefault)) )
	    hexparse(gw,CID_UnicodeRanges,_("Unicode Ranges"),uranges,4,&err );
	if ( err )
return( true );

	if ( sf->subfontcnt!=0 ) {
	    cidversion = GetReal8(gw,CID_Version,_("_Version"),&err);
	    if ( err )
return(true);
	}
	os2version = sf->os2_version;
	if ( d->ttf_set ) {
	    char *os2v = GGadgetGetTitle8(GWidgetGetControl(gw,CID_OS2Version));
	    if ( strcasecmp(os2v,(char *) os2versions[0].text )== 0 )
		os2version = 0;
	    else
		os2version = GetInt8(gw,CID_OS2Version,_("Weight, Width, Slope Only"),&err);
	    free(os2v);
	    /* Only use the normal routine if we get no value, because */
	    /*  "400 Book" is a reasonable setting, but would cause GetInt */
	    /*  to complain */
	    weight = u_strtol(_GGadgetGetTitle(GWidgetGetControl(gw,CID_WeightClass)),NULL,10);
	    if ( weight == 0 ) {
		int i;
		char *wc = GGadgetGetTitle8(GWidgetGetControl(gw,CID_WeightClass));
		for ( i=0; widthclass[i].text!=NULL; ++i ) {
		    if ( strcmp(wc,(char *) widthclass[i].text)==0 ) {
			weight = (intpt) widthclass[i].userdata;
		break;
		    }
		}
		free(wc);
	    }
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
	    capheight = GetInt8(gw,CID_CapHeight, _("Ca_pital Height:"),&err);
	    xheight = GetInt8(gw,CID_XHeight, _("_X Height:"),&err);
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
		    ff_post_error(_("Bad IBM Family"),_("Tag must be 4 characters long"));
		else
		    ff_post_error(_("Bad IBM Family"),_("A tag must be 4 ASCII characters"));
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
	    ff_post_error(_("Bad Ascent/Descent"),_("Ascent and Descent must be positive and their sum less than 16384"));
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
		ff_post_error(_("Bad Style"),_("A style may not have both condense and extend set (it makes no sense)"));
return( true );
	    }
	}

	nlitem = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_Namelist));
	if ( nlitem==NULL )
	    nl = DefaultNameListForNewFonts();
	else {
	    char *name = u2utf8_copy(nlitem->text);
	    nl = NameListByName(name);
	    free(name);
	}
	if ( nl->uses_unicode && !allow_utf8_glyphnames ) {
	    ff_post_error(_("Namelist contains non-ASCII names"),_("Glyph names should be limited to characters in the ASCII character set,\nbut there are names in this namelist which use characters outside\nthat range."));
return(true);
	}
	if ( !SSNameValidate(d))
return( true );
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
	sf->strokedfont = strokedfont;
	sf->strokewidth = strokewidth;
	GDrawSetCursor(gw,ct_watch);
	namechange = SetFontName(gw,sf);
	if ( namechange ) retitle_fv = true;
	usexuid = GGadgetIsChecked(GWidgetGetControl(gw,CID_UseXUID));
	useuniqueid = GGadgetIsChecked(GWidgetGetControl(gw,CID_UseUniqueID));
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_XUID));
	xuidchanged = (sf->xuid==NULL && *txt!='\0') ||
			(sf->xuid!=NULL && uc_strcmp(txt,sf->xuid)==0);
	if ( namechange &&
		((uniqueid!=0 && uniqueid==sf->uniqueid && useuniqueid) ||
		 (sf->xuid!=NULL && uc_strcmp(txt,sf->xuid)==0 && usexuid) ||
		 ttfuniqueidmatch(sf,d)) ) {
	    char *buts[4];
	    int ans;
	    buts[0] = _("Change");
	    buts[1] = _("Retain");
	    buts[2] = _("_Cancel");
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
	sf->use_xuid = usexuid;
	sf->use_uniqueid = useuniqueid;

	sf->sfntRevision = sfntRevision;

	free(sf->woffMetadata);
	sf->woffMetadata = NULL;
	if ( *_GGadgetGetTitle(GWidgetGetControl(gw,CID_WoffMetadata))!='\0' )
	    sf->woffMetadata = GGadgetGetTitle8(GWidgetGetControl(gw,CID_WoffMetadata));
	sf->woffMajor = woffMajor;
	sf->woffMinor = woffMinor;

	free(sf->gasp);
	sf->gasp_cnt = gasprows;
	if ( gasprows==0 )
	    sf->gasp = NULL;
	else {
	    sf->gasp = malloc(gasprows*sizeof(struct gasp));
	    sf->gasp_version = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_GaspVersion));
	    for ( i=0; i<gasprows; ++i ) {
		sf->gasp[i].ppem = gasp[5*i].u.md_ival;
		if ( sf->gasp_version==0 )
		    sf->gasp[i].flags = gasp[5*i+1].u.md_ival |
			    (gasp[5*i+2].u.md_ival<<1);
		else
		    sf->gasp[i].flags = gasp[5*i+1].u.md_ival |
			    (gasp[5*i+2].u.md_ival<<1) |
			    (gasp[5*i+4].u.md_ival<<2) |
			    (gasp[5*i+3].u.md_ival<<3);
	    }
	}
	sf->head_optimized_for_cleartype = GGadgetIsChecked(GWidgetGetControl(gw,CID_HeadClearType));

	OtfNameListFree(sf->fontstyle_name);
	sf->fontstyle_name = OtfNameFromStyleNames(GWidgetGetControl(gw,CID_StyleName));
	sf->design_size = design_size;
	sf->design_range_bottom = size_bottom;
	sf->design_range_top = size_top;
	sf->fontstyle_id = styleid;

	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Notice));
	free(sf->copyright); sf->copyright = cu_copy(txt);
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Comment));
	free(sf->comments); sf->comments = u2utf8_copy(*txt?txt:NULL);
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_FontLog));
	free(sf->fontlog); sf->fontlog = u2utf8_copy(*txt?txt:NULL);
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_DefBaseName));
	if ( *txt=='\0' || GGadgetIsChecked(GWidgetGetControl(gw,CID_SameAsFontname)) )
	    txt = NULL;
	free(sf->defbasefilename); sf->defbasefilename = u2utf8_copy(txt);
	if ( sf->subfontcnt!=0 ) {
	    sf->cidversion = cidversion;
	} else {
	    txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Version));
	    free(sf->version); sf->version = cu_copy(txt);
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
	if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_CPageDefault)) ) {
	    memcpy(sf->pfminfo.codepages,codepages,sizeof(codepages));
	    sf->pfminfo.hascodepages = true;
	} else
	    sf->pfminfo.hascodepages = false;
	if ( !GGadgetIsChecked(GWidgetGetControl(gw,CID_URangesDefault)) ) {
	    memcpy(sf->pfminfo.unicoderanges,uranges,sizeof(uranges));
	    sf->pfminfo.hasunicoderanges = true;
	} else
	    sf->pfminfo.hasunicoderanges = false;

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

	PSDictFree(sf->private);
	sf->private = GFI_ParsePrivate(d);

	if ( d->names_set )
	    StoreTTFNames(d);
	StoreSSNames(d);
	if ( d->ttf_set ) {
	    sf->os2_version = os2version;
	    sf->use_typo_metrics = GGadgetIsChecked(GWidgetGetControl(gw,CID_UseTypoMetrics));
	    sf->weight_width_slope_only = GGadgetIsChecked(GWidgetGetControl(gw,CID_WeightWidthSlopeOnly));
	    sf->pfminfo.weight = weight;
	    sf->pfminfo.width = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_WidthClass))+1;
	    pfmfam = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PFMFamily));
	    if ( pfmfam!=NULL )
		sf->pfminfo.pfmfamily = (intpt) (pfmfam->userdata);
	    else
		sf->pfminfo.pfmfamily = 0x11;
	    ibmfam = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_IBMFamily));
	    if ( ibmfam!=NULL )
		sf->pfminfo.os2_family_class = (intpt) (ibmfam->userdata);
	    else
		sf->pfminfo.os2_family_class = 0x00;
        stylemap = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_StyleMap));
        if ( stylemap!=NULL )
        sf->pfminfo.stylemap = (intpt) (stylemap->userdata);
        else
        sf->pfminfo.stylemap = -1;
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
	    sf->pfminfo.os2_capheight = capheight;
	    sf->pfminfo.os2_xheight = xheight;
	    sf->pfminfo.pfmset = true;

	    sf->pfminfo.subsuper_set = !GGadgetIsChecked(GWidgetGetControl(gw,CID_SubSuperDefault));
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
	/* must come after all scaleable fields (linegap, etc.) */
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
	GFI_SetLayers(d);
	if ( guideorder2!=sf->grid.order2 ) {
	    if ( guideorder2 )
		SFConvertGridToOrder2(sf);
	    else
		SFConvertGridToOrder3(sf);
	}
	GFI_ApplyLookupChanges(d);
	if ( retitle_fv )
	    FVSetTitles(sf);
	if ( reformat_fv || was_ml!=sf->multilayer || was_stroke!=sf->strokedfont )
	    FontViewReformatAll(sf);
	sf->changed = true;
	SFSetModTime(sf);
	sf->changed_since_autosave = true;
	sf->changed_since_xuidchanged = !xuidchanged;
	/* Just in case they changed the blue values and we are showing blues */
	/*  in outline views... */
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    CharView *cv;
	    for ( cv = (CharView *) (sf->glyphs[i]->views); cv!=NULL; cv=(CharView *) (cv->b.next) ) {
		cv->back_img_out_of_date = true;
		GDrawRequestExpose(cv->v,NULL,false);
	    }
	}
	MacFeatListFree(sf->features);
	sf->features = GGadgetGetUserData(GWidgetGetControl(d->gw,CID_Features));
	last_aspect = d->old_aspect;

	/* Class 0 is unused */
	MarkClassFree(sf->mark_class_cnt,sf->mark_classes,sf->mark_class_names);
	sf->mark_class_cnt = mc_rows + 1;
	sf->mark_classes     = malloc((mc_rows+1)*sizeof(char *));
	sf->mark_class_names = malloc((mc_rows+1)*sizeof(char *));
	sf->mark_classes[0] = sf->mark_class_names[0] = NULL;
	for ( i=0; i<mc_rows; ++i ) {
	    sf->mark_class_names[i+1] = copy(markclasses[2*i+0].u.md_str);
	    sf->mark_classes[i+1]     = GlyphNameListDeUnicode(markclasses[2*i+1].u.md_str);
	}

	/* Set 0 is used */
	MarkSetFree(sf->mark_set_cnt,sf->mark_sets,sf->mark_set_names);
	sf->mark_set_cnt = ms_rows;
	sf->mark_sets      = malloc((ms_rows)*sizeof(char *));
	sf->mark_set_names = malloc((ms_rows)*sizeof(char *));
	for ( i=0; i<ms_rows; ++i ) {
	    sf->mark_set_names[i] = copy(marksets[2*i+0].u.md_str);
	    sf->mark_sets[i]      = GlyphNameListDeUnicode(marksets[2*i+1].u.md_str);
	}

	GFI_Close(d);

	SFReplaceFontnameBDFProps(sf);
	MVReFeatureAll(sf);
	MVReKernAll(sf);

	collabclient_sendFontLevelRedo( sf );

    }
return( true );
}

static void GFI_AsDsLab(struct gfi_data *d, int cid, int onlylabel) {
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
    if ( onlylabel )
return;

    if ( cid == CID_TypoAscentIsOff ) {
	const unichar_t *as = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Ascent));
	double av=u_strtod(as,&end);
	b.maxy = *end=='\0' ? av : d->sf->ascent;
    } else if ( cid == CID_TypoDescentIsOff ) {
	const unichar_t *ds = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
	double dv=u_strtod(ds,&end);
	b.miny = *end=='\0' ? -dv : -d->sf->descent;
    } else {
	CIDLayerFindBounds(d->sf,ly_fore,&b);
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
	GFI_AsDsLab(d,GGadgetGetCid(g),false);
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
	int old1, old2;
	memset(&info,0,sizeof(info));
	old1 = d->sf->pfminfo.pfmset; old2 = d->sf->pfminfo.panose_set;
	d->sf->pfminfo.pfmset = false; d->sf->pfminfo.panose_set = false;
	SFDefaultOS2Info(&info,d->sf,n);
	d->sf->pfminfo.pfmset = old1; d->sf->pfminfo.panose_set = old2;
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

static int GFI_ShowPanoseDocs(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	help("http://panose.com/");
    }
return( true );
}

static void GFI_SetPanoseLists(struct gfi_data *d) {
    int kind = GGadgetGetFirstListSelectedItem(GWidgetGetControl(d->gw,CID_PanFamily));
    int i;

    if ( kind>=0 && kind!=d->last_panose_family ) {
	if ( kind>6 ) kind=6;	/* No data for any kinds beyond 5, so anything bigger is just undefined, and that's 6 */
	for ( i=1; i<10; ++i ) {
	    GGadget *lab = GWidgetGetControl(d->gw,CID_PanFamilyLab+i);
	    GGadget *list = GWidgetGetControl(d->gw,CID_PanFamily+i);
	    int temp = GGadgetGetFirstListSelectedItem(list);
	    if ( kind==5 && i==2 ) temp=1;
	    if ( temp>=16 && (kind!=4 || i!=5)) temp=15;
	    else if ( temp>16 ) temp=16;	/* Decorative.serifvar has 17 elements */
	    GGadgetSetTitle8WithMn(lab,panoses[kind][i-1].name);
	    GGadgetSetList(list,GTextInfoArrayFromList(panoses[kind][i-1].variants,NULL),false);
	    GGadgetSelectOneListItem(list,temp);
	}
    }
}

static int GFI_ChangePanoseFamily(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GFI_SetPanoseLists(d);
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
	const unichar_t *ia = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
	unichar_t *aend, *dend, *iend;
	double av=u_strtod(as,&aend),dv=u_strtod(ds,&dend),iav=u_strtod(ia,&iend);
	struct pfminfo info;
	if ( *aend!='\0' ) av = d->sf->ascent;
	if ( *dend!='\0' ) dv = d->sf->descent;
	if ( *iend!='\0' ) iav = d->sf->italicangle;
	memset(&info,0,sizeof(info));
	SFDefaultOS2SubSuper(&info,(int) (dv+av), iav);
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

    if ( d->sf->os2_version>=0 && d->sf->os2_version<=4 )
	GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_OS2Version),d->sf->os2_version);
    else {
	sprintf( buffer,"%d", d->sf->os2_version );
	GGadgetSetTitle8(GWidgetGetControl(d->gw,CID_OS2Version),buffer);
    }
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_UseTypoMetrics),d->sf->use_typo_metrics);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_WeightWidthSlopeOnly),d->sf->weight_width_slope_only);

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
    if ( info.panose_set ) {
	for ( i=0; i<10; ++i ) {
	    int v = info.panose[i];
	    if ( v>=16 && (info.panose[0]!=4 || i!=5 )) v=15;
	    else if ( v>16 ) v=16;	/* Sigh Decorative.SerifVar has 17 elements */
	    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_PanFamily+i),v);
	    if ( i==0 )
		GFI_SetPanoseLists(d);
	}
    }

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
    GFI_AsDsLab(d,CID_WinAscentIsOff,true);
    sprintf( buffer, "%d", info.os2_winascent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WinAscent),ubuf);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_WinDescentIsOff),info.windescent_add);
    GFI_AsDsLab(d,CID_WinDescentIsOff,true);
    sprintf( buffer, "%d", info.os2_windescent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WinDescent),ubuf);

    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TypoAscentIsOff),info.typoascent_add);
    GFI_AsDsLab(d,CID_TypoAscentIsOff,true);
    sprintf( buffer, "%d", info.os2_typoascent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TypoAscent),ubuf);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TypoDescentIsOff),info.typodescent_add);
    GFI_AsDsLab(d,CID_TypoDescentIsOff,true);
    sprintf( buffer, "%d", info.os2_typodescent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TypoDescent),ubuf);

    sprintf( buffer, "%d", info.os2_capheight );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_CapHeight),ubuf);
    sprintf( buffer, "%d", info.os2_xheight );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_XHeight),ubuf);

    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_HHeadAscentIsOff),info.hheadascent_add);
    GFI_AsDsLab(d,CID_HHeadAscentIsOff,true);
    sprintf( buffer, "%d", info.hhead_ascent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_HHeadAscent),ubuf);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_HHeadDescentIsOff),info.hheaddescent_add);
    GFI_AsDsLab(d,CID_HHeadDescentIsOff,true);
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
	    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMathSym)) )
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
    GGadgetCreateData txgcd[35], boxes[3], *txarray[18][3], *barray[9];
    GTextInfo txlabel[35];
    int i,y,k,j;
    char **params, **popups;
    char values[20][20];

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXText)) )
return( true );
	else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMathSym)) ) {
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

	k=j=0; y = 10;
	for ( i=0; params[i]!=0; ++i ) {
	    txlabel[k].text = (unichar_t *) params[i];
	    txlabel[k].text_is_1byte = true;
	    txgcd[k].gd.label = &txlabel[k];
	    txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = y+4;
	    txgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	    txgcd[k].gd.popup_msg = (unichar_t *) popups[i];
	    txgcd[k++].creator = GLabelCreate;
	    txarray[j][0] = &txgcd[k-1];

	    sprintf( values[i], "%g", d->texdata.params[i+7]*(double) (d->sf->ascent+d->sf->descent)/(double) (1<<20));
	    txlabel[k].text = (unichar_t *) values[i];
	    txlabel[k].text_is_1byte = true;
	    txgcd[k].gd.label = &txlabel[k];
	    txgcd[k].gd.pos.x = 85; txgcd[k].gd.pos.y = y;
	    txgcd[k].gd.pos.width = 75;
	    txgcd[k].gd.flags = gg_visible | gg_enabled;
	    txgcd[k].gd.cid = CID_TeX + i;
	    txgcd[k++].creator = GTextFieldCreate;
	    txarray[j][1] = &txgcd[k-1]; txarray[j++][2] = NULL;
	    y += 26;
	}
	txarray[j][0] = GCD_Glue; txarray[j][1] = GCD_Glue; txarray[j++][2] = NULL;

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

	barray[0] = barray[2] = barray[3] = barray[4] = barray[6] = GCD_Glue; barray[7] = NULL;
	barray[1] = &txgcd[k-2]; barray[5] = &txgcd[k-1];
	txarray[j][0] = &boxes[2]; txarray[j][1] = GCD_ColSpan; txarray[j++][2] = NULL;
	txarray[j][0] = NULL;

	memset(boxes,0,sizeof(boxes));
	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = txarray[0];
	boxes[0].creator = GHVGroupCreate;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = barray;
	boxes[2].creator = GHBoxCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
	GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
	GHVBoxFitWindow(boxes[0].ret);
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
	GGadget *extrasp = GWidgetGetControl(d->gw,CID_TeXExtraSpLabel);

	if ( GGadgetGetCid(g)==CID_TeXText ) {
	    GGadgetSetTitle8(extrasp,
/* GT: Extra Space */
		    _("Extra Sp:"));
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MoreParams),false);
	} else {
	    GGadgetSetTitle8(extrasp, _("Math Sp:"));
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MoreParams),true);
	}
	/* In Polish we need to reflow the text after changing the label */
	GHVBoxReflow(GWidgetGetControl(d->gw,CID_TeXBox));
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
	GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TeXMathSym), true);
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

static void FigureUnicode(struct gfi_data *d) {
    int includeempties = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_UnicodeEmpties));
    GGadget *list = GWidgetGetControl(d->gw,CID_Unicode);
    struct rangeinfo *ri;
    int cnt, i;
    GTextInfo **ti;
    char buffer[200];

    GGadgetClearList(list);
    ri = SFUnicodeRanges(d->sf,(includeempties?ur_includeempty:0)|ur_sortbyunicode);
    if ( ri==NULL ) cnt=0;
    else
	for ( cnt=0; ri[cnt].range!=NULL; ++cnt );

    ti = malloc((cnt+1) * sizeof( GTextInfo * ));
    for ( i=0; i<cnt; ++i ) {
	if ( ri[i].range->first==-1 )
	    snprintf( buffer, sizeof(buffer),
		    "%s  %d/0", _(ri[i].range->name), ri[i].cnt);
	else
	    snprintf( buffer, sizeof(buffer),
		    "%s  U+%04X-U+%04X %d/%d",
		    _(ri[i].range->name),
		    (int) ri[i].range->first, (int) ri[i].range->last,
		    ri[i].cnt, ri[i].range->actual );
	ti[i] = calloc(1,sizeof(GTextInfo));
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	ti[i]->text = utf82u_copy(buffer);
	ti[i]->userdata = ri[i].range;
    }
    ti[i] = calloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,ti,false);
    free(ri);
}

static int GFI_UnicodeRangeChange(GGadget *g, GEvent *e) {
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    GTextInfo *ti = GGadgetGetListItemSelected(g);
    struct unicoderange *r;
    int gid, first=-1;
    SplineFont *sf = d->sf;
    FontView *fv = (FontView *) (sf->fv);
    EncMap *map = fv->b.map;
    int i, enc;

    if ( ti==NULL )
return( true );
    if ( e->type!=et_controlevent ||
	    (e->u.control.subtype != et_listselected &&e->u.control.subtype != et_listdoubleclick))
return( true );

    r = ti->userdata;

    for ( i=0; i<map->enccount; ++i )
	fv->b.selected[i] = 0;

    if ( e->u.control.subtype == et_listselected ) {
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	    enc = map->backmap[gid];
	    if ( sf->glyphs[gid]->unicodeenc>=r->first && sf->glyphs[gid]->unicodeenc<=r->last &&
		    enc!=-1 ) {
		if ( first==-1 || enc<first ) first = enc;
		fv->b.selected[enc] = true;
	    }
	}
    } else if ( e->u.control.subtype == et_listdoubleclick && !r->unassigned ) {
	char *found = calloc(r->last-r->first+1,1);
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	    int u = sf->glyphs[gid]->unicodeenc;
	    if ( u>=r->first && u<=r->last ) {
		found[u-r->first] = true;
	    }
	}
	for ( i=0; i<=r->last-r->first; ++i ) {
	    if ( isunicodepointassigned(i+r->first) && !found[i] ) {
		enc = EncFromUni(i+r->first,map->enc);
		if ( enc!=-1 ) {
		    if ( first==-1 || enc<first ) first = enc;
		    fv->b.selected[enc] = true;
		}
	    }
	}
	free(found);
    }
    if ( first==-1 ) {
	GDrawBeep(NULL);
    } else {
	FVScrollToChar(fv,first);
    }
    GDrawRequestExpose(fv->v,NULL,false);
return( true );
}

static int GFI_UnicodeEmptiesChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	FigureUnicode(d);
    }
return( true );
}

static int GFI_AspectChange(GGadget *g, GEvent *e) {
    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_radiochanged )) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int new_aspect = GTabSetGetSel(g);
	int rows;

	if ( d->old_aspect == d->tn_aspect )
	    GMatrixEditGet(GWidgetGetControl(d->gw,CID_TNames), &rows);
	if ( !d->ttf_set && new_aspect == d->ttfv_aspect )
	    TTFSetup(d);
	else if ( !d->names_set && new_aspect == d->tn_aspect ) {
	    TTFNames_Resort(d);
	    d->names_set = true;
	} else if ( !d->tex_set && new_aspect == d->tx_aspect )
	    DefaultTeX(d);
	else if ( new_aspect == d->unicode_aspect )
	    FigureUnicode(d);
	d->old_aspect = new_aspect;
    }
return( true );
}

static int GFI_URangeAspectChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GTabSetSetSel(GWidgetGetControl(d->gw,CID_Tabs),d->ttfv_aspect);
	GFI_AspectChange(GWidgetGetControl(d->gw,CID_Tabs),NULL);
	GTabSetSetSel(GWidgetGetControl(d->gw,CID_TTFTabs),4);
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	GFI_CancelClose(d);
    } else if ( event->type==et_destroy ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	free(d);
    } else if ( event->type==et_char ) {
return( GFI_Char(GDrawGetUserData(gw),event));
    }
return( true );
}

static int OS2_UnicodeChange(GGadget *g, GEvent *e) {
    int32 flags[4];
    int len,i,bit,set;

    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_textchanged )) {
	const unichar_t *ret;
	unichar_t *end;
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GWindow gw = d->gw;
	GGadget *list;

	ret = _GGadgetGetTitle(g);
	flags[3] = u_strtoul(ret,&end,16);
	while ( !ishexdigit(*end) && *end!='\0' ) ++end;
	flags[2] = u_strtoul(end,&end,16);
	while ( !ishexdigit(*end) && *end!='\0' ) ++end;
	flags[1] = u_strtoul(end,&end,16);
	while ( !ishexdigit(*end) && *end!='\0' ) ++end;
	flags[0] = u_strtoul(end,&end,16);

	list = GWidgetGetControl(gw,CID_UnicodeList);

	for ( i=0; unicoderangenames[i].text!=NULL; ++i ) {
	    bit = (intpt) (unicoderangenames[i].userdata);
	    set = (flags[bit>>5]&(1<<(bit&31)))?1 : 0;
	    GGadgetSelectListItem(list,i,set);
	}
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GWindow gw = d->gw;
	char ranges[40];
	GTextInfo **list = GGadgetGetList(g,&len);
	GGadget *field = GWidgetGetControl(gw,CID_UnicodeRanges);

	flags[0] = flags[1] = flags[2] = flags[3] = 0;
	for ( i=0; i<len; ++i )
	    if ( list[i]->selected ) {
		bit = ((intpt) (list[i]->userdata));
		flags[bit>>5] |= (1<<(bit&31));
	    }

	sprintf( ranges, "%08x.%08x.%08x.%08x", flags[3], flags[2], flags[1], flags[0]);
	GGadgetSetTitle8(field,ranges);
    }
return( true );
}

static int OS2_URangesDefault(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int def = GGadgetIsChecked(g);
	GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_UnicodeRanges),!def);
	GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_UnicodeList),!def);
	if ( def ) {
	    SplineFont *sf = gfi->sf;
	    char ranges[40];
	    OS2FigureUnicodeRanges(sf,sf->pfminfo.unicoderanges);
	    sprintf( ranges, "%08x.%08x.%08x.%08x",
		    sf->pfminfo.unicoderanges[3], sf->pfminfo.unicoderanges[2],
		    sf->pfminfo.unicoderanges[1], sf->pfminfo.unicoderanges[0]);
	    GGadgetSetTitle8(GWidgetGetControl(gfi->gw,CID_UnicodeRanges),ranges);
	    OS2_UnicodeChange(GWidgetGetControl(gfi->gw,CID_UnicodeRanges),NULL);
	}
    }
return( true );
}

static int OS2_CodePageChange(GGadget *g, GEvent *e) {
    int32 flags[2];
    int len,i,bit,set;

    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_textchanged )) {
	const unichar_t *ret;
	unichar_t *end;
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GWindow gw = d->gw;
	GGadget *list;

	ret = _GGadgetGetTitle(g);
	flags[1] = u_strtoul(ret,&end,16);
	while ( !ishexdigit(*end) && *end!='\0' ) ++end;
	flags[0] = u_strtoul(end,&end,16);

	list = GWidgetGetControl(gw,CID_CodePageList);

	for ( i=0; codepagenames[i].text!=NULL; ++i ) {
	    bit = (intpt) (codepagenames[i].userdata);
	    set = (flags[bit>>5]&(1<<(bit&31)))?1 : 0;
	    GGadgetSelectListItem(list,i,set);
	}
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GWindow gw = d->gw;
	char ranges[40];
	GTextInfo **list = GGadgetGetList(g,&len);
	GGadget *field = GWidgetGetControl(gw,CID_CodePageRanges);

	flags[0] = flags[1] = 0;
	for ( i=0; i<len; ++i )
	    if ( list[i]->selected ) {
		bit = ((intpt) (list[i]->userdata));
		flags[bit>>5] |= (1<<(bit&31));
	    }

	sprintf( ranges, "%08x.%08x", flags[1], flags[0]);
	GGadgetSetTitle8(field,ranges);
    }
return( true );
}

static int OS2_CPageDefault(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int def = GGadgetIsChecked(g);
	GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_CodePageRanges),!def);
	GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_CodePageList),!def);
	if ( def ) {
	    SplineFont *sf = gfi->sf;
	    char codepages[40];
	    OS2FigureCodePages(sf,sf->pfminfo.codepages);
	    sprintf( codepages, "%08x.%08x",
		    sf->pfminfo.codepages[1], sf->pfminfo.codepages[0]);
	    GGadgetSetTitle8(GWidgetGetControl(gfi->gw,CID_CodePageRanges),codepages);
	    OS2_CodePageChange(GWidgetGetControl(gfi->gw,CID_CodePageRanges),NULL);
	}
    }
return( true );
}

static int GFI_UseUniqueIDChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_UniqueID),
		GGadgetIsChecked(g));
    }
return( true );
}

static int GFI_UseXUIDChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_XUID),
		GGadgetIsChecked(g));
    }
return( true );
}

static void LookupSetup(struct lkdata *lk,OTLookup *lookups) {
    int cnt, subcnt;
    OTLookup *otl;
    struct lookup_subtable *sub;

    for ( cnt=0, otl=lookups; otl!=NULL; ++cnt, otl=otl->next );
    lk->cnt = cnt; lk->max = cnt+10;
    lk->all = calloc(lk->max,sizeof(struct lkinfo));
    for ( cnt=0, otl=lookups; otl!=NULL; ++cnt, otl=otl->next ) {
	lk->all[cnt].lookup = otl;
	for ( subcnt=0, sub=otl->subtables; sub!=NULL; ++subcnt, sub=sub->next );
	lk->all[cnt].subtable_cnt = subcnt; lk->all[cnt].subtable_max = subcnt+10;
	lk->all[cnt].subtables = calloc(lk->all[cnt].subtable_max,sizeof(struct lksubinfo));
	for ( subcnt=0, sub=otl->subtables; sub!=NULL; ++subcnt, sub=sub->next )
	    lk->all[cnt].subtables[subcnt].subtable = sub;
    }
}

static void LookupInfoFree(struct lkdata *lk) {
    int cnt;

    for ( cnt=0; cnt<lk->cnt; ++cnt )
	free(lk->all[cnt].subtables);
    free(lk->all);
}

#define LK_MARGIN 2

struct selection_bits {
    int lookup_cnt, sub_cnt;	/* Number of selected lookups, and selected sub tables */
    int a_lookup, a_sub;	/* The index of one of those lookups, or subtables */
    int a_sub_lookup;		/*  the index of the lookup containing a_sub */
    int any_first, any_last;	/* Whether any of the selected items is first or last in its catagory */
    int sub_table_mergeable;	/* Can we merge the selected subtables? */
    int lookup_mergeable;	/* Can we merge the selected lookups? */
};

static void LookupParseSelection(struct lkdata *lk, struct selection_bits *sel) {
    int lookup_cnt, sub_cnt, any_first, any_last, all_one_lookup;
    int a_lookup, a_sub, a_sub_lookup;
    int sub_mergeable, lookup_mergeable;
    int i,j;

    lookup_cnt = sub_cnt = any_first = any_last = 0;
    all_one_lookup = a_lookup = a_sub = a_sub_lookup = -1;
    sub_mergeable = lookup_mergeable = true;
    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	if ( lk->all[i].selected ) {
	    ++lookup_cnt;
	    if ( a_lookup==-1 )
		a_lookup = i;
	    else if ( lk->all[i].lookup->lookup_type!=lk->all[a_lookup].lookup->lookup_type ||
		    lk->all[i].lookup->lookup_flags!=lk->all[a_lookup].lookup->lookup_flags )
		lookup_mergeable = false;
	    if ( i==0 ) any_first=true;
	    if ( i==lk->cnt-1 ) any_last=true;
	}
	if ( lk->all[i].open ) {
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		if ( lk->all[i].subtables[j].selected ) {
		    ++sub_cnt;
		    if ( a_sub==-1 ) {
			a_sub = j; a_sub_lookup = i;
		    }
		    if ( j==0 ) any_first = true;
		    if ( j==lk->all[i].subtable_cnt-1 ) any_last = true;
		    if ( lk->all[i].subtables[j].subtable->kc!=NULL ||
			    lk->all[i].subtables[j].subtable->fpst!=NULL ||
			    lk->all[i].subtables[j].subtable->sm!=NULL )
			sub_mergeable = false;
		    if ( all_one_lookup==-1 )
			all_one_lookup = i;
		    else if ( all_one_lookup!=i )
			all_one_lookup = -2;
		}
	    }
	}
    }

    sel->lookup_cnt = lookup_cnt;
    sel->sub_cnt = sub_cnt;
    sel->a_lookup = a_lookup;
    sel->a_sub = a_sub;
    sel->a_sub_lookup = a_sub_lookup;
    sel->any_first = any_first;
    sel->any_last = any_last;
    sel->sub_table_mergeable = sub_mergeable && all_one_lookup>=0 && sub_cnt>=2 && lookup_cnt==0;
    sel->lookup_mergeable = lookup_mergeable && lookup_cnt>=2 && sub_cnt==0;
}

static int LookupsImportable(SplineFont *sf,int isgpos) {
    FontView *ofv;

    for ( ofv=fv_list; ofv!=NULL; ofv = (FontView *) (ofv->b.next) ) {
	SplineFont *osf = ofv->b.sf;
	if ( osf->cidmaster ) osf = osf->cidmaster;
	if ( osf==sf || sf->cidmaster==osf )
    continue;
	if ( (isgpos && osf->gpos_lookups!=NULL) || (!isgpos && osf->gsub_lookups!=NULL) )
    break;
    }
return( ofv!=NULL );
}

void GFI_LookupEnableButtons(struct gfi_data *gfi, int isgpos) {
    struct lkdata *lk = &gfi->tables[isgpos];
    struct selection_bits sel;

    LookupParseSelection(lk,&sel);

    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_LookupTop),!sel.any_first &&
	    sel.lookup_cnt+sel.sub_cnt==1);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_LookupUp),!sel.any_first &&
	    sel.lookup_cnt+sel.sub_cnt!=0);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_LookupDown),!sel.any_last &&
	    sel.lookup_cnt+sel.sub_cnt!=0);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_LookupBottom),!sel.any_last &&
	    sel.lookup_cnt+sel.sub_cnt==1);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_AddLookup),
	    (sel.lookup_cnt+sel.sub_cnt<=1));
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_AddSubtable),
	    (sel.lookup_cnt==1 && sel.sub_cnt<=1) || (sel.lookup_cnt==0 && sel.sub_cnt==1));
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_EditMetadata),
	    (sel.lookup_cnt==1 && sel.sub_cnt==0) ||
	    (sel.lookup_cnt==0 && sel.sub_cnt==1));
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_EditSubtable),sel.lookup_cnt==0 &&
	    sel.sub_cnt==1);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_DeleteLookup),sel.lookup_cnt!=0 ||
	    sel.sub_cnt!=0);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_MergeLookup),
	    (sel.lookup_cnt>=2 && sel.sub_cnt==0 && sel.lookup_mergeable) ||
	    (sel.lookup_cnt==0 && sel.sub_cnt>=2 && sel.sub_table_mergeable));
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_RevertLookups),true);
    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_LookupSort),lk->cnt>1 );

    GGadgetSetEnabled(GWidgetGetControl(gfi->gw,CID_ImportLookups),
	    LookupsImportable(gfi->sf,isgpos));
}

void GFI_LookupScrollbars(struct gfi_data *gfi, int isgpos, int refresh) {
    int lcnt, i,j;
    int width=0, wmax;
    GWindow gw = GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos));
    struct lkdata *lk = &gfi->tables[isgpos];
    GGadget *vsb = GWidgetGetControl(gfi->gw,CID_LookupVSB+isgpos);
    GGadget *hsb = GWidgetGetControl(gfi->gw,CID_LookupHSB+isgpos);
    int off_top, off_left;

    GDrawSetFont(gw,gfi->font);
    lcnt = 0;
    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	++lcnt;
	wmax = GDrawGetText8Width(gw,lk->all[i].lookup->lookup_name,-1);
	if ( wmax > width ) width = wmax;
	if ( lk->all[i].open ) {
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		++lcnt;
		wmax = gfi->fh+GDrawGetText8Width(gw,lk->all[i].subtables[j].subtable->subtable_name,-1);
		if ( wmax > width ) width = wmax;
	    }
	}
    }
    width += gfi->fh;
    GScrollBarSetBounds(vsb,0,lcnt,(gfi->lkheight-2*LK_MARGIN)/gfi->fh);
    GScrollBarSetBounds(hsb,0,width,gfi->lkwidth-2*LK_MARGIN);
    off_top = lk->off_top;
    if ( off_top+((gfi->lkheight-2*LK_MARGIN)/gfi->fh) > lcnt )
	off_top = lcnt - (gfi->lkheight-2*LK_MARGIN)/gfi->fh;
    if ( off_top<0 )
	off_top  = 0;
    off_left = lk->off_left;
    if ( off_left+gfi->lkwidth-2*LK_MARGIN > width )
	off_left = width-(gfi->lkwidth-2*LK_MARGIN);
    if ( off_left<0 )
	off_left  = 0;
    if ( off_top!=lk->off_top || off_left!=lk->off_left ) {
	lk->off_top = off_top; lk->off_left = off_left;
	GScrollBarSetPos(vsb,off_top);
	GScrollBarSetPos(hsb,off_left);
	refresh = true;
    }
    if ( refresh )
	GDrawRequestExpose(gw,NULL,false);
}

static int LookupsHScroll(GGadget *g,GEvent *event) {
    struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
    int isgpos = GGadgetGetCid(g)-CID_LookupHSB;
    struct lkdata *lk = &gfi->tables[isgpos];
    int newpos = lk->off_left;
    int32 sb_min, sb_max, sb_pagesize;

    if ( event->type!=et_controlevent || event->u.control.subtype != et_scrollbarchange )
return( true );

    GScrollBarGetBounds(event->u.control.g,&sb_min,&sb_max,&sb_pagesize);
    switch( event->u.control.u.sb.type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*sb_pagesize/10;
      break;
      case et_sb_up:
        newpos -= sb_pagesize/15;
      break;
      case et_sb_down:
        newpos += sb_pagesize/15;
      break;
      case et_sb_downpage:
        newpos += 9*sb_pagesize/10;
      break;
      case et_sb_bottom:
        newpos = sb_max-sb_pagesize;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = event->u.control.u.sb.pos;
      break;
      case et_sb_halfup:
        newpos -= sb_pagesize/30;
      break;
      case et_sb_halfdown:
        newpos += sb_pagesize/30;
      break;
    }
    if ( newpos>sb_max-sb_pagesize )
        newpos = sb_max-sb_pagesize;
    if ( newpos<0 ) newpos = 0;
    if ( newpos!=lk->off_left ) {
	lk->off_left = newpos;
	GScrollBarSetPos(event->u.control.g,newpos);
	GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos)),NULL,false);
    }
return( true );
}

static int LookupsVScroll(GGadget *g,GEvent *event) {
    struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
    int isgpos = GGadgetGetCid(g)-CID_LookupVSB;
    struct lkdata *lk = &gfi->tables[isgpos];
    int newpos = lk->off_top;
    int32 sb_min, sb_max, sb_pagesize;

    if ( event->type!=et_controlevent || event->u.control.subtype != et_scrollbarchange )
return( true );

    GScrollBarGetBounds(event->u.control.g,&sb_min,&sb_max,&sb_pagesize);
    switch( event->u.control.u.sb.type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= 9*sb_pagesize/10;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += 9*sb_pagesize/10;
      break;
      case et_sb_bottom:
        newpos = (sb_max-sb_pagesize);
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = event->u.control.u.sb.pos;
      break;
    }
    if ( newpos>(sb_max-sb_pagesize) )
        newpos = (sb_max-sb_pagesize);
    if ( newpos<0 ) newpos = 0;
    if ( newpos!=lk->off_top ) {
	/*int diff = newpos-lk->off_top;*/
	lk->off_top = newpos;
	GScrollBarSetPos(event->u.control.g,newpos);
	/*GDrawScroll(GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos)),NULL,0,diff*gfi->fh);*/
	GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos)),NULL,false);
    }
return( true );
}

static int GFI_LookupOrder(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j,k;
	struct lkinfo temp;
	struct lksubinfo temp2;
	int cid = GGadgetGetCid(g);
	GWindow gw = GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos));

	if ( cid==CID_LookupTop ) {
	    for ( i=0; i<lk->cnt; ++i ) {
		if ( lk->all[i].deleted )
	    continue;
		if ( lk->all[i].selected ) {
		    temp = lk->all[i];
		    for ( k=i-1; k>=0; --k )
			lk->all[k+1] = lk->all[k];
		    lk->all[0] = temp;
    goto done;
		}
		if ( lk->all[i].open ) {
		    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
			if ( lk->all[i].subtables[j].deleted )
		    continue;
			if ( lk->all[i].subtables[j].selected ) {
			    temp2 = lk->all[i].subtables[j];
			    for ( k=j-1; k>=0; --k )
				lk->all[i].subtables[k+1] = lk->all[i].subtables[k];
			    lk->all[i].subtables[0] = temp2;
    goto done;
			}
		    }
		}
	    }
	} else if ( cid==CID_LookupBottom ) {
	    for ( i=0; i<lk->cnt; ++i ) {
		if ( lk->all[i].deleted )
	    continue;
		if ( lk->all[i].selected ) {
		    temp = lk->all[i];
		    for ( k=i; k<lk->cnt-1; ++k )
			lk->all[k] = lk->all[k+1];
		    lk->all[lk->cnt-1] = temp;
    goto done;
		}
		if ( lk->all[i].open ) {
		    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
			if ( lk->all[i].subtables[j].deleted )
		    continue;
			if ( lk->all[i].subtables[j].selected ) {
			    temp2 = lk->all[i].subtables[j];
			    for ( k=j; k<lk->all[i].subtable_cnt-1; ++k )
				lk->all[i].subtables[k] = lk->all[i].subtables[k+1];
			    lk->all[i].subtables[lk->all[i].subtable_cnt-1] = temp2;
    goto done;
			}
		    }
		}
	    }
	} else if ( cid==CID_LookupUp ) {
	    for ( i=0; i<lk->cnt; ++i ) {
		if ( lk->all[i].deleted )
	    continue;
		if ( lk->all[i].selected && i!=0 ) {
		    temp = lk->all[i];
		    lk->all[i] = lk->all[i-1];
		    lk->all[i-1] = temp;
		}
		if ( lk->all[i].open ) {
		    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
			if ( lk->all[i].subtables[j].deleted )
		    continue;
			if ( lk->all[i].subtables[j].selected && j!=0 ) {
			    temp2 = lk->all[i].subtables[j];
			    lk->all[i].subtables[j] = lk->all[i].subtables[j-1];
			    lk->all[i].subtables[j-1] = temp2;
			}
		    }
		}
	    }
	} else if ( cid==CID_LookupDown ) {
	    for ( i=lk->cnt-1; i>=0; --i ) {
		if ( lk->all[i].deleted )
	    continue;
		if ( lk->all[i].selected && i!=lk->cnt-1 ) {
		    temp = lk->all[i];
		    lk->all[i] = lk->all[i+1];
		    lk->all[i+1] = temp;
		}
		if ( lk->all[i].open ) {
		    for ( j=lk->all[i].subtable_cnt-1; j>=0; --j ) {
			if ( lk->all[i].subtables[j].deleted )
		    continue;
			if ( lk->all[i].subtables[j].selected && j!=lk->all[i].subtable_cnt-1 ) {
			    temp2 = lk->all[i].subtables[j];
			    lk->all[i].subtables[j] = lk->all[i].subtables[j+1];
			    lk->all[i].subtables[j+1] = temp2;
			}
		    }
		}
	    }
	}
    done:
	GFI_LookupEnableButtons(gfi,isgpos);
	GDrawRequestExpose(gw,NULL,false);
    }
return( true );
}

static int GFI_LookupSort(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	struct lkinfo temp;
	int i,j;

	for ( i=0; i<lk->cnt; ++i ) {
	    int order = FeatureOrderId(isgpos,lk->all[i].lookup->features);
	    for ( j=i+1; j<lk->cnt; ++j ) {
		int jorder = FeatureOrderId(isgpos,lk->all[j].lookup->features);
		if ( order>jorder) {
		    temp = lk->all[i];
		    lk->all[i] = lk->all[j];
		    lk->all[j] = temp;
		    order = jorder;
		}
	    }
	}
	GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos)),NULL,false);
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

/* ??? *//* How about a series of buttons to show only by lookup_type, feat-tag, script-tag */

void GFI_FinishContextNew(struct gfi_data *d,FPST *fpst, int success) {
    OTLookup *otl;
    struct lookup_subtable *sub, *prev;
    FPST *ftest, *fprev;

    if ( !success ) {
	/* We can't allow incomplete FPSTs to float around */
	/* If they didn't fill it in, delete it */
	otl = fpst->subtable->lookup;
	prev = NULL;
	for ( sub=otl->subtables; sub!=NULL && sub!=fpst->subtable; prev = sub, sub=sub->next );
	if ( sub!=NULL ) {
	    if ( prev==NULL )
		otl->subtables = sub->next;
	    else
		prev->next = sub->next;
	    free(sub->subtable_name);
	    chunkfree(sub,sizeof(struct lookup_subtable));
	}
	fprev = NULL;
	for ( ftest=d->sf->possub; ftest!=NULL && ftest!=fpst; fprev = ftest, ftest=ftest->next );
	if ( ftest!=NULL ) {
	    if ( fprev==NULL )
		d->sf->possub = fpst->next;
	    else
		fprev->next = fpst->next;
	}

	chunkfree(fpst,sizeof(FPST));
    }
}

void GFI_FinishSMNew(struct gfi_data *d,ASM *sm, int success, int isnew) {
    OTLookup *otl;
    struct lookup_subtable *sub, *prev;
    ASM *smtest, *smprev;

    if ( !success && isnew ) {
	/* We can't allow incomplete state machines floating around */
	/* If they didn't fill it in, delete it */
	otl = sm->subtable->lookup;
	prev = NULL;
	for ( sub=otl->subtables; sub!=NULL && sub!=sm->subtable; prev = sub, sub=sub->next );
	if ( sub!=NULL ) {
	    if ( prev==NULL )
		otl->subtables = sub->next;
	    else
		prev->next = sub->next;
	    free(sub->subtable_name);
	    chunkfree(sub,sizeof(struct lookup_subtable));
	}
	smprev = NULL;
	for ( smtest=d->sf->sm; smtest!=NULL && smtest!=sm; smprev = smtest, smtest=smtest->next );
	if ( smtest!=NULL ) {
	    if ( smprev==NULL )
		d->sf->sm = sm->next;
	    else
		smprev->next = sm->next;
	}
	chunkfree(sm,sizeof(ASM));
    }
}

static void LookupSubtableContents(struct gfi_data *gfi,int isgpos) {
    struct lkdata *lk = &gfi->tables[isgpos];
    int i,j;

    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	if ( lk->all[i].open ) {
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		if ( lk->all[i].subtables[j].selected ) {
		    _LookupSubtableContents(gfi->sf,lk->all[i].subtables[j].subtable,NULL,gfi->def_layer);
return;
		}
	    }
	}
    }
}

static int GFI_LookupEditSubtableContents(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	LookupSubtableContents(gfi,isgpos);
    }
return( true );
}

static int GFI_LookupAddLookup(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j,k,lcnt;
	OTLookup *otl = chunkalloc(sizeof(OTLookup)), *test;

	k = 0;
	for ( test = isgpos ? gfi->sf->gpos_lookups : gfi->sf->gsub_lookups;
		test!=NULL; test = test->next )
	    ++k;
	otl->lookup_index = k;

	if ( !EditLookup(otl,isgpos,gfi->sf)) {
	    chunkfree(otl,sizeof(OTLookup));
return( true );
	}
	for ( i=lk->cnt-1; i>=0; --i ) {
	    if ( !lk->all[i].deleted && lk->all[i].selected ) {
		lk->all[i].selected = false;
	break;
	    }
	    if ( !lk->all[i].deleted && lk->all[i].open ) {
		for ( j=0; j<lk->all[i].subtable_cnt; ++j )
		    if ( !lk->all[i].subtables[j].deleted &&
			    lk->all[i].subtables[j].selected ) {
			lk->all[i].subtables[j].selected = false;
		break;
		    }
		if ( j<lk->all[i].subtable_cnt )
	break;
	    }
	}
	if ( lk->cnt>=lk->max )
	    lk->all = realloc(lk->all,(lk->max+=10)*sizeof(struct lkinfo));
	for ( k=lk->cnt; k>i+1; --k )
	    lk->all[k] = lk->all[k-1];
	memset(&lk->all[k],0,sizeof(struct lkinfo));
	lk->all[k].lookup = otl;
	lk->all[k].new = true;
	lk->all[k].selected = true;
	++lk->cnt;
	if ( isgpos ) {
	    otl->next = gfi->sf->gpos_lookups;
	    gfi->sf->gpos_lookups = otl;
	} else {
	    otl->next = gfi->sf->gsub_lookups;
	    gfi->sf->gsub_lookups = otl;
	}

	/* Make sure the window is scrolled to display the new lookup */
	lcnt=0;
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    if ( i==k )
	break;
	    ++lcnt;
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		++lcnt;
	    }
	}
	if ( lcnt<lk->off_top || lcnt>=lk->off_top+(gfi->lkheight-2*LK_MARGIN)/gfi->fh )
	    lk->off_top = lcnt;

	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static int GFI_LookupAddSubtable(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j,k,lcnt;
	struct lookup_subtable *sub;

	lcnt = 0;
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    j = -1;
	    ++lcnt;
	    if ( lk->all[i].selected )
	break;
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		++lcnt;
		if ( lk->all[i].subtables[j].selected )
	goto break_2_loops;
	    }
	}
	break_2_loops:
	if ( i==lk->cnt )
return( true );

	sub = chunkalloc(sizeof(struct lookup_subtable));
	sub->lookup = lk->all[i].lookup;
	if ( !EditSubtable(sub,isgpos,gfi->sf,NULL,gfi->def_layer)) {
	    chunkfree(sub,sizeof(struct lookup_subtable));
return( true );
	}
	if ( lk->all[i].subtable_cnt>=lk->all[i].subtable_max )
	    lk->all[i].subtables = realloc(lk->all[i].subtables,(lk->all[i].subtable_max+=10)*sizeof(struct lksubinfo));
	for ( k=lk->all[i].subtable_cnt; k>j+1; --k )
	    lk->all[i].subtables[k] = lk->all[i].subtables[k-1];
	memset(&lk->all[i].subtables[k],0,sizeof(struct lksubinfo));
	lk->all[i].subtables[k].subtable = sub;
	lk->all[i].subtables[k].new = true;
	sub->next = lk->all[i].lookup->subtables;
	lk->all[i].lookup->subtables = sub;
	++lk->all[i].subtable_cnt;

	/* Make sure the window is scrolled to display the new subtable */
	if ( lcnt<lk->off_top || lcnt>=lk->off_top+(gfi->lkheight-2*LK_MARGIN)/gfi->fh )
	    lk->off_top = lcnt;

	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static int GFI_LookupEditMetadata(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j;

	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    if ( lk->all[i].selected ) {
		EditLookup(lk->all[i].lookup,isgpos,gfi->sf);
		GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos)),NULL,false);
return( true );
	    } else if ( lk->all[i].open ) {
		for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		    if ( lk->all[i].subtables[j].deleted )
		continue;
		    if ( lk->all[i].subtables[j].selected ) {
			EditSubtable(lk->all[i].subtables[j].subtable,isgpos,gfi->sf,NULL,gfi->def_layer);
			GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos)),NULL,false);
return( true );
		    }
		}
	    }
	}
    }
return( true );
}

static int GFI_LookupMergeLookup(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	char *buts[3];
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	struct selection_bits sel;
	int i,j;
	struct lkinfo *lkfirst;
	struct lksubinfo *sbfirst;
	struct lookup_subtable *sub;

	LookupParseSelection(lk,&sel);
	if ( !sel.sub_table_mergeable && !sel.lookup_mergeable )
return( true );

	buts[0] = _("Do it");
	buts[1] = _("_Cancel");
	buts[2] = NULL;
	if ( gwwv_ask(_("Cannot be Undone"),(const char **) buts,0,1,_("The Merge operation cannot be reverted.\nDo it anyway?"))==1 )
return( true );
	if ( sel.lookup_mergeable ) {
	    lkfirst = NULL;
	    for ( i=0; i<lk->cnt; ++i ) {
		if ( lk->all[i].selected && !lk->all[i].deleted ) {
		    if ( lkfirst==NULL )
			lkfirst = &lk->all[i];
		    else {
			FLMerge(lkfirst->lookup,lk->all[i].lookup);
			if ( lkfirst->subtable_cnt+lk->all[i].subtable_cnt >= lkfirst->subtable_max )
			    lkfirst->subtables = realloc(lkfirst->subtables,(lkfirst->subtable_max+=lk->all[i].subtable_cnt)*sizeof(struct lksubinfo));
			memcpy(lkfirst->subtables+lkfirst->subtable_cnt,
				lk->all[i].subtables,lk->all[i].subtable_cnt*sizeof(struct lksubinfo));
			lkfirst->subtable_cnt += lk->all[i].subtable_cnt;
			for ( j=0; j<lk->all[i].subtable_cnt; ++j )
			    lk->all[i].subtables[j].subtable->lookup = lkfirst->lookup;
			if ( lk->all[i].lookup->subtables!=NULL ) {
			    for ( sub = lk->all[i].lookup->subtables; sub->next!=NULL; sub = sub->next );
			    sub->next = lkfirst->lookup->subtables;
			    lkfirst->lookup->subtables = lk->all[i].lookup->subtables;
			    lk->all[i].lookup->subtables = NULL;
			}
			lk->all[i].subtable_cnt = 0;
			lk->all[i].deleted = true;
			lk->all[i].open = false;
			lk->all[i].selected = false;
		    }
		}
	    }
	} else if ( sel.sub_table_mergeable ) {
	    sbfirst = NULL;
	    for ( i=0; i<lk->cnt; ++i ) if ( !lk->all[i].deleted && lk->all[i].open ) {
		for ( j=0; j<lk->all[i].subtable_cnt; ++j ) if ( !lk->all[i].subtables[j].deleted ) {
		    if ( lk->all[i].subtables[j].selected ) {
			if ( sbfirst == NULL )
			    sbfirst = &lk->all[i].subtables[j];
			else {
			    SFSubTablesMerge(gfi->sf,sbfirst->subtable,lk->all[i].subtables[j].subtable);
			    lk->all[i].subtables[j].deleted = true;
			    lk->all[i].subtables[j].selected = false;
			}
		    }
		}
		if ( sbfirst!=NULL )	/* Can only merge subtables within a lookup, so if we found anything, in a lookup that's everything */
	    break;
	    }
	}
	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static int GFI_LookupDeleteLookup(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j;

	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    if ( lk->all[i].selected )
		lk->all[i].deleted = true;
	    else if ( lk->all[i].open ) {
		for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		    if ( lk->all[i].subtables[j].deleted )
		continue;
		    if ( lk->all[i].subtables[j].selected )
			lk->all[i].subtables[j].deleted = true;
		}
	    }
	}

	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }

return( true );
}

static int GFI_LookupRevertLookup(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	int i,j;

	/* First remove any new lookups, subtables */
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].new )
		SFRemoveLookup(gfi->sf,lk->all[i].lookup,0);
	    else {
		for ( j=0; j<lk->all[i].subtable_cnt; ++j )
		    if ( lk->all[i].subtables[j].new )
			SFRemoveLookupSubTable(gfi->sf,lk->all[i].subtables[j].subtable,0);
	    }
	}

	/* Now since we didn't actually delete anything we don't need to do */
	/*  anything to resurrect them */

	/* Finally we need to restore the original order. */
	/* But that just means regenerating the lk structure. So free it and */
	/*  regenerate it */

	LookupInfoFree(lk);
	LookupSetup(lk,isgpos?gfi->sf->gpos_lookups:gfi->sf->gsub_lookups);

	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static int import_e_h(GWindow gw, GEvent *event) {
    int *done = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	*done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("fontinfo.html#Lookups");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent ) {
	if ( event->u.control.subtype == et_buttonactivate ) {
	    switch ( GGadgetGetCid(event->u.control.g)) {
	      case CID_OK:
		*done = 2;
	      break;
	      case CID_Cancel:
		*done = true;
	      break;
	    }
	} else if ( event->u.control.subtype == et_listdoubleclick )
	    *done = 2;
    }
return( true );
}

static int GFI_LookupImportLookup(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	FontView *ofv;
	SplineFont *osf;
	OTLookup *otl;
	int i, j, cnt;
	GTextInfo *ti;
	GGadgetCreateData gcd[7], *varray[8], *harray[7];
	GTextInfo label[7];
	GWindowAttrs wattrs;
	GRect pos;
	GWindow gw;
	int done = 0;

	/* Figure out what lookups can be imported from which (open) fonts */
	ti = NULL;
	for ( j=0; j<2; ++j ) {
	    for ( ofv=fv_list; ofv!=NULL; ofv=(FontView *) (ofv->b.next) ) {
		osf = ofv->b.sf;
		if ( osf->cidmaster ) osf = osf->cidmaster;
		osf->ticked = false;
	    }
	    cnt = 0;
	    for ( ofv=fv_list; ofv!=NULL; ofv=(FontView *) (ofv->b.next) ) {
		osf = ofv->b.sf;
		if ( osf->cidmaster ) osf = osf->cidmaster;
		if ( osf->ticked || osf==gfi->sf || osf==gfi->sf->cidmaster ||
			( isgpos && osf->gpos_lookups==NULL) ||
			(!isgpos && osf->gsub_lookups==NULL) )
	    continue;
		osf->ticked = true;
		if ( cnt!=0 ) {
		    if ( ti )
			ti[cnt].line = true;
		    ++cnt;
		}
		if ( ti ) {
		    ti[cnt].text = (unichar_t *) copy( osf->fontname );
		    ti[cnt].text_is_1byte = true;
		    ti[cnt].disabled = true;
		    ti[cnt].userdata = osf;
		}
		++cnt;
		for ( otl = isgpos ? osf->gpos_lookups : osf->gsub_lookups; otl!=NULL; otl=otl->next ) {
		    if ( ti ) {
			ti[cnt].text = (unichar_t *) strconcat( " ", otl->lookup_name );
			ti[cnt].text_is_1byte = true;
			ti[cnt].userdata = otl;
		    }
		    ++cnt;
		}
	    }
	    if ( ti==NULL )
		ti = calloc((cnt+1),sizeof(GTextInfo));
	}

	memset(gcd,0,sizeof(gcd));
	memset(label,0,sizeof(label));

	i = 0;
	label[i].text = (unichar_t *) _("Select lookups from other fonts");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 12; gcd[i].gd.pos.y = 6+6;
	gcd[i].gd.flags = gg_visible | gg_enabled;
	gcd[i].creator = GLabelCreate;
	varray[0] = &gcd[i++]; varray[1] = NULL;

	gcd[i].gd.pos.height = 12*12+6;
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_list_multiplesel|gg_utf8_popup;
	gcd[i].gd.u.list = ti;
	gcd[i].creator = GListCreate;
	varray[2] = &gcd[i++]; varray[3] = NULL;

	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_Import");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = CID_OK;
	harray[0] = GCD_Glue; harray[1] = &gcd[i]; harray[2] = GCD_Glue;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = CID_Cancel;
	harray[3] = GCD_Glue; harray[4] = &gcd[i]; harray[5] = GCD_Glue; harray[6] = NULL;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.boxelements = harray;
	gcd[i].creator = GHBoxCreate;
	varray[4] = &gcd[i++]; varray[5] = NULL; varray[6] = NULL;

	gcd[i].gd.pos.x = gcd[i].gd.pos.y = 2;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.boxelements = varray;
	gcd[i].creator = GHVGroupCreate;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  _("Import Lookup");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,150));
	pos.height = GDrawPointsToPixels(NULL,193);
	gw = GDrawCreateTopWindow(NULL,&pos,import_e_h,&done,&wattrs);

	GGadgetsCreate(gw,&gcd[i]);
	GHVBoxSetExpandableRow(gcd[i].ret,1);
	GHVBoxSetExpandableCol(gcd[i-1].ret,gb_expandgluesame);
	GHVBoxFitWindow(gcd[i].ret);
	GTextInfoListFree(ti);
	GDrawSetVisible(gw,true);

	while ( !done )
	    GDrawProcessOneEvent(NULL);
	if ( done==2 ) {
	    int32 len;
	    GTextInfo **ti = GGadgetGetList(gcd[1].ret,&len);
	    OTLookup **list = malloc((len+1)*sizeof(OTLookup*));
	    struct lkdata *lk = &gfi->tables[isgpos];
	    OTLookup *before = NULL;

	    for ( i=0; i<lk->cnt; ++i ) {
		if ( lk->all[i].deleted )
	    continue;
		if ( lk->all[i].selected ) {
		    before = lk->all[i].lookup;
	    break;
		}
	    }
	    osf = NULL;
	    for ( i=0; i<len; ++i ) {
		if ( ti[i]->disabled )
		    osf = ti[i]->userdata;
		else if ( ti[i]->selected && ti[i]->text!=NULL ) {
		    for ( j=0; i+j<len && !ti[i+j]->disabled &&
			    ti[i+j]->selected && ti[i+j]->text; ++j )
			list[j] = (OTLookup *) ti[i+j]->userdata;
		    list[j] = NULL;
		    OTLookupsCopyInto(gfi->sf,osf,list,before);
		    i += j-1;
		}
	    }
	    free(list);
	}
	GDrawDestroyWindow(gw);

	GFI_LookupScrollbars(gfi,isgpos, true);
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static int GFI_LookupAspectChange(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	GFI_LookupEnableButtons(gfi,isgpos);
    }
return( true );
}

static void LookupExpose(GWindow pixmap, struct gfi_data *gfi, int isgpos) {
    int lcnt, i,j;
    struct lkdata *lk = &gfi->tables[isgpos];
    GRect r, old;
    extern GBox _ggadget_Default_Box;

    r.x = LK_MARGIN; r.width = gfi->lkwidth-2*LK_MARGIN;
    r.y = LK_MARGIN; r.height = gfi->lkheight-2*LK_MARGIN;
    GDrawPushClip(pixmap,&r,&old);
    GDrawSetFont(pixmap,gfi->font);

    lcnt = 0;
    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	if ( lcnt>=lk->off_top ) {
	    if ( lk->all[i].selected ) {
		r.x = LK_MARGIN; r.width = gfi->lkwidth-2*LK_MARGIN;
		r.y = (lcnt-lk->off_top)*gfi->fh; r.height = gfi->fh;
		GDrawFillRect(pixmap,&r,ACTIVE_BORDER);
	    }
	    r.x = LK_MARGIN-lk->off_left; r.width = (gfi->as&~1);
	    r.y = LK_MARGIN+(lcnt-lk->off_top)*gfi->fh; r.height = r.width;
	    GDrawDrawRect(pixmap,&r,0x000000);
	    GDrawDrawLine(pixmap,r.x+2,r.y+(r.height/2), r.x+r.width-2,r.y+(r.height/2), 0x000000);
	    if ( !lk->all[i].open )
		GDrawDrawLine(pixmap,r.x+(r.width/2),r.y+2, r.x+(r.width/2),r.y+r.height-2, 0x000000);
	    GDrawDrawText8(pixmap,r.x+gfi->fh, r.y+gfi->as,
		    lk->all[i].lookup->lookup_name,-1,MAIN_FOREGROUND);
	}
	++lcnt;
	if ( lk->all[i].open ) {
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		if ( lcnt>=lk->off_top ) {
		    if ( lk->all[i].subtables[j].selected ) {
			r.x = LK_MARGIN; r.width = gfi->lkwidth-2*LK_MARGIN;
			r.y = LK_MARGIN+(lcnt-lk->off_top)*gfi->fh; r.height = gfi->fh;
			GDrawFillRect(pixmap,&r,ACTIVE_BORDER);
		    }
		    r.x = LK_MARGIN+2*gfi->fh-lk->off_left;
		    r.y = LK_MARGIN+(lcnt-lk->off_top)*gfi->fh;
		    GDrawDrawText8(pixmap,r.x, r.y+gfi->as,
			    lk->all[i].subtables[j].subtable->subtable_name,-1,MAIN_FOREGROUND);
		}
		++lcnt;
	    }
	}
    }
    GDrawPopClip(pixmap,&old);
}

static void LookupDeselect(struct lkdata *lk) {
    int i,j;

    for ( i=0; i<lk->cnt; ++i ) {
	lk->all[i].selected = false;
	for ( j=0; j<lk->all[i].subtable_cnt; ++j )
	    lk->all[i].subtables[j].selected = false;
    }
}

static void LookupPopup(GWindow gw,OTLookup *otl,struct lookup_subtable *sub,
	struct lkdata *lk) {
    static char popup_msg[600];
    int pos;
    char *lookuptype;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int l;

    if ( (otl->lookup_type&0xff)>= 0xf0 ) {
	if ( otl->lookup_type==kern_statemachine )
	    lookuptype = _("Kerning State Machine");
	else if ( otl->lookup_type==morx_indic )
	    lookuptype = _("Indic State Machine");
	else if ( otl->lookup_type==morx_context )
	    lookuptype = _("Contextual State Machine");
	else
	    lookuptype = _("Contextual State Machine");
    } else if ( (otl->lookup_type>>8)<2 && (otl->lookup_type&0xff)<10 )
	lookuptype = _(lookup_type_names[otl->lookup_type>>8][otl->lookup_type&0xff]);
    else
	lookuptype = S_("LookupType|Unknown");
    snprintf(popup_msg,sizeof(popup_msg), "%s\n", lookuptype);
    pos = strlen(popup_msg);

    if ( sub!=NULL && otl->lookup_type==gpos_pair && sub->kc!=NULL ) {
	snprintf(popup_msg+pos,sizeof(popup_msg)-pos,"%s",_("(kerning class)\n") );
	pos += strlen( popup_msg+pos );
    }

    if ( otl->features==NULL )
	snprintf(popup_msg+pos,sizeof(popup_msg)-pos,"%s",_("Not attached to a feature"));
    else {
	for ( fl=otl->features; fl!=NULL && pos<sizeof(popup_msg)-2; fl=fl->next ) {
	    snprintf(popup_msg+pos,sizeof(popup_msg)-pos,"%c%c%c%c: ",
		    fl->featuretag>>24, fl->featuretag>>16,
		    fl->featuretag>>8, fl->featuretag&0xff );
	    pos += strlen( popup_msg+pos );
	    for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
		snprintf(popup_msg+pos,sizeof(popup_msg)-pos,"%c%c%c%c{",
			sl->script>>24, sl->script>>16,
			sl->script>>8, sl->script&0xff );
		pos += strlen( popup_msg+pos );
		for ( l=0; l<sl->lang_cnt; ++l ) {
		    uint32 lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
		    snprintf(popup_msg+pos,sizeof(popup_msg)-pos,"%c%c%c%c,",
			    lang>>24, lang>>16,
			    lang>>8, lang&0xff );
		    pos += strlen( popup_msg+pos );
		}
		if ( popup_msg[pos-1]==',' )
		    popup_msg[pos-1] = '}';
		else if ( pos<sizeof(popup_msg)-2 )
		    popup_msg[pos++] = '}';
		if ( pos<sizeof(popup_msg)-2 )
		    popup_msg[pos++] = ' ';
	    }
	    if ( pos<sizeof(popup_msg)-2 )
		popup_msg[pos++] = '\n';
	}
    }

    /* Find out if any other lookups invoke it (ie. a context lookup invokes */
    /*  does it invoke ME?) */
    for ( l=0; l<lk->cnt; ++l ) {
	int j, r, used = false;
	if ( lk->all[l].deleted )
    continue;
	for ( j=0; j<lk->all[l].subtable_cnt && !used; ++j ) {
	    struct lookup_subtable *sub = lk->all[l].subtables[j].subtable;
	    if ( lk->all[l].subtables[j].deleted )
    continue;
	    if ( sub->fpst!=NULL ) {
		for ( r=0; r<sub->fpst->rule_cnt && !used; ++r ) {
		    struct fpst_rule *rule = &sub->fpst->rules[r];
		    int ls;
		    for ( ls = 0; ls<rule->lookup_cnt; ++ls ) {
			if ( rule->lookups[ls].lookup == otl ) {
			    used = true;
		    break;
			}
		    }
		}
	    }
	}
	if ( used ) {
	    snprintf(popup_msg+pos,sizeof(popup_msg)-pos,_(" Used in %s\n"),
		    lk->all[l].lookup->lookup_name );
	    pos += strlen( popup_msg+pos );
	}
    }

    if ( pos>=sizeof(popup_msg) )
	pos = sizeof(popup_msg)-1;
    popup_msg[pos]='\0';
    GGadgetPreparePopup8(gw,popup_msg);
}

static void AddDFLT(OTLookup *otl) {
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int l;

    for ( fl = otl->features; fl!=NULL; fl=fl->next ) {
	int hasDFLT = false, hasdflt=false;
	for ( sl=fl->scripts; sl!=NULL; sl=sl->next ) {
	    if ( sl->script == DEFAULT_SCRIPT )
		hasDFLT = true;
	    for ( l=0; l<sl->lang_cnt; ++l ) {
		uint32 lang = l<MAX_LANG ? sl->langs[l] : sl->morelangs[l-MAX_LANG];
		if ( lang==DEFAULT_LANG ) {
		    hasdflt = true;
	    break;
		}
	    }
	    if ( hasdflt && hasDFLT )
	break;
	}
	if ( hasDFLT	/* Already there */ ||
		!hasdflt /* Shouldn't add it */ )
    continue;
	sl = chunkalloc(sizeof(struct scriptlanglist));
	sl->script = DEFAULT_SCRIPT;
	sl->lang_cnt = 1;
	sl->langs[0] = DEFAULT_LANG;
	sl->next = fl->scripts;
	fl->scripts = sl;
    }
}

static void AALTRemoveOld(SplineFont *sf,struct lkdata *lk) {
    int i;
    FeatureScriptLangList *fl, *prev;

    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	prev = NULL;
	for ( fl = lk->all[i].lookup->features; fl!=NULL; prev=fl, fl=fl->next ) {
	    if ( fl->featuretag==CHR('a','a','l','t') ) {
		if ( fl==lk->all[i].lookup->features && fl->next==NULL && !LookupUsedNested(sf,lk->all[i].lookup) )
		    lk->all[i].deleted = true;
		else {
		    if ( prev==NULL )
			lk->all[i].lookup->features = fl->next;
		    else
			prev->next = fl->next;
		    fl->next = NULL;
		    FeatureScriptLangListFree(fl);
		}
	break;
	    }
	}
    }
}

static void AALTCreateNew(SplineFont *sf, struct lkdata *lk) {
    /* different script/lang combinations may need different 'aalt' lookups */
    /*  well, let's just say different script combinations */
    /* for each script/lang combo find all single/alternate subs for each */
    /*  glyph. Merge those choices and create new lookup with that info */
    struct sllk *sllk = NULL;
    int sllk_cnt=0, sllk_max = 0;
    int i,k;
    OTLookup *otl;

    /* Find all scripts, and all the single/alternate lookups for each */
    /*  and all the languages used for these in each script */
    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	sllk = AddOTLToSllks( lk->all[i].lookup, sllk, &sllk_cnt, &sllk_max );
    }
    /* Each of these gets its own gsub_alternate lookup which gets inserted */
    /*  at the head of the lookup list. Each lookup has one subtable */
    for ( i=0; i<sllk_cnt; ++i ) {
	if ( sllk[i].cnt==0 )		/* Script used, but provides no alternates */
    continue;
	otl = NewAALTLookup(sf,sllk,sllk_cnt,i);
	if ( lk->cnt>=lk->max )
	    lk->all = realloc(lk->all,(lk->max+=10)*sizeof(struct lkinfo));
	for ( k=lk->cnt; k>0; --k )
	    lk->all[k] = lk->all[k-1];
	memset(&lk->all[0],0,sizeof(struct lkinfo));
	lk->all[0].lookup = otl;
	lk->all[0].new = true;
	++lk->cnt;

	/* Now add the new subtable */
	lk->all[0].subtables = calloc(1,sizeof(struct lksubinfo));
	lk->all[0].subtable_cnt = lk->all[0].subtable_max = 1;
	lk->all[0].subtables[0].subtable = otl->subtables;
	lk->all[0].subtables[0].new = true;
    }

    SllkFree(sllk,sllk_cnt);
}

static void lookupmenu_dispatch(GWindow v, GMenuItem *mi, GEvent *e) {
    GEvent dummy;
    struct gfi_data *gfi = GDrawGetUserData(v);
    int i;
    char *buts[4];

    if ( mi->mid==CID_SaveFeat || mi->mid==CID_SaveLookup ) {
	char *filename, *defname;
	FILE *out;
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];
	OTLookup *otl = NULL;

	if ( mi->mid==CID_SaveLookup ) {
	    for ( i=0; i<lk->cnt && (!lk->all[i].selected || lk->all[i].deleted); ++i );
	    if ( i==lk->cnt )
return;
	    otl = lk->all[i].lookup;
	    SFFindUnusedLookups(gfi->sf);
	    if ( otl->unused ) {
		ff_post_error(_("No data"), _("This lookup contains no data"));
return;
	    }
	}
	defname = strconcat(gfi->sf->fontname,".fea");
	filename = gwwv_save_filename(_("Feature file?"),defname,"*.fea");
	free(defname);
	if ( filename==NULL )
return;
	/* Convert to def encoding !!! */
	out = fopen(filename,"w");
	if ( out==NULL ) {
	    ff_post_error(_("Cannot open file"),_("Cannot open %s"), filename );
	    free(filename);
return;
	}
	if ( otl!=NULL )
	    FeatDumpOneLookup( out,gfi->sf,otl );
	else
	    FeatDumpFontLookups( out,gfi->sf );
	if ( ferror(out)) {
	    ff_post_error(_("Output error"),_("An error occurred writing %s"), filename );
	    free(filename);
	    fclose(out);
return;
	}
	free(filename);
	fclose(out);
    } else if ( mi->mid==CID_AddAllAlternates ) {
	/* First we want to remove any old aalt-only lookups */
	/*  (and remove the 'aalt' tag from any lookups with multiple features) */
	/* Then add the new ones */
	struct lkdata *lk = &gfi->tables[0];		/* GSUB */
	int has_aalt_only=false, has_aalt_mixed = false;
	int i, ret;
	FeatureScriptLangList *fl;

	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    for ( fl = lk->all[i].lookup->features; fl!=NULL; fl=fl->next ) {
		if ( fl->featuretag==CHR('a','a','l','t') ) {
		    if ( fl==lk->all[i].lookup->features && fl->next==NULL )
			has_aalt_only = true;
		    else
			has_aalt_mixed = true;
	    break;
		}
	    }
	}
	if ( has_aalt_only || has_aalt_mixed ) {
	    buts[0] = _("_OK"); buts[1] = _("_Cancel"); buts[2] = NULL;
	    ret = gwwv_ask(has_aalt_only?_("Lookups will be removed"):_("Feature tags will be removed"),
		    (const char **) buts,0,1,
		    !has_aalt_mixed ?
		    _("Warning: There are already some 'aalt' lookups in\n"
		      "the font. If you proceed with this command those\n"
		      "lookups will be removed and new lookups will be\n"
		      "generated. The old information will be LOST.\n"
		      " Is that what you want?") :
		    !has_aalt_only ?
		    _("Warning: There are already some 'aalt' lookups in\n"
		      "the font but there are other feature tags associated\n"
		      "with these lookups. If you proceed with this command\n"
		      "the 'aalt' tag will be removed from those lookups,\n"
		      "and new lookups will be generate which will NOT be\n"
		      "associated with the other feature tag(s).\n"
		      " Is that what you want?") :
		    _("Warning: There are already some 'aalt' lookups in\n"
		      "the font, some have no other feature tags associated\n"
		      "with them and these will be removed, others have other\n"
		      "tags associated and these will remain while the 'aalt'\n"
		      "tag will be removed from the lookup -- a new lookup\n"
		      "will be generated which is not associated with any\n"
		      "other feature tags.\n"
		      " Is that what you want?") );
	    if ( ret==1 )
return;
	    AALTRemoveOld(gfi->sf,lk);
	}
	AALTCreateNew(gfi->sf,lk);
	GDrawRequestExpose(GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+0)),NULL,false);
    } else if ( mi->mid==CID_AddDFLT ) {
	struct selection_bits sel;
	int toall, ret, i;
	char *buts[4];
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];

	LookupParseSelection(lk,&sel);
	if ( sel.lookup_cnt==0 ) {
	    buts[0] = _("_Apply to All"); buts[1] = _("_Cancel"); buts[2]=NULL;
	} else {
	    buts[0] = _("_Apply to All"); buts[1] = _("_Apply to Selection"); buts[2] = _("_Cancel"); buts[3]=NULL;
	}
	ret = gwwv_ask(_("Apply to:"),(const char **) buts,0,sel.lookup_cnt==0?1:2,_("Apply change to which lookups?"));
	toall = false;
	if ( ret==0 )
	    toall = true;
	else if ( (ret==1 && sel.lookup_cnt==0) || (ret==2 && sel.lookup_cnt!=0))
return;
	for ( i=0; i<lk->cnt; ++i ) {
	    if ( lk->all[i].deleted )
	continue;
	    if ( lk->all[i].selected || toall ) {
		AddDFLT(lk->all[i].lookup);
	    }
	}
    } else if ( mi->mid==CID_AddLanguage || mi->mid==CID_RmLanguage ) {
	int isgpos = GTabSetGetSel(GWidgetGetControl(gfi->gw,CID_Lookups));
	struct lkdata *lk = &gfi->tables[isgpos];

	AddRmLang(gfi->sf,lk,mi->mid==CID_AddLanguage);
    } else {
	memset(&dummy,0,sizeof(dummy));
	dummy.type = et_controlevent;
	dummy.u.control.subtype = et_buttonactivate;
	dummy.u.control.g = GWidgetGetControl(gfi->gw,mi->mid);
	dummy.w = GGadgetGetWindow(dummy.u.control.g);
	dummy.u.control.u.button.button = e->u.mouse.button;
	GGadgetDispatchEvent(dummy.u.control.g,&dummy);
    }
}

static GMenuItem lookuppopupmenu[] = {
    { { (unichar_t *) N_("_Top"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_LookupTop },
    { { (unichar_t *) N_("_Up"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_LookupUp },
    { { (unichar_t *) N_("_Down"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_LookupDown },
    { { (unichar_t *) N_("_Bottom"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_LookupBottom },
    { { (unichar_t *) N_("_Sort"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_LookupSort },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Add _Lookup"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_AddLookup },
    { { (unichar_t *) N_("Add Sub_table"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_AddSubtable },
    { { (unichar_t *) N_("Edit _Metadata"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_EditMetadata },
    { { (unichar_t *) N_("_Edit Data"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_EditSubtable },
    { { (unichar_t *) N_("De_lete"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_DeleteLookup },
    { { (unichar_t *) N_("_Merge"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_MergeLookup },
    { { (unichar_t *) N_("Sa_ve Lookup..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_SaveLookup },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("Add Language to Script..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_AddLanguage },
    { { (unichar_t *) N_("Remove Language from Script..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_RmLanguage },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Add 'aalt' features"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_AddAllAlternates },
    { { (unichar_t *) N_("Add 'D_FLT' script"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_AddDFLT },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, 0, '\0' }, '\0', 0, NULL, NULL, NULL, 0 }, /* line */
    { { (unichar_t *) N_("_Revert All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_RevertLookups },
    { { (unichar_t *) N_("_Import..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_ImportLookups },
    { { (unichar_t *) N_("S_ave Feature File..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, '\0', ksm_control, NULL, NULL, lookupmenu_dispatch, CID_SaveFeat },
    GMENUITEM_EMPTY
};

static void LookupMenu(struct gfi_data *gfi,struct lkdata *lk,int isgpos, GEvent *event) {
    struct selection_bits sel;
    int i,j;
    static int initted = false;

    if ( !initted ) {
        int i;
        initted = true;

        for ( i=0; lookuppopupmenu[i].ti.text!=NULL || lookuppopupmenu[i].ti.line; ++i )
            if ( lookuppopupmenu[i].ti.text!=NULL )
                lookuppopupmenu[i].ti.text = (unichar_t *) _( (char *)lookuppopupmenu[i].ti.text);
    }

    GGadgetEndPopup();

    LookupParseSelection(lk,&sel);
    for ( i=0; lookuppopupmenu[i].ti.text!=NULL || lookuppopupmenu[i].ti.line; ++i ) {
	switch ( lookuppopupmenu[i].mid ) {
	  case 0:
	    /* Lines */;
	  break;
	  case CID_LookupTop:
	    lookuppopupmenu[i].ti.disabled = sel.any_first || sel.lookup_cnt+sel.sub_cnt!=1;
	  break;
	  case CID_LookupUp:
	    lookuppopupmenu[i].ti.disabled = sel.any_first || sel.lookup_cnt+sel.sub_cnt==0;
	  break;
	  case CID_LookupDown:
	    lookuppopupmenu[i].ti.disabled = sel.any_last || sel.lookup_cnt+sel.sub_cnt==0;
	  break;
	  case CID_LookupBottom:
	    lookuppopupmenu[i].ti.disabled = sel.any_last || sel.lookup_cnt+sel.sub_cnt!=1;
	  break;
	  case CID_LookupSort:
	    lookuppopupmenu[i].ti.disabled = lk->cnt<=1;
	  break;
	  case CID_AddLookup:
	    lookuppopupmenu[i].ti.disabled = sel.lookup_cnt+sel.sub_cnt>1;
	  break;
	  case CID_AddSubtable:
	    lookuppopupmenu[i].ti.disabled = (sel.lookup_cnt!=1 || sel.sub_cnt>1) && (sel.lookup_cnt!=0 || sel.sub_cnt!=1);
	  break;
	  case CID_EditMetadata:
	    lookuppopupmenu[i].ti.disabled = (sel.lookup_cnt!=1 || sel.sub_cnt!=0) &&
			(sel.lookup_cnt!=0 || sel.sub_cnt!=1);
	  break;
	  case CID_EditSubtable:
	    lookuppopupmenu[i].ti.disabled = sel.lookup_cnt!=0 || sel.sub_cnt!=1;
	  break;
	  case CID_DeleteLookup:
	    lookuppopupmenu[i].ti.disabled = sel.lookup_cnt==0 && sel.sub_cnt==0;
	  break;
	  case CID_MergeLookup:
	    lookuppopupmenu[i].ti.disabled = !(
		    (sel.lookup_cnt>=2 && sel.sub_cnt==0 && sel.lookup_mergeable) ||
		    (sel.lookup_cnt==0 && sel.sub_cnt>=2 && sel.sub_table_mergeable)  );
	  break;
	  case CID_ImportLookups:
	    lookuppopupmenu[i].ti.disabled = !LookupsImportable(gfi->sf,isgpos);
	  break;
	  case CID_RevertLookups:
	    lookuppopupmenu[i].ti.disabled = false;
	  break;
	  case CID_SaveLookup:
	    lookuppopupmenu[i].ti.disabled = sel.lookup_cnt!=1 || sel.sub_cnt!=0;
	    for ( j=0; j<lk->cnt; ++j ) if ( lk->all[j].selected ) {
		int type = lk->all[j].lookup->lookup_type;
		if ( type==kern_statemachine || type==morx_indic ||
			type==morx_context || type==morx_insert )
		    lookuppopupmenu[i].ti.disabled = true;
	    break;
	    }
	  break;
	  case CID_AddDFLT:
	    lookuppopupmenu[i].ti.disabled = lk->cnt==0;
	  break;
	  case CID_AddAllAlternates:
	    lookuppopupmenu[i].ti.disabled = lk->cnt==0 || lk==&gfi->tables[1]/*Only applies to GSUB*/;
	  break;
	  case CID_SaveFeat:
	    lookuppopupmenu[i].ti.disabled = lk->cnt<=0;
	  break;
	}
    }
    GMenuCreatePopupMenu(event->w,event, lookuppopupmenu);
}


static int LookupIndex(struct gfi_data *othergfi, int isgpos, GWindow gw,
	GWindow othergw, GEvent *event, int *subindex) {
    GPoint pt;
    struct lkdata *otherlk = &othergfi->tables[isgpos];
    int l,i,j, lcnt;

    pt.x = event->u.mouse.x; pt.y = event->u.mouse.y;
    if ( gw!=othergw )
	GDrawTranslateCoordinates(gw,othergw,&pt);
    l = (pt.y-LK_MARGIN)/othergfi->fh + otherlk->off_top;

    if ( l<0 ) {
	*subindex = -1;
return( -1 );
    }

    lcnt = 0;
    for ( i=0; i<otherlk->cnt; ++i ) {
	if ( otherlk->all[i].deleted )
    continue;
	if ( l==lcnt ) {
	    *subindex = -1;
return( i );
	}
	++lcnt;
	if ( otherlk->all[i].open ) {
	    for ( j=0; j<otherlk->all[i].subtable_cnt; ++j ) {
		if ( otherlk->all[i].subtables[j].deleted )
	    continue;
		if ( l==lcnt ) {
		    *subindex = j;
return( i );
		}
		++lcnt;
	    }
	}
    }
    /* drop after last lookup */
    *subindex = -1;
return( i );
}

static int LookupsDropable(struct gfi_data *gfi, struct gfi_data *othergfi,
	int isgpos, GWindow gw, GWindow othergw, GEvent *event) {
    int lookup, subtable;
    struct lkdata *lk = &gfi->tables[isgpos];
    struct lkdata *otherlk = &othergfi->tables[isgpos];

    if ( gfi->first_sel_subtable==-1 )
return( true );			/* Lookups can be dropped anywhere */

    lookup = LookupIndex(othergfi,isgpos,gw,othergw,event,&subtable);
    if ( lookup<0 || lookup>=lk->cnt )
return( false );		/* Subtables must be dropped in a lookup */

    if ( gfi!=othergfi /* || lookup!=gfi->first_sel_lookup*/ )
return( false );		/* Can't merge subtables from one lookup to another */

    /* Subtables can only be dropped in a lookup with the same type */
    /* At the moment we don't need this test, as we only allow subtables to be dropped within their original lookup */
return( otherlk->all[lookup].lookup->lookup_type ==
		lk->all[gfi->first_sel_lookup].lookup->lookup_type );
}

static void LookupDragDrop(struct gfi_data *gfi, int isgpos, GEvent *event) {
    FontViewBase *fv;
    struct gfi_data *othergfi=NULL;
    GWindow gw = GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos));
    GWindow othergw = NULL;
    int i,j;

    othergw = GDrawGetPointerWindow(gfi->gw);

    for ( fv = (FontViewBase *) fv_list; fv!=NULL; fv=fv->next ) if ( fv->sf->fontinfo!=NULL ) {
	int otherisgpos;

	othergfi = fv->sf->fontinfo;
	otherisgpos = GTabSetGetSel(GWidgetGetControl(othergfi->gw,CID_Lookups));
	if ( otherisgpos!=isgpos )
    continue;
	if ( othergw == GDrawableGetWindow(GWidgetGetControl(othergfi->gw,CID_LookupWin+otherisgpos)))
    break;
    }
    if ( fv==NULL )
	othergw = NULL;

    if ( fv==NULL || !LookupsDropable(gfi,othergfi,isgpos,gw,othergw,event)) {
	if ( event->type==et_mouseup ) {
	    GDrawSetCursor(gw,ct_mypointer);
	    gfi->lk_drag_and_drop = gfi->lk_dropablecursor = false;
return;
	} else {
	    if ( gfi->lk_dropablecursor ) {
		gfi->lk_dropablecursor = false;
		GDrawSetCursor(gw,ct_prohibition);
	    }
return;
	}
    } else {
	if ( event->type==et_mousemove ) {
	    if ( !gfi->lk_dropablecursor ) {
		gfi->lk_dropablecursor = true;
		GDrawSetCursor(gw,ct_features);
	    }
return;
	} else {
	    struct lkdata *otherlk = &othergfi->tables[isgpos];
	    struct lkdata *lk = &gfi->tables[isgpos];
	    struct lkinfo temp;
	    struct lksubinfo subtemp;
	    int lookup, subtable;

	    lookup = LookupIndex(othergfi,isgpos,gw,othergw,event,&subtable);

	    if ( gfi==othergfi ) {
		/* dropping in the same window just moves things around */
		if ( gfi->first_sel_subtable == -1 ) {
		    /* Moving lookups */
		    if ( lookup<0 ) lookup=0;
		    else if ( lookup>=lk->cnt ) lookup=lk->cnt;
		    if ( lookup==gfi->first_sel_lookup )
  goto done_drop;
		    for ( i=0; i<lk->cnt; ++i )
			lk->all[i].moved = false;
		    for ( i=gfi->first_sel_lookup; i<lk->cnt; ++i ) {
			if ( lk->all[i].deleted || !lk->all[i].selected ||
				lk->all[i].moved || i==lookup )
		    continue;
			temp = lk->all[i];
			temp.moved = true;
			if ( i<lookup ) {
			    for ( j=i+1; j<lookup; ++j )
				lk->all[j-1] = lk->all[j];
			    lk->all[j-1] = temp;
			} else {
			    for ( j=i; j>lookup; --j )
				lk->all[j] = lk->all[j-1];
			    lk->all[j] = temp;
			}
			--i;
		    }
		} else if ( lookup==gfi->first_sel_lookup ) {
		    if ( subtable< 0 ) subtable=0;
		    if ( subtable==gfi->first_sel_subtable )
  goto done_drop;
		    for ( i=0; i<lk->cnt; ++i )
			lk->all[lookup].subtables[i].moved = false;
		    for ( i=gfi->first_sel_subtable; i<lk->all[lookup].subtable_cnt; ++i ) {
			if ( lk->all[lookup].subtables[i].deleted || !lk->all[lookup].subtables[i].selected ||
				lk->all[lookup].subtables[i].moved || i==subtable )
		    continue;
			subtemp = lk->all[lookup].subtables[i];
			subtemp.moved = true;
			if ( i<subtable ) {
			    for ( j=i+1; j<subtable; ++j )
				lk->all[lookup].subtables[j-1] = lk->all[lookup].subtables[j];
			    lk->all[lookup].subtables[j-1] = subtemp;
			} else {
			    for ( j=i; j>subtable; --j )
				lk->all[lookup].subtables[j] = lk->all[lookup].subtables[j-1];
			    lk->all[lookup].subtables[j] = subtemp;
			}
			--i;
		    }
		} else {
		    int old_lk = gfi->first_sel_lookup;
		    int sel_cnt=0, k;
		    if ( subtable<0 ) /* dropped into lookup */
			subtable = 0;
		    for ( i=gfi->first_sel_subtable; i<lk->all[old_lk].subtable_cnt; ++i )
			if ( !lk->all[old_lk].subtables[i].deleted && lk->all[old_lk].subtables[i].selected )
			    ++sel_cnt;
		    if ( lk->all[lookup].subtable_cnt+sel_cnt >= lk->all[lookup].subtable_max )
			lk->all[lookup].subtables = realloc(lk->all[lookup].subtables,
				(lk->all[lookup].subtable_max += sel_cnt+3)*sizeof(struct lksubinfo));
		    for ( i= lk->all[lookup].subtable_cnt+sel_cnt-1; i>=subtable; --i )
			lk->all[lookup].subtables[i] = lk->all[lookup].subtables[i-sel_cnt];
		    k=0;
		    for ( i=gfi->first_sel_subtable; i<lk->all[old_lk].subtable_cnt; ++i ) {
			if ( !lk->all[old_lk].subtables[i].deleted && lk->all[old_lk].subtables[i].selected )
			    lk->all[lookup].subtables[subtable+k++] = lk->all[old_lk].subtables[i];
		    }
		    lk->all[lookup].subtable_cnt += k;
		    k=0;
		    for ( i=gfi->first_sel_subtable; i<lk->all[old_lk].subtable_cnt; ++i ) {
			if ( !lk->all[old_lk].subtables[i].deleted && lk->all[old_lk].subtables[i].selected )
			    ++k;
			else if ( k!=0 )
			    lk->all[old_lk].subtables[i-k] = lk->all[old_lk].subtables[i];
		    }
		    lk->all[old_lk].subtable_cnt -= k;
		}
	    } else {
		if ( gfi->first_sel_subtable == -1 ) {
		    /* Import lookups from one font to another */
		    OTLookup **list, *before;
		    int cnt=0;
		    for ( i=0; i<lk->cnt; ++i ) {
			if ( lk->all[i].deleted )
		    continue;
			if ( lk->all[i].selected )
			    ++cnt;
		    }
		    list = malloc((cnt+1)*sizeof(OTLookup *));
		    cnt=0;
		    for ( i=0; i<lk->cnt; ++i ) {
			if ( lk->all[i].deleted )
		    continue;
			if ( lk->all[i].selected )
			    list[cnt++] = lk->all[i].lookup;
		    }
		    list[cnt] = NULL;
		    if ( lookup<0 ) lookup=0;
		    before = lookup>=otherlk->cnt ? NULL : otherlk->all[lookup].lookup;
		    OTLookupsCopyInto(othergfi->sf,gfi->sf,list,before);
		    free( list );
		} else
		    IError("Attempt to merge subtables across fonts not permitted" );
		GDrawRequestExpose(othergw,NULL,false);
	    }
  done_drop:
	    GDrawSetCursor(gw,ct_mypointer);
	    GDrawRequestExpose(gw,NULL,false);
	    gfi->lk_drag_and_drop = gfi->lk_dropablecursor = false;
return;
	}
    }
}

static int LookupDragDropable(struct gfi_data *gfi, struct lkdata *lk) {
    int i,j;
    int first_l= -1, first_s= -1;

    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	if ( lk->all[i].selected ) {
	    if ( first_s!=-1 )
return( false );		/* Can't have mixed lookups and subtables */
	    if ( first_l==-1 )
		first_l = i;
	}
	if ( lk->all[i].open ) {
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		if ( lk->all[i].subtables[j].selected ) {
		    if ( first_l==i || first_l==-1 )
			first_l = i;
		    else
return( false );		/* Mixed lookups and subtables */
		    if ( first_s==-1 )
			first_s = j;
		}
	    }
	}
    }
    gfi->first_sel_lookup   = first_l;
    gfi->first_sel_subtable = first_s;
return( true );
}

static void LookupMouse(struct gfi_data *gfi, int isgpos, GEvent *event) {
    struct lkdata *lk = &gfi->tables[isgpos];
    int l = (event->u.mouse.y-LK_MARGIN)/gfi->fh + lk->off_top;
    int inbox = event->u.mouse.x>=LK_MARGIN &&
	    event->u.mouse.x>=LK_MARGIN-lk->off_left &&
	    event->u.mouse.x<=LK_MARGIN-lk->off_left+gfi->as+1;
    GWindow gw = GDrawableGetWindow(GWidgetGetControl(gfi->gw,CID_LookupWin+isgpos));
    int i,j,lcnt;

    if ( event->type!=et_mousedown && gfi->lk_drag_and_drop ) {
	LookupDragDrop(gfi,isgpos,event);
return;
    }

    if ( l<0 || event->u.mouse.y>=(gfi->lkheight-2*LK_MARGIN) )
return;

    lcnt = 0;
    for ( i=0; i<lk->cnt; ++i ) {
	if ( lk->all[i].deleted )
    continue;
	if ( l==lcnt ) {
	    if ( event->type==et_mouseup )
return;
	    else if ( event->type==et_mousemove ) {
		LookupPopup(gw,lk->all[i].lookup,NULL,lk);
return;
	    } else {
		if ( gfi->lk_drag_and_drop ) {
		    GDrawSetCursor(gw,ct_mypointer);
		    gfi->lk_drag_and_drop = gfi->lk_dropablecursor = false;
		}
		if ( inbox || event->u.mouse.clicks>1 ) {
		    lk->all[i].open = !lk->all[i].open;
		    GFI_LookupScrollbars(gfi, isgpos, true);
return;
		}
		if ( !(event->u.mouse.state&(ksm_shift|ksm_control)) ) {
		    /* If line is selected, and we're going to pop up a menu */
		    /*  then don't clear other selected lines */
		    if ( !lk->all[i].selected ||
			    (event->u.mouse.button!=3 && !LookupDragDropable(gfi,lk)))
			LookupDeselect(lk);
		    else if ( event->u.mouse.button!=3 ) {
			gfi->lk_drag_and_drop = gfi->lk_dropablecursor = true;
			GDrawSetCursor(gw,ct_features);
		    }
		    lk->all[i].selected = true;
		} else if ( event->u.mouse.state&ksm_control ) {
		    lk->all[i].selected = !lk->all[i].selected;
		} else if ( lk->all[i].selected ) {
		    lk->all[i].selected = false;
		} else {
		    for ( j=0; j<lk->cnt; ++j )
			if ( !lk->all[j].deleted  && lk->all[j].selected )
		    break;
		    if ( j==lk->cnt )
			lk->all[i].selected = true;
		    else if ( j<i ) {
			for ( ; j<=i ; ++j )
			    if ( !lk->all[j].deleted )
				lk->all[j].selected = true;
		    } else {
			for ( ; j>=i ; --j )
			    if ( !lk->all[j].deleted )
				lk->all[j].selected = true;
		    }
		}
		GFI_LookupEnableButtons(gfi,isgpos);
		GDrawRequestExpose(gw,NULL,false);
		if ( event->u.mouse.button==3 )
		    LookupMenu(gfi,lk,isgpos,event);
return;
	    }
	}
	++lcnt;
	if ( lk->all[i].open ) {
	    for ( j=0; j<lk->all[i].subtable_cnt; ++j ) {
		if ( lk->all[i].subtables[j].deleted )
	    continue;
		if ( l==lcnt ) {
		    if ( event->type==et_mouseup )
return;
		    else if ( event->type==et_mousemove ) {
			LookupPopup(gw,lk->all[i].lookup,lk->all[i].subtables[j].subtable,lk);
return;
		    } else {
			if ( inbox )
return;		/* Can't open this guy */
			if ( event->u.mouse.clicks>1 )
			    LookupSubtableContents(gfi,isgpos);
			else {
			    if ( !(event->u.mouse.state&(ksm_shift|ksm_control)) ) {
				/* If line is selected, and we're going to pop up a menu */
				/*  then don't clear other selected lines */
				if ( !lk->all[i].subtables[j].selected ||
					(event->u.mouse.button!=3 && !LookupDragDropable(gfi,lk)))
				    LookupDeselect(lk);
				else if ( event->u.mouse.button!=3 ) {
				    gfi->lk_drag_and_drop = gfi->lk_dropablecursor = true;
				    GDrawSetCursor(gw,ct_features);
				}
				lk->all[i].subtables[j].selected = true;
			    } else
				lk->all[i].subtables[j].selected = !lk->all[i].subtables[j].selected;
			    GFI_LookupEnableButtons(gfi,isgpos);
			    GDrawRequestExpose(gw,NULL,false);
			    if ( event->u.mouse.button==3 )
				LookupMenu(gfi,lk,isgpos,event);
			}
return;
		    }
		}
		++lcnt;
	    }
	}
    }
}

static int lookups_e_h(GWindow gw, GEvent *event, int isgpos) {
    struct gfi_data *gfi = GDrawGetUserData(gw);

    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
return( GGadgetDispatchEvent(GWidgetGetControl(gw,CID_LookupVSB+isgpos),event));
    }

    switch ( event->type ) {
      case et_char:
return( GFI_Char(gfi,event) );
      case et_expose:
	LookupExpose(gw,gfi,isgpos);
      break;
      case et_mousedown: case et_mousemove: case et_mouseup:
	LookupMouse(gfi,isgpos,event);
      break;
      case et_resize: {
	GRect r;
	GDrawGetSize(gw,&r);
	gfi->lkheight = r.height; gfi->lkwidth = r.width;
	GFI_LookupScrollbars(gfi,false,false);
	GFI_LookupScrollbars(gfi,true,false);
      }
      break;
    }
return( true );
}

static int gposlookups_e_h(GWindow gw, GEvent *event) {
return( lookups_e_h(gw,event,true));
}

static int gsublookups_e_h(GWindow gw, GEvent *event) {
return( lookups_e_h(gw,event,false));
}

void FontInfo(SplineFont *sf,int deflayer,int defaspect,int sync) {
    // This opens the Font Info box to the tab specified by defaspect,
    // which indexes the array aspects, populated below.
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GTabInfo aspects[26], vaspects[6], lkaspects[3];
    GGadgetCreateData mgcd[10], ngcd[19], psgcd[30], tngcd[8],
	pgcd[12], vgcd[21], pangcd[23], comgcd[4], txgcd[23], floggcd[4],
	mfgcd[8], mcgcd[8], szgcd[19], mkgcd[7], metgcd[33], vagcd[3], ssgcd[23],
	xugcd[8], dgcd[6], ugcd[6], gaspgcd[5], gaspgcd_def[2], lksubgcd[2][4],
	lkgcd[2], lkbuttonsgcd[15], cgcd[12], lgcd[20], msgcd[7], ssngcd[8],
	woffgcd[8], privategcd_def[4];
    GGadgetCreateData mb[2], mb2, nb[2], nb2, nb3, xub[2], psb[2], psb2[3], ppbox[4],
	    vbox[4], metbox[2], ssbox[2], panbox[2], combox[2], mkbox[3],
	    txbox[5], ubox[3], dbox[2], flogbox[2],
	    mcbox[3], mfbox[3], szbox[6], tnboxes[4], gaspboxes[3],
	    lkbox[7], cbox[6], lbox[8], msbox[3], ssboxes[4], woffbox[2];
    GGadgetCreateData *marray[7], *marray2[9], *narray[29], *narray2[7], *narray3[3],
	*xuarray[20], *psarray[10], *psarray2[21], *psarray4[10],
	*pparray[6], *vradio[5], *varray[41], *metarray[53],
	*ssarray[58], *panarray[40], *comarray[3], *flogarray[3],
	*mkarray[6], *msarray[6],
	*txarray[5], *txarray2[30],
	*txarray3[6], *txarray4[6], *uarray[5], *darray[10],
	*mcarray[13], *mcarray2[7],
	*mfarray[14], *szarray[7], *szarray2[5], *szarray3[7],
	*szarray4[4], *tnvarray[4], *tnharray[6], *tnharray2[5], *gaspharray[6],
	*gaspvarray[3], *lkarray[2][7], *lkbuttonsarray[17], *lkharray[3],
	*charray1[4], *charray2[4], *charray3[4], *cvarray[9], *cvarray2[4],
	*larray[16], *larray2[25], *larray3[6], *larray4[5], *uharray[4],
	*ssvarray[4], *woffarray[16];
    GTextInfo mlabel[10], nlabel[18], pslabel[30], tnlabel[7],
	plabel[12], vlabel[21], panlabel[22], comlabel[3], txlabel[23],
	mflabel[8], mclabel[8], szlabel[17], mklabel[7], metlabel[32],
	sslabel[23], xulabel[8], dlabel[5], ulabel[3], gasplabel[5],
	lkbuttonslabel[14], clabel[11], floglabel[3], llabel[20], mslabel[7],
	ssnlabel[7], wofflabel[8], privatelabel_def[3];
    GTextInfo *namelistnames;
    struct gfi_data *d;
    char iabuf[20], upbuf[20], uwbuf[20], asbuf[20], dsbuf[20],
	    vbuf[20], uibuf[12], embuf[20];
    char dszbuf[20], dsbbuf[20], dstbuf[21], sibuf[20], swbuf[20], sfntrbuf[20];
    char ranges[40], codepages[40];
    char woffmajorbuf[20], woffminorbuf[20];
    int i,j,k,g, psrow;
    int mcs;
    char title[130];
    FontRequest rq;
    int as, ds, ld;
    char **nlnames;
    char createtime[200], modtime[200];
    unichar_t *tmpcreatetime, *tmpmodtime;
    time_t t;
    const struct tm *tm;
    struct matrixinit mi, gaspmi, layersmi, ssmi, sizemi, marks_mi, markc_mi, private_mi;
    struct matrix_data *marks_md, *markc_md;
    int ltype;
    static GBox small_blue_box;
    extern GBox _GGadget_button_box;
    static GFont *fi_font=NULL;
    char *copied_copyright;

    FontInfoInit();

    if ( sf->fontinfo!=NULL ) {
	GDrawSetVisible(((struct gfi_data *) (sf->fontinfo))->gw,true);
	GDrawRaise( ((struct gfi_data *) (sf->fontinfo))->gw );
return;
    }
    if ( defaspect==-1 )
	defaspect = last_aspect;

    d = calloc(1,sizeof(struct gfi_data));
    sf->fontinfo = d;

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
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268+85));
    pos.height = GDrawPointsToPixels(NULL,375);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,d,&wattrs);

    d->sf = sf;
    d->def_layer = deflayer;
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

    ngcd[8].gd.pos.x = 12; ngcd[8].gd.pos.y = ngcd[6].gd.pos.y+26;
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

    nlabel[10].text = (unichar_t *) _("sfnt _Revision:");
    nlabel[10].text_is_1byte = true;
    nlabel[10].text_in_resource = true;
    ngcd[10].gd.label = &nlabel[10];
    ngcd[10].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    ngcd[10].gd.popup_msg = (unichar_t *) _("If you leave this field blank FontForge will use a default based on\neither the version string above, or one in the 'name' table." );
    ngcd[10].creator = GLabelCreate;

    sfntrbuf[0]='\0';
    if ( sf->sfntRevision!=sfntRevisionUnset )
	sprintf( sfntrbuf, "%g", sf->sfntRevision/65536.0 );
    ngcd[11].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    nlabel[11].text = (unichar_t *) sfntrbuf;
    nlabel[11].text_is_1byte = true;
    ngcd[11].gd.label = &nlabel[11];
    ngcd[11].gd.cid = CID_Revision;
    ngcd[11].gd.popup_msg = (unichar_t *) _("If you leave this field blank FontForge will use a default based on\neither the version string above, or one in the 'name' table." );
    ngcd[11].creator = GTextFieldCreate;

    nlabel[12].text = (unichar_t *) _("_Base Filename:");
    nlabel[12].text_is_1byte = true;
    nlabel[12].text_in_resource = true;
    ngcd[12].gd.label = &nlabel[12];
    ngcd[12].gd.popup_msg = (unichar_t *) _("Use this as the default base for the filename\nwhen generating a font." );
    ngcd[12].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    ngcd[12].creator = GLabelCreate;

/* GT: The space in front of "Same" makes things line up better */
    nlabel[13].text = (unichar_t *) _(" Same as Fontname");
    nlabel[13].text_is_1byte = true;
    ngcd[13].gd.label = &nlabel[13];
    ngcd[13].gd.flags = sf->defbasefilename==NULL ? (gg_visible | gg_enabled | gg_cb_on ) : (gg_visible | gg_enabled);
    ngcd[13].gd.cid = CID_SameAsFontname;
    ngcd[13].creator = GRadioCreate;

    nlabel[14].text = (unichar_t *) "";
    nlabel[14].text_is_1byte = true;
    ngcd[14].gd.label = &nlabel[14];
    ngcd[14].gd.flags = sf->defbasefilename!=NULL ?
		(gg_visible | gg_enabled | gg_cb_on | gg_rad_continueold ) :
		(gg_visible | gg_enabled | gg_rad_continueold);
    ngcd[14].gd.cid = CID_HasDefBase;
    ngcd[14].creator = GRadioCreate;

    ngcd[15].gd.flags = gg_visible | gg_enabled|gg_text_xim;
    nlabel[15].text = (unichar_t *) (sf->defbasefilename?sf->defbasefilename:"");
    nlabel[15].text_is_1byte = true;
    ngcd[15].gd.label = &nlabel[15];
    ngcd[15].gd.cid = CID_DefBaseName;
    ngcd[15].gd.handle_controlevent = GFI_DefBaseChange;
    ngcd[15].creator = GTextFieldCreate;

    ngcd[16].gd.flags = gg_visible | gg_enabled;
    nlabel[16].text = (unichar_t *) _("Copy_right:");
    nlabel[16].text_is_1byte = true;
    nlabel[16].text_in_resource = true;
    ngcd[16].gd.label = &nlabel[16];
    ngcd[16].creator = GLabelCreate;

    ngcd[17].gd.pos.width = ngcd[5].gd.pos.x+ngcd[5].gd.pos.width-26;
    ngcd[17].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap | gg_utf8_popup;
    copied_copyright = NULL;
    if ( sf->copyright!=NULL ) {
	if ( !AllAscii(sf->copyright))
	    nlabel[17].text = (unichar_t *) (copied_copyright = StripToASCII(sf->copyright));
	else
	    nlabel[17].text = (unichar_t *) sf->copyright;
	nlabel[17].text_is_1byte = true;
	ngcd[17].gd.label = &nlabel[17];
    }
    ngcd[17].gd.cid = CID_Notice;
    ngcd[17].gd.popup_msg = (unichar_t *) _("This must be ASCII, so you may not use the copyright symbol (use (c) instead).");
    ngcd[17].creator = GTextAreaCreate;

    memset(&nb,0,sizeof(nb)); memset(&nb2,0,sizeof(nb2)); memset(&nb3,0,sizeof(nb3));

    narray3[0] = &ngcd[14]; narray3[1] = &ngcd[15]; narray3[2] = NULL;

    narray2[0] = &ngcd[12]; narray2[1] = &ngcd[13]; narray2[2] = NULL;
    narray2[3] = GCD_RowSpan; narray2[4] = &nb3; narray2[5] = NULL;
    narray2[6] = NULL;

    narray[0] = &ngcd[0]; narray[1] = &ngcd[1]; narray[2] = NULL;
    narray[3] = &ngcd[2]; narray[4] = &ngcd[3]; narray[5] = NULL;
    narray[6] = &ngcd[4]; narray[7] = &ngcd[5]; narray[8] = NULL;
    narray[9] = &ngcd[6]; narray[10] = &ngcd[7]; narray[11] = NULL;
    narray[12] = &ngcd[8]; narray[13] = &ngcd[9]; narray[14] = NULL;
    narray[15] = &ngcd[10]; narray[16] = &ngcd[11]; narray[17] = NULL;
    narray[18] = &nb2; narray[19] = GCD_ColSpan; narray[20] = NULL;
    narray[21] = &ngcd[16]; narray[22] = GCD_ColSpan; narray[23] = NULL;
    narray[24] = &ngcd[17]; narray[25] = GCD_ColSpan; narray[26] = NULL;
    narray[27] = NULL;

    nb3.gd.flags = gg_enabled|gg_visible;
    nb3.gd.u.boxelements = narray3;
    nb3.creator = GHBoxCreate;

    nb2.gd.flags = gg_enabled|gg_visible;
    nb2.gd.u.boxelements = narray2;
    nb2.creator = GHVBoxCreate;

    nb[0].gd.flags = gg_enabled|gg_visible;
    nb[0].gd.u.boxelements = narray;
    nb[0].creator = GHVBoxCreate;

/******************************************************************************/
    memset(&xulabel,0,sizeof(xulabel));
    memset(&xugcd,0,sizeof(xugcd));

    xulabel[0].text = (unichar_t *) _("(Adobe now considers XUID/UniqueID unnecessary)");
    xulabel[0].text_is_1byte = true;
    xulabel[0].text_in_resource = true;
    xugcd[0].gd.label = &xulabel[0];
    xugcd[0].gd.flags = gg_visible | gg_enabled;
    xugcd[0].creator = GLabelCreate;

    xulabel[1].text = (unichar_t *) _("Use XUID");
    xulabel[1].text_is_1byte = true;
    xulabel[1].text_in_resource = true;
    xugcd[1].gd.label = &xulabel[1];
    xugcd[1].gd.flags = gg_visible | gg_enabled;
    if ( sf->use_xuid ) xugcd[1].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    xugcd[1].gd.cid = CID_UseXUID;
    xugcd[1].gd.handle_controlevent = GFI_UseXUIDChanged;
    xugcd[1].creator = GCheckBoxCreate;

    xugcd[2].gd.pos.x = 12; xugcd[2].gd.pos.y = 10+6;
    xugcd[2].gd.flags = gg_visible | gg_enabled;
    xulabel[2].text = (unichar_t *) _("_XUID:");
    xulabel[2].text_is_1byte = true;
    xulabel[2].text_in_resource = true;
    xugcd[2].gd.label = &xulabel[2];
    xugcd[2].creator = GLabelCreate;

    xugcd[3].gd.flags = gg_visible;
    if ( sf->use_xuid ) xugcd[3].gd.flags = gg_visible | gg_enabled;
    if ( sf->xuid!=NULL ) {
	xulabel[3].text = (unichar_t *) sf->xuid;
	xulabel[3].text_is_1byte = true;
	xugcd[3].gd.label = &xulabel[3];
    }
    xugcd[3].gd.cid = CID_XUID;
    xugcd[3].creator = GTextFieldCreate;

    xulabel[4].text = (unichar_t *) _("Use UniqueID");
    xulabel[4].text_is_1byte = true;
    xulabel[4].text_in_resource = true;
    xugcd[4].gd.label = &xulabel[4];
    xugcd[4].gd.flags = gg_visible | gg_enabled;
    if ( sf->use_uniqueid ) xugcd[4].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    xugcd[4].gd.cid = CID_UseUniqueID;
    xugcd[4].gd.handle_controlevent = GFI_UseUniqueIDChanged;
    xugcd[4].creator = GCheckBoxCreate;

    xulabel[5].text = (unichar_t *) _("_UniqueID:");
    xulabel[5].text_is_1byte = true;
    xulabel[5].text_in_resource = true;
    xugcd[5].gd.label = &xulabel[5];
    xugcd[5].gd.flags = gg_visible | gg_enabled;
    xugcd[5].creator = GLabelCreate;

    xugcd[6].gd.flags = gg_visible;
    if ( sf->use_uniqueid ) xugcd[6].gd.flags = gg_visible | gg_enabled;
    xulabel[6].text = (unichar_t *) "";
    xulabel[6].text_is_1byte = true;
    if ( sf->uniqueid!=0 ) {
	sprintf( uibuf, "%d", sf->uniqueid );
	xulabel[6].text = (unichar_t *) uibuf;
    }
    xugcd[6].gd.label = &xulabel[6];
    xugcd[6].gd.cid = CID_UniqueID;
    xugcd[6].creator = GTextFieldCreate;

    xuarray[0] = &xugcd[0]; xuarray[1] = GCD_ColSpan; xuarray[2] = NULL;
    xuarray[3] = &xugcd[1]; xuarray[4] = GCD_ColSpan; xuarray[5] = NULL;
    xuarray[6] = &xugcd[2]; xuarray[7] = &xugcd[3]; xuarray[8] = NULL;
    xuarray[9] = &xugcd[4]; xuarray[10] = GCD_ColSpan; xuarray[11] = NULL;
    xuarray[12] = &xugcd[5]; xuarray[13] = &xugcd[6]; xuarray[14] = NULL;
    xuarray[15] = GCD_Glue; xuarray[16] = GCD_Glue; xuarray[17] = NULL;
    xuarray[18] = NULL;

    memset(xub,0,sizeof(xub));
    xub[0].gd.flags = gg_enabled|gg_visible;
    xub[0].gd.u.boxelements = xuarray;
    xub[0].creator = GHVBoxCreate;

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
    pslabel[4].text = (unichar_t *) _(" _Em Size:");
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
    sprintf( iabuf, "%g", (double) sf->italicangle );
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
    sprintf( upbuf, "%g", (double) sf->upos );
    pslabel[10].text = (unichar_t *) upbuf;
    pslabel[10].text_is_1byte = true;
    psgcd[10].gd.label = &pslabel[10];
    psgcd[10].gd.cid = CID_UPos;
    psgcd[10].creator = GTextFieldCreate;

    psgcd[12].gd.pos.x = 155; psgcd[12].gd.pos.y = psgcd[11].gd.pos.y;
    psgcd[12].gd.flags = gg_visible | gg_enabled;
    pslabel[12].text = (unichar_t *) S_("Underline|_Height:");
    pslabel[12].text_is_1byte = true;
    pslabel[12].text_in_resource = true;
    psgcd[12].gd.label = &pslabel[12];
    psgcd[12].creator = GLabelCreate;

    psgcd[13].gd.pos.x = 200; psgcd[13].gd.pos.y = psgcd[10].gd.pos.y; psgcd[13].gd.pos.width = 47;
    psgcd[13].gd.flags = gg_visible | gg_enabled;
    sprintf( uwbuf, "%g", (double) sf->uwidth );
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
    k = 15;

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
    namelistnames = calloc(i+1,sizeof(GTextInfo));
    for ( i=0; nlnames[i]!=NULL; ++i) {
	namelistnames[i].text = (unichar_t *) nlnames[i];
	namelistnames[i].text_is_1byte = true;
	if ( strcmp(_(sf->for_new_glyphs->title),nlnames[i])==0 ) {
	    namelistnames[i].selected = true;
	    psgcd[k].gd.label = &namelistnames[i];
	}
    }
    psgcd[k++].gd.u.list = namelistnames;
    free(nlnames);


    if ( sf->subfontcnt!=0 ) {
	for ( i=0; i<=13; ++i )
	    psgcd[i].gd.flags &= ~gg_enabled;
    } else if ( sf->cidmaster!=NULL ) {
	for ( i=14; i<=16; ++i )
	    psgcd[i].gd.flags &= ~gg_enabled;
    }

    psarray[0] = &psb2[0];
    psarray[1] = &psgcd[14];
    j=2;
    psarray[j++] = &psb2[1];
    psrow = j;
    psarray[j++] = GCD_Glue;
    psarray[j++] = NULL;

    psarray2[0] = &psgcd[0]; psarray2[1] = &psgcd[1]; psarray2[2] = &psgcd[2]; psarray2[3] = &psgcd[3]; psarray2[4] = NULL;
    psarray2[5] = &psgcd[4]; psarray2[6] = &psgcd[5]; psarray2[7] = &psgcd[6]; psarray2[8] = GCD_ColSpan; psarray2[9] = NULL;
    psarray2[10] = &psgcd[8]; psarray2[11] = &psgcd[7]; psarray2[12] = &psgcd[9]; psarray2[13] = GCD_ColSpan; psarray2[14] = NULL;
    psarray2[15] = &psgcd[11]; psarray2[16] = &psgcd[10]; psarray2[17] = &psgcd[12]; psarray2[18] = &psgcd[13]; psarray2[19] = NULL;
    psarray2[20] = NULL;

    psarray4[0] = &psgcd[k-4]; psarray4[1] = &psgcd[k-3]; psarray4[2] = NULL;
    psarray4[3] = &psgcd[k-2]; psarray4[4] = &psgcd[k-1]; psarray4[5] = NULL;
    psarray4[6] = NULL;

    memset(psb,0,sizeof(psb));
    psb[0].gd.flags = gg_enabled|gg_visible;
    psb[0].gd.u.boxelements = psarray;
    psb[0].creator = GVBoxCreate;

    memset(psb2,0,sizeof(psb2));
    psb2[0].gd.flags = gg_enabled|gg_visible;
    psb2[0].gd.u.boxelements = psarray2;
    psb2[0].creator = GHVBoxCreate;

    psb2[1].gd.flags = gg_enabled|gg_visible;
    psb2[1].gd.u.boxelements = psarray4;
    psb2[1].creator = GHVBoxCreate;
/******************************************************************************/

    memset(&llabel,0,sizeof(llabel));
    memset(&lgcd,0,sizeof(lgcd));
    memset(&lbox,0,sizeof(lbox));

    ltype = -1;
    for ( j=0; j<sf->layer_cnt; ++j ) {
	if ( ltype==-1 )
	    ltype = sf->layers[j].order2;
	else if ( ltype!=sf->layers[j].order2 )
	    ltype = -2;
    }

    k = j = 0;

    lgcd[k].gd.pos.x = 12; lgcd[k].gd.pos.y = 0;
    llabel[k].text = (unichar_t *) _("Font Type:");
    llabel[k].text_is_1byte = true;
    llabel[k].text_in_resource = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.flags = (gg_visible | gg_enabled);
    lgcd[k++].creator = GLabelCreate;
    larray2[j++] = &lgcd[k-1]; larray2[j++] = GCD_ColSpan; larray2[j++] = GCD_Glue; larray2[j++] = GCD_Glue; larray2[j++] = NULL;

    lgcd[k].gd.pos.x = 12; lgcd[k].gd.pos.y = lgcd[k-1].gd.pos.y+k;
    llabel[k].text = (unichar_t *) _("_Outline Font");
    llabel[k].text_is_1byte = true;
    llabel[k].text_in_resource = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.flags = (!sf->strokedfont && !sf->multilayer)?
	    (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    lgcd[k].gd.handle_controlevent = GFI_Type3Change;
    lgcd[k++].creator = GRadioCreate;
    larray2[j++] = GCD_HPad10; larray2[j++] = &lgcd[k-1]; larray2[j++] = GCD_Glue; larray2[j++] = GCD_Glue; larray2[j++] = NULL;

    lgcd[k].gd.pos.x = 12; lgcd[k].gd.pos.y = lgcd[k-1].gd.pos.y+14;
    llabel[k].text = (unichar_t *) _("_Type3 Multi Layered Font");
    llabel[k].text_is_1byte = true;
    llabel[k].text_in_resource = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.flags = ltype!=0 ? (gg_visible | gg_utf8_popup | gg_rad_continueold) :
	sf->multilayer ? (gg_visible | gg_enabled | gg_utf8_popup | gg_cb_on | gg_rad_continueold) :
	(gg_visible | gg_enabled | gg_utf8_popup | gg_rad_continueold);
    lgcd[k].gd.cid = CID_IsMultiLayer;
    lgcd[k].gd.handle_controlevent = GFI_Type3Change;
    lgcd[k].creator = GRadioCreate;
    lgcd[k++].gd.popup_msg = (unichar_t *) _("Allow editing of multiple colors and shades, fills and strokes.\nMulti layered fonts can only be output as type3 or svg fonts.");
    larray2[j++] = GCD_HPad10; larray2[j++] = &lgcd[k-1]; larray2[j++] = GCD_ColSpan; larray2[j++] = GCD_Glue; larray2[j++] = NULL;

    lgcd[k].gd.pos.x = 12; lgcd[k].gd.pos.y = lgcd[k-1].gd.pos.y+14;
    llabel[k].text = (unichar_t *) _("_Stroked Font");
    llabel[k].text_is_1byte = true;
    llabel[k].text_in_resource = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.flags = sf->strokedfont ? (gg_visible | gg_enabled | gg_utf8_popup | gg_cb_on) : (gg_visible | gg_enabled | gg_utf8_popup);
    lgcd[k].gd.cid = CID_IsStrokedFont;
    lgcd[k].gd.handle_controlevent = GFI_Type3Change;
    lgcd[k].creator = GRadioCreate;
    lgcd[k++].gd.popup_msg = (unichar_t *) _("Glyphs will be composed of stroked lines rather than filled outlines.\nAll glyphs are stroked at the following width");
    larray2[j++] = GCD_HPad10; larray2[j++] = &lgcd[k-1];

    lgcd[k].gd.pos.x = 12; lgcd[k].gd.pos.y = lgcd[k-1].gd.pos.y+20;
    llabel[k].text = (unichar_t *) _("  Stroke _Width:");
    llabel[k].text_is_1byte = true;
    llabel[k].text_in_resource = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.flags = gg_visible | gg_enabled;
    lgcd[k++].creator = GLabelCreate;
    larray2[j++] = &lgcd[k-1];

    sprintf( swbuf,"%g", (double) sf->strokewidth );
    lgcd[k].gd.pos.x = 115; lgcd[k].gd.pos.y = lgcd[k-1].gd.pos.y-6; lgcd[k].gd.pos.width = 137;
    lgcd[k].gd.flags = gg_visible | gg_enabled;
    llabel[k].text = (unichar_t *) swbuf;
    llabel[k].text_is_1byte = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.cid = CID_StrokeWidth;
    lgcd[k++].creator = GTextFieldCreate;
    larray2[j++] = &lgcd[k-1]; larray2[j++] = NULL; larray2[j++] = NULL;

    lbox[2].gd.flags = gg_enabled|gg_visible;
    lbox[2].gd.u.boxelements = larray2;
    lbox[2].creator = GHVBoxCreate;
    larray[0] = &lbox[2]; larray[1] = NULL;

    llabel[k].text = (unichar_t *) _("All layers _cubic");
    llabel[k].text_is_1byte = true;
    llabel[k].text_in_resource = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.flags = ltype==0 || sf->multilayer ?
	(gg_visible | gg_enabled | gg_cb_on | gg_utf8_popup) :
	(gg_visible | gg_enabled | gg_utf8_popup);
    lgcd[k].gd.cid = CID_IsOrder3;
    lgcd[k].gd.handle_controlevent = GFI_OrderChange;
    lgcd[k].creator = GRadioCreate;
    lgcd[k++].gd.popup_msg = (unichar_t *) _(
	"Use cubic (that is postscript) splines to hold the outlines of all\n"
	"layers of this font. Cubic splines are generally easier to edit\n"
	"than quadratic (and you may still generate a truetype font from them).");

    llabel[k].text = (unichar_t *) _("All layers _quadratic");
    llabel[k].text_is_1byte = true;
    llabel[k].text_in_resource = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.flags = sf->multilayer ? (gg_visible | gg_utf8_popup) :
	    ltype==1 ? (gg_visible | gg_enabled | gg_cb_on | gg_utf8_popup) :
		(gg_visible | gg_enabled | gg_utf8_popup);
    lgcd[k].gd.cid = CID_IsOrder2;
    lgcd[k].gd.handle_controlevent = GFI_OrderChange;
    lgcd[k].creator = GRadioCreate;
    lgcd[k++].gd.popup_msg = (unichar_t *) _(
	"Use quadratic (that is truetype) splines to hold the outlines of all\n"
	"layers of this font rather than cubic (postscript) splines.");

    llabel[k].text = (unichar_t *) _("_Mixed");
    llabel[k].text_is_1byte = true;
    llabel[k].text_in_resource = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.flags = sf->multilayer ? (gg_visible | gg_utf8_popup) :
	    ltype<0 ? (gg_visible | gg_enabled | gg_cb_on | gg_utf8_popup) :
		(gg_visible | gg_enabled | gg_utf8_popup);
    lgcd[k].gd.handle_controlevent = GFI_OrderChange;
    lgcd[k].gd.cid = CID_IsMixed;
    lgcd[k].creator = GRadioCreate;
    lgcd[k++].gd.popup_msg = (unichar_t *) _(
	"The order of each layer of the font can be controlled\n"
	"individually. This might be useful if you wished to\n"
	"retain both quadratic and cubic versions of a font.");
    larray3[0] = &lgcd[k-3]; larray3[1] = &lgcd[k-2]; larray3[2] = &lgcd[k-1]; larray3[3] = GCD_Glue; larray3[4] = NULL;

    lbox[3].gd.flags = gg_enabled|gg_visible;
    lbox[3].gd.u.boxelements = larray3;
    lbox[3].creator = GHBoxCreate;
    larray[2] = GCD_HPad10; larray[3] = NULL;
    larray[4] = &lbox[3]; larray[5] = NULL;

    lgcd[k].gd.pos.x = 12; lgcd[k].gd.pos.y = 0;
    llabel[k].text = (unichar_t *) _("Guidelines:");
    llabel[k].text_is_1byte = true;
    llabel[k].text_in_resource = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.flags = (gg_visible | gg_enabled);
    lgcd[k++].creator = GLabelCreate;
    larray4[0] = &lgcd[k-1];

    llabel[k].text = (unichar_t *) _("Quadratic");
    llabel[k].text_is_1byte = true;
    llabel[k].text_in_resource = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.flags = sf->multilayer || ltype>=0 ? (gg_visible | gg_utf8_popup ) :
	    (gg_visible | gg_enabled | gg_utf8_popup);
    if ( sf->grid.order2 )
	lgcd[k].gd.flags |= gg_cb_on;
    lgcd[k].gd.cid = CID_GuideOrder2;
    lgcd[k].creator = GCheckBoxCreate;
    lgcd[k++].gd.popup_msg = (unichar_t *) _(
	"Use quadratic splines for the guidelines layer of the font");
    larray4[1] = &lgcd[k-1]; larray4[2] = GCD_Glue; larray4[3] = NULL;

    lbox[4].gd.flags = gg_enabled|gg_visible;
    lbox[4].gd.u.boxelements = larray4;
    lbox[4].creator = GHBoxCreate;
    larray[6] = &lbox[4]; larray[7] = NULL;

    lgcd[k].gd.pos.x = 12; lgcd[k].gd.pos.y = 0;
    llabel[k].text = (unichar_t *) _("\nLayers:");
    llabel[k].text_is_1byte = true;
    llabel[k].text_in_resource = true;
    lgcd[k].gd.label = &llabel[k];
    lgcd[k].gd.flags = (gg_visible | gg_enabled);
    lgcd[k++].creator = GLabelCreate;
    larray[8] = &lgcd[k-1]; larray[9] = NULL;

    LayersMatrixInit(&layersmi,d);

    lgcd[k].gd.pos.width = 300; lgcd[k].gd.pos.height = 180;
    lgcd[k].gd.flags = sf->multilayer ? gg_visible : (gg_enabled | gg_visible);
    lgcd[k].gd.cid = CID_Backgrounds;
    lgcd[k].gd.u.matrix = &layersmi;
    lgcd[k].data = d;
    lgcd[k++].creator = GMatrixEditCreate;
    larray[10] = &lgcd[k-1]; larray[11] = NULL; larray[12] = NULL;

    lbox[0].gd.flags = gg_enabled|gg_visible;
    lbox[0].gd.u.boxelements = larray;
    lbox[0].creator = GHVGroupCreate;

/******************************************************************************/

    memset(&plabel,0,sizeof(plabel));
    memset(&pgcd,0,sizeof(pgcd));

    PSPrivate_MatrixInit(&private_mi,d);

    i=0;
    pgcd[i].gd.pos.width = 300; pgcd[i].gd.pos.height = 200;
    pgcd[i].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    pgcd[i].gd.cid = CID_Private;
    pgcd[i].gd.u.matrix = &private_mi;
    pgcd[i].gd.popup_msg = (unichar_t *) _(
	"The PostScript 'Private' dictionary gives you control over\n"
	"several font-wide versions of hinting.\n"
	"The 'Private' dictionary only applies to PostScript fonts.");
    pgcd[i].data = d;
    pgcd[i++].creator = GMatrixEditCreate;
    pparray[0] = &pgcd[0]; pparray[1] = NULL;

    memset(ppbox,0,sizeof(ppbox));
    ppbox[0].gd.flags = gg_enabled|gg_visible;
    ppbox[0].gd.u.boxelements = pparray;
    ppbox[0].creator = GVBoxCreate;


    memset(&privatelabel_def,0,sizeof(privatelabel_def));
    memset(&privategcd_def,0,sizeof(privategcd_def));
    privategcd_def[0].gd.flags = gg_visible ;
    privatelabel_def[0].text = (unichar_t *) _("_Guess");
    privatelabel_def[0].text_is_1byte = true;
    privatelabel_def[0].text_in_resource = true;
    privategcd_def[0].gd.label = &privatelabel_def[0];
    privategcd_def[0].gd.handle_controlevent = PI_Guess;
    privategcd_def[0].gd.cid = CID_Guess;
    privategcd_def[0].creator = GButtonCreate;

    privategcd_def[1].gd.flags = gg_visible | gg_utf8_popup ;
    privatelabel_def[1].text = (unichar_t *) _("_Histogram");
    privatelabel_def[1].text_is_1byte = true;
    privatelabel_def[1].text_in_resource = true;
    privategcd_def[1].gd.label = &privatelabel_def[1];
    privategcd_def[1].gd.handle_controlevent = PI_Hist;
    privategcd_def[1].gd.cid = CID_Hist;
    privategcd_def[1].gd.popup_msg = (unichar_t *) _("Histogram Dialog");
    privategcd_def[1].creator = GButtonCreate;

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
    if ( sf->pfminfo.fstype == -1 )
	i = 3;
    else if ( sf->pfminfo.fstype&0x8 )
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

    vgcd[15].gd.pos.x = 10; vgcd[15].gd.pos.y = vgcd[11].gd.pos.y+24+6;
    vlabel[15].text = (unichar_t *) _("_OS/2 Version");
    vlabel[15].text_is_1byte = true;
    vlabel[15].text_in_resource = true;
    vgcd[15].gd.label = &vlabel[15];
    vgcd[15].gd.popup_msg = (unichar_t *) _("The 'OS/2' table has changed slightly over the years.\nGenerally fields have been added, but occasionally their\nmeanings have been redefined." );
    vgcd[15].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    vgcd[15].creator = GLabelCreate;

    vgcd[16].gd.pos.x = 90; vgcd[16].gd.pos.y = vgcd[15].gd.pos.y-4; vgcd[16].gd.pos.width = vgcd[7].gd.pos.width;
    vgcd[16].gd.flags = gg_visible | gg_enabled;
    vgcd[16].gd.cid = CID_OS2Version;
    vgcd[16].gd.u.list = os2versions;
    vgcd[16].creator = GListFieldCreate;
    
    vgcd[17].gd.pos.x = 10; vgcd[17].gd.pos.y = vgcd[13].gd.pos.y+26+6;
    vlabel[17].text = (unichar_t *) _("Style Map:");
    vlabel[17].text_is_1byte = true;
    vlabel[17].text_in_resource = true;
    vgcd[17].gd.label = &vlabel[17];
    vgcd[17].gd.flags = gg_visible | gg_enabled;
    vgcd[17].creator = GLabelCreate;

    vgcd[18].gd.pos.x = 90; vgcd[18].gd.pos.y = vgcd[17].gd.pos.y-6; vgcd[18].gd.pos.width = vgcd[7].gd.pos.width;
    vgcd[18].gd.flags = gg_visible | gg_enabled;
    vgcd[18].gd.cid = CID_StyleMap;
    vgcd[18].gd.u.list = stylemap;
    vgcd[18].creator = GListButtonCreate;
    stylemap[0].selected = stylemap[1].selected =
	    stylemap[2].selected = stylemap[3].selected =
        stylemap[4].selected = stylemap[5].selected = false;
    if ( sf->pfminfo.stylemap == 0x00 )
	i = 1;
    else if ( sf->pfminfo.stylemap == 0x40 )
	i = 2;
    else if ( sf->pfminfo.stylemap == 0x01 )
	i = 3;
    else if ( sf->pfminfo.stylemap == 0x20 )
	i = 4;
    else if ( sf->pfminfo.stylemap == 0x21 )
	i = 5;
    else
	i = 0;
    stylemap[i].selected = true;
    vgcd[18].gd.label = &stylemap[i];

    vgcd[14].gd.pos.x = 10; vgcd[14].gd.pos.y = vgcd[18].gd.pos.y+24+6;
    vlabel[14].text = (unichar_t *) _("Weight, Width, Slope Only");
    vlabel[14].text_is_1byte = true;
    vlabel[14].text_in_resource = true;
    vgcd[14].gd.label = &vlabel[14];
    vgcd[14].gd.cid = CID_WeightWidthSlopeOnly;
    vgcd[14].gd.popup_msg = (unichar_t *) _("MS needs to know whether a font family's members differ\nonly in weight, width and slope (and not in other variables\nlike optical size)." );
    vgcd[14].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    vgcd[14].creator = GCheckBoxCreate;

    vradio[0] = GCD_Glue; vradio[1] = &vgcd[8]; vradio[2] = &vgcd[9]; vradio[3] = GCD_Glue; vradio[4] = NULL;

    varray[0] = &vgcd[15]; varray[1] = &vgcd[16]; varray[2] = NULL;
    varray[3] = &vgcd[0]; varray[4] = &vgcd[1]; varray[5] = NULL;
    varray[6] = &vgcd[2]; varray[7] = &vgcd[3]; varray[8] = NULL;
    varray[9] = &vgcd[4]; varray[10] = &vgcd[5]; varray[11] = NULL;
    varray[12] = &vgcd[6]; varray[13] = &vgcd[7]; varray[14] = NULL;
    varray[15] = &vbox[2]; varray[16] = GCD_ColSpan; varray[17] = NULL;
    varray[18] = &vgcd[10]; varray[19] = &vgcd[11]; varray[20] = NULL;
    varray[21] = &vgcd[12]; varray[22] = &vgcd[13]; varray[23] = NULL;
    varray[24] = &vgcd[17]; varray[25] = &vgcd[18]; varray[26] = NULL;
    varray[27] = &vgcd[14]; varray[28] = GCD_ColSpan; varray[29] = NULL;
    varray[30] = GCD_Glue; varray[31] = GCD_Glue; varray[32] = NULL;
    varray[33] = GCD_Glue; varray[34] = GCD_Glue; varray[35] = NULL;
    varray[36] = varray[40] = NULL;

    memset(vbox,0,sizeof(vbox));
    vbox[0].gd.flags = gg_enabled|gg_visible;
    vbox[0].gd.u.boxelements = varray;
    vbox[0].creator = GHVBoxCreate;

    vbox[2].gd.flags = gg_enabled|gg_visible;
    vbox[2].gd.u.boxelements = vradio;
    vbox[2].creator = GHBoxCreate;

/******************************************************************************/

    memset(&metgcd,0,sizeof(metgcd));
    memset(&metlabel,'\0',sizeof(metlabel));
    memset(&metarray,'\0',sizeof(metarray));

    i = j = 0;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = 9;
    metlabel[i].text = (unichar_t *) _("Win _Ascent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("Anything outside the OS/2 WinAscent &\nWinDescent fields will be clipped by windows.\nThis includes marks, etc. that have been repositioned by GPOS.\n(The descent field is usually positive.)\nIf the \"[] Is Offset\" checkbox is clear then\nany number you enter will be the value used in OS/2.\nIf set then any number you enter will be added to the\nfont's bounds. You should leave this\nfield 0 and check \"[*] Is Offset\" in most cases.\n\nNote: WinDescent is a POSITIVE number for\nthings below the baseline");
    metgcd[i].gd.cid = CID_WinAscentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_WinAscent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
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
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("Win _Descent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.cid = CID_WinDescentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_WinDescent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
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
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 5; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y;
    metlabel[i].text = (unichar_t *) _("Really use Typo metrics");
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_UseTypoMetrics;
    metgcd[i].gd.popup_msg = (unichar_t *) _("The specification already says that the typo metrics should be\nused to determine line spacing. But so many\nprograms fail to follow the spec. that MS decided an additional\nbit was needed to remind them to do so.");
    metarray[j++] = &metgcd[i]; metarray[j++] = GCD_ColSpan; metarray[j++] = GCD_Glue;
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("_Typo Ascent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("The typo ascent&descent fields are>supposed<\nto specify the line spacing on windows.\nIn fact usually the win ascent/descent fields do.\n(The descent field is usually negative.)\nIf the \"[] Is Offset\" checkbox is clear then\nany number you enter will be the value used in OS/2.\nIf set then any number you enter will be added to the\nEm-size. You should leave this\nfield 0 and check \"[*] Is Offset\" in most cases.\n\nNOTE: Typo Descent is a NEGATIVE number for\nthings below the baseline");
    metgcd[i].gd.cid = CID_TypoAscentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_TypoAscent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
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
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("T_ypo Descent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.cid = CID_TypoDescentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_TypoDescent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
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
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("Typo Line _Gap:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("Sets the TypoLinegap field in the OS/2 table, used on MS Windows");
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* Line gap value set later */
    metgcd[i].gd.cid = CID_TypoLineGap;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;
    metarray[j++] = GCD_Glue;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("_HHead Ascent Offset:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("This specifies the line spacing on the mac.\n(The descent field is usually negative.)\nIf the \"[] Is Offset\" checkbox is clear then\nany number you enter will be the value used in hhea.\nIf set then any number you enter will be added to the\nfont's bounds. You should leave this\nfield 0 and check \"[*] Is Offset\" in most cases.\n\nNOTE: hhea Descent is a NEGATIVE value for things\nbelow the baseline");
    metgcd[i].gd.cid = CID_HHeadAscentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_HHeadAscent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
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
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("HHead De_scent Offset:");
    metlabel[i].text_in_resource = true;
    metlabel[i].text_is_1byte = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metgcd[i].gd.cid = CID_HHeadDescentLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_HHeadDescent;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
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
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GCheckBoxCreate;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+4;
    metlabel[i].text = (unichar_t *) _("HHead _Line Gap:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("Sets the linegap field in the hhea table, used on the mac");
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* Line gap value set later */
    metgcd[i].gd.cid = CID_LineGap;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;
    metarray[j++] = GCD_Glue;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y+26+6;
    metlabel[i].text = (unichar_t *) _("VHead _Column Spacing:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled | gg_utf8_popup) : (gg_visible | gg_utf8_popup);
    metgcd[i].gd.popup_msg = (unichar_t *) _("Sets the linegap field in the vhea table.\nThis is the horizontal spacing between rows\nof vertically set text.");
    metgcd[i].gd.cid = CID_VLineGapLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-6;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled | gg_utf8_popup) : (gg_visible | gg_utf8_popup);
	/* V Line gap value set later */
    metgcd[i].gd.cid = CID_VLineGap;
    metgcd[i].gd.popup_msg = metgcd[17].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;
    metarray[j++] = GCD_Glue;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+6;
    metlabel[i].text = (unichar_t *) _("Ca_pital Height:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("This denotes the height of X.");
    metgcd[i].gd.cid = CID_CapHeightLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_CapHeight;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;
    metarray[j++] = GCD_Glue;
    metarray[j++] = NULL;

    metgcd[i].gd.pos.x = 10; metgcd[i].gd.pos.y = metgcd[i-2].gd.pos.y+26+6;
    metlabel[i].text = (unichar_t *) _("_X Height:");
    metlabel[i].text_is_1byte = true;
    metlabel[i].text_in_resource = true;
    metgcd[i].gd.label = &metlabel[i];
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    metgcd[i].gd.popup_msg = (unichar_t *) _("This denotes the height of x.");
    metgcd[i].gd.cid = CID_XHeightLab;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GLabelCreate;

    metgcd[i].gd.pos.x = 125; metgcd[i].gd.pos.y = metgcd[i-1].gd.pos.y-4;
    metgcd[i].gd.pos.width = 50;
    metgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	/* value set later */
    metgcd[i].gd.cid = CID_XHeight;
    metgcd[i].gd.popup_msg = metgcd[i-1].gd.popup_msg;
    metarray[j++] = &metgcd[i];
    metgcd[i++].creator = GTextFieldCreate;
    metarray[j++] = GCD_Glue;
    metarray[j++] = NULL;

    metarray[j++] = GCD_Glue; metarray[j++] = GCD_Glue; metarray[j++] = GCD_Glue;

    metarray[j++] = NULL; metarray[j++] = NULL;

    memset(metbox,0,sizeof(metbox));
    metbox[0].gd.flags = gg_enabled|gg_visible;
    metbox[0].gd.u.boxelements = metarray;
    metbox[0].creator = GHVBoxCreate;
/******************************************************************************/

    memset(&ssgcd,0,sizeof(ssgcd));
    memset(&sslabel,'\0',sizeof(sslabel));

    i = j = 0;

    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = 5;
    sslabel[i].text = (unichar_t *) S_("SubscriptSuperUse|Default");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    ssgcd[i].gd.cid = CID_SubSuperDefault;
    ssgcd[i].gd.handle_controlevent = GFI_SubSuperDefault;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GCheckBoxCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue;
    ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+16;
    sslabel[i].text = (unichar_t *) _("Subscript");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 120; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-4;
/* GT: X is a coordinate, the leading spaces help to align it */
    sslabel[i].text = (unichar_t *) _("  X");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 180; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
/* GT: Y is a coordinate, the leading spaces help to align it */
    sslabel[i].text = (unichar_t *) _("Y");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-3].gd.pos.y+14+4;
    sslabel[i].text = (unichar_t *) _("Size");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubXSize;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubYSize;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-3].gd.pos.y+26;
    sslabel[i].text = (unichar_t *) _("Offset");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubXOffset;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SubYOffset;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;


    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+30;
    sslabel[i].text = (unichar_t *) _("Superscript");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue;
    ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+14+4;
    sslabel[i].text = (unichar_t *) _("Size");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperXSize;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperYSize;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-3].gd.pos.y+26;
    sslabel[i].text = (unichar_t *) _("Offset");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;

    ssgcd[i].gd.pos.x = 100; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperXOffset;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_SuperYOffset;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;


    ssgcd[i].gd.pos.x = 5; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+30;
    sslabel[i].text = (unichar_t *) _("Strikeout");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue;
    ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y+14+4;
    sslabel[i].text = (unichar_t *) _("Size");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;
    ssarray[j++] = GCD_Glue;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_StrikeoutSize;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;

    ssgcd[i].gd.pos.x = 10; ssgcd[i].gd.pos.y = ssgcd[i-2].gd.pos.y+26;
    sslabel[i].text = (unichar_t *) _("Pos");
    sslabel[i].text_is_1byte = true;
    ssgcd[i].gd.label = &sslabel[i];
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GLabelCreate;
    ssarray[j++] = GCD_Glue;

    ssgcd[i].gd.pos.x = 160; ssgcd[i].gd.pos.y = ssgcd[i-1].gd.pos.y-6;
    ssgcd[i].gd.pos.width = 50;
    ssgcd[i].gd.flags = gg_visible | gg_enabled;
	/* set later */
    ssgcd[i].gd.cid = CID_StrikeoutPos;
    ssarray[j++] = &ssgcd[i];
    ssgcd[i++].creator = GTextFieldCreate;
    ssarray[j++] = GCD_Glue; ssarray[j++] = NULL;

    ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue; ssarray[j++] = GCD_Glue;

    ssarray[j++] = NULL; ssarray[j++] = NULL;

    memset(ssbox,0,sizeof(ssbox));
    ssbox[0].gd.flags = gg_enabled|gg_visible;
    ssbox[0].gd.u.boxelements = ssarray;
    ssbox[0].creator = GHVBoxCreate;
/******************************************************************************/
    memset(&panlabel,0,sizeof(panlabel));
    memset(&pangcd,0,sizeof(pangcd));

    if ( small_blue_box.main_foreground==0 ) {
	extern void _GButtonInit(void);
	_GButtonInit();
	small_blue_box = _GGadget_button_box;
	small_blue_box.border_type = bt_box;
	small_blue_box.border_shape = bs_rect;
	small_blue_box.border_width = 1;
	small_blue_box.flags = 0;
	small_blue_box.padding = 0;
	small_blue_box.main_foreground = 0x0000ff;
	small_blue_box.border_darker = small_blue_box.main_foreground;
	small_blue_box.border_darkest = small_blue_box.border_brighter =
		small_blue_box.border_brightest =
		small_blue_box.main_background = GDrawGetDefaultBackground(NULL);
    }

    i = j = 0;

    pangcd[i].gd.pos.x = 5; pangcd[i].gd.pos.y = 5;
    panlabel[i].text = (unichar_t *) S_("PanoseUse|Default");
    panlabel[i].text_is_1byte = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    pangcd[i].gd.cid = CID_PanDefault;
    /*pangcd[i].gd.popup_msg = pangcd[i-1].gd.popup_msg;*/
    pangcd[i].gd.handle_controlevent = GFI_PanoseDefault;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GCheckBoxCreate;

    panlabel[i].text = (unichar_t *) _( "http://panose.com/");
    panlabel[i].text_is_1byte = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.pos.x = 12; ugcd[1].gd.pos.y = 10;
    pangcd[i].gd.flags = gg_visible | gg_enabled | gg_dontcopybox;
    pangcd[i].gd.box = &small_blue_box;
    pangcd[i].gd.handle_controlevent = GFI_ShowPanoseDocs;
    pangcd[i++].creator = GButtonCreate;
    panarray[j++] = &pangcd[i-1]; panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+14+4;
    panlabel[i].text = (unichar_t *) S_("Panose|_Family");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanFamilyLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanFamily;
    pangcd[i].gd.u.list = panfamily;
    panarray[j++] = &pangcd[i];
    pangcd[i].gd.handle_controlevent = GFI_ChangePanoseFamily;
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Serifs");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanSerifsLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanSerifs;
    pangcd[i].gd.u.list = panserifs;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) S_("Panose|_Weight");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanWeightLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanWeight;
    pangcd[i].gd.u.list = panweight;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Proportion");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanPropLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanProp;
    pangcd[i].gd.u.list = panprop;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Contrast");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanContrastLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanContrast;
    pangcd[i].gd.u.list = pancontrast;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("Stroke _Variation");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanStrokeVarLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanStrokeVar;
    pangcd[i].gd.u.list = panstrokevar;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Arm Style");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanArmStyleLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanArmStyle;
    pangcd[i].gd.u.list = panarmstyle;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Letterform");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanLetterformLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanLetterform;
    pangcd[i].gd.u.list = panletterform;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_Midline");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanMidLineLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanMidLine;
    pangcd[i].gd.u.list = panmidline;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    pangcd[i].gd.pos.x = 20; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y+26+5;
    panlabel[i].text = (unichar_t *) _("_X-Height");
    panlabel[i].text_is_1byte = true;
    panlabel[i].text_in_resource = true;
    pangcd[i].gd.label = &panlabel[i];
    pangcd[i].gd.cid = CID_PanXHeightLab;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GLabelCreate;

    pangcd[i].gd.pos.x = 100; pangcd[i].gd.pos.y = pangcd[i-1].gd.pos.y-6; pangcd[i].gd.pos.width = 120;
    pangcd[i].gd.flags = gg_visible | gg_enabled;
    pangcd[i].gd.cid = CID_PanXHeight;
    pangcd[i].gd.u.list = panxheight;
    panarray[j++] = &pangcd[i];
    pangcd[i++].creator = GListButtonCreate;
    panarray[j++] = NULL;

    panarray[j++] = GCD_Glue; panarray[j++] = GCD_Glue;

    panarray[j++] = NULL; panarray[j++] = NULL;

    memset(panbox,0,sizeof(panbox));
    panbox[0].gd.flags = gg_enabled|gg_visible;
    panbox[0].gd.u.boxelements = panarray;
    panbox[0].creator = GHVBoxCreate;
/******************************************************************************/
    memset(cgcd,0,sizeof(cgcd));
    memset(clabel,0,sizeof(clabel));
    memset(cbox,0,sizeof(cbox));

    i = j = 0;
    clabel[i].text = (unichar_t *) _("Unicode Ranges:");
    clabel[i].text_is_1byte = true;
    cgcd[i].gd.label = &clabel[i];
    cgcd[i].gd.flags = gg_visible | gg_enabled;
    cgcd[i++].creator = GLabelCreate;
    charray1[0] = &cgcd[i-1];

    clabel[i].text = (unichar_t *) _("Default");
    clabel[i].text_is_1byte = true;
    cgcd[i].gd.label = &clabel[i];
    cgcd[i].gd.flags = gg_visible | gg_enabled;
    if ( !sf->pfminfo.hasunicoderanges ) {
	cgcd[i].gd.flags = gg_visible | gg_enabled | gg_cb_on;
	OS2FigureUnicodeRanges(sf,sf->pfminfo.unicoderanges);
    }
    cgcd[i].gd.cid = CID_URangesDefault;
    cgcd[i].gd.handle_controlevent = OS2_URangesDefault;
    cgcd[i++].creator = GCheckBoxCreate;
    charray1[1] = &cgcd[i-1]; charray1[2] = GCD_Glue; charray1[3] = NULL;

    cbox[2].gd.flags = gg_enabled|gg_visible;
    cbox[2].gd.u.boxelements = charray1;
    cbox[2].creator = GHBoxCreate;
    cvarray[j++] = &cbox[2];

    sprintf( ranges, "%08x.%08x.%08x.%08x",
	    sf->pfminfo.unicoderanges[3], sf->pfminfo.unicoderanges[2],
	    sf->pfminfo.unicoderanges[1], sf->pfminfo.unicoderanges[0]);
    clabel[i].text = (unichar_t *) ranges;
    clabel[i].text_is_1byte = true;
    cgcd[i].gd.label = &clabel[i];
    cgcd[i].gd.pos.width = 270;
    cgcd[i].gd.flags = /* gg_visible |*/ gg_enabled;
    if ( !sf->pfminfo.hasunicoderanges )
	cgcd[i].gd.flags = /* gg_visible*/ 0;
    cgcd[i].gd.cid = CID_UnicodeRanges;
    cgcd[i].gd.handle_controlevent = OS2_UnicodeChange;
    cgcd[i++].creator = GTextFieldCreate;
    cvarray[j++] = &cgcd[i-1];

    cgcd[i].gd.pos.width = 220; cgcd[i].gd.pos.height = 6*12+10;
    cgcd[i].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
    if ( !sf->pfminfo.hasunicoderanges )
	cgcd[i].gd.flags = gg_visible | gg_list_multiplesel;
    cgcd[i].gd.cid = CID_UnicodeList;
    cgcd[i].gd.u.list = unicoderangenames;
    cgcd[i].gd.handle_controlevent = OS2_UnicodeChange;
    cgcd[i++].creator = GListCreate;
    cvarray[j++] = &cgcd[i-1];

    cgcd[i].gd.flags = gg_visible | gg_enabled ;
    cgcd[i++].creator = GLineCreate;
    cvarray[j++] = &cgcd[i-1];

    clabel[i].text = (unichar_t *) _("MS Code Pages:");
    clabel[i].text_is_1byte = true;
    cgcd[i].gd.label = &clabel[i];
    cgcd[i].gd.flags = gg_visible | gg_enabled;
    cgcd[i++].creator = GLabelCreate;
    charray2[0] = &cgcd[i-1];

    clabel[i].text = (unichar_t *) _("Default");
    clabel[i].text_is_1byte = true;
    cgcd[i].gd.label = &clabel[i];
    cgcd[i].gd.flags = gg_visible | gg_enabled;
    if ( !sf->pfminfo.hascodepages ) {
	cgcd[i].gd.flags = gg_visible | gg_enabled | gg_cb_on;
	OS2FigureCodePages(sf,sf->pfminfo.codepages);
    }
    cgcd[i].gd.cid = CID_CPageDefault;
    cgcd[i].gd.handle_controlevent = OS2_CPageDefault;
    cgcd[i++].creator = GCheckBoxCreate;
    charray2[1] = &cgcd[i-1]; charray2[2] = GCD_Glue; charray2[3] = NULL;

    cbox[3].gd.flags = gg_enabled|gg_visible;
    cbox[3].gd.u.boxelements = charray2;
    cbox[3].creator = GHBoxCreate;
    cvarray[j++] = &cbox[3];

    sprintf( codepages, "%08x.%08x",
	    sf->pfminfo.codepages[1], sf->pfminfo.codepages[0]);
    clabel[i].text = (unichar_t *) codepages;
    clabel[i].text_is_1byte = true;
    cgcd[i].gd.label = &clabel[i];
    cgcd[i].gd.pos.width = 140;
    cgcd[i].gd.flags = /*gg_visible |*/ gg_enabled;
    if ( !sf->pfminfo.hascodepages )
	cgcd[i].gd.flags = /*gg_visible*/ 0;
    cgcd[i].gd.cid = CID_CodePageRanges;
    cgcd[i].gd.handle_controlevent = OS2_CodePageChange;
    cgcd[i++].creator = GTextFieldCreate;
    cvarray2[0] = &cgcd[i-1]; cvarray2[1] = GCD_Glue; cvarray2[2] = NULL;

    cbox[4].gd.flags = gg_enabled|gg_visible;
    cbox[4].gd.u.boxelements = cvarray2;
    cbox[4].creator = GVBoxCreate;
    charray3[0] = &cbox[4];

    cgcd[i].gd.pos.width = 200; cgcd[i].gd.pos.height = 6*12+10;
    cgcd[i].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
    if ( !sf->pfminfo.hascodepages )
	cgcd[i].gd.flags = gg_visible | gg_list_multiplesel;
    cgcd[i].gd.cid = CID_CodePageList;
    cgcd[i].gd.u.list = codepagenames;
    cgcd[i].gd.handle_controlevent = OS2_CodePageChange;
    cgcd[i++].creator = GListCreate;
    charray3[1] = &cgcd[i-1]; charray3[2] = NULL;

    cbox[5].gd.flags = gg_enabled|gg_visible;
    cbox[5].gd.u.boxelements = charray3;
    cbox[5].creator = GHBoxCreate;
    cvarray[j++] = &cbox[5]; cvarray[j++] = GCD_Glue; cvarray[j++] = NULL;

    cbox[0].gd.flags = gg_enabled|gg_visible;
    cbox[0].gd.u.boxelements = cvarray;
    cbox[0].creator = GVBoxCreate;

/******************************************************************************/

    memset(&vagcd,0,sizeof(vagcd));
    memset(&vaspects,'\0',sizeof(vaspects));

    i = 0;
    vaspects[i].text = (unichar_t *) _("Misc.");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = vbox;

    vaspects[i].text = (unichar_t *) _("Metrics");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = metbox;

    vaspects[i].text = (unichar_t *) _("Sub/Super");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = ssbox;

    vaspects[i].text = (unichar_t *) _("Panose");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = panbox;

    vaspects[i].text = (unichar_t *) _("Charsets");
    vaspects[i].text_is_1byte = true;
    vaspects[i++].gcd = cbox;

    vagcd[0].gd.pos.x = 4; vagcd[0].gd.pos.y = 6;
    vagcd[0].gd.pos.width = 252;
    vagcd[0].gd.pos.height = 300;
    vagcd[0].gd.u.tabs = vaspects;
    vagcd[0].gd.flags = gg_visible | gg_enabled | gg_tabset_scroll;
    /*vagcd[0].gd.handle_controlevent = GFI_TTFAspectChange;*/
    vagcd[0].gd.cid = CID_TTFTabs;
    vagcd[0].creator = GTabSetCreate;

/******************************************************************************/
    memset(&gaspboxes,0,sizeof(gaspboxes));
    memset(&gaspgcd,0,sizeof(gaspgcd));
    memset(&gasplabel,0,sizeof(gasplabel));
    memset(&gaspgcd_def,0,sizeof(gaspgcd_def));

    GaspMatrixInit(&gaspmi,d);

    i = j = 0;

    gasplabel[i].text = (unichar_t *) S_("Gasp|_Version");
    gasplabel[i].text_is_1byte = true;
    gasplabel[i].text_in_resource = true;
    gaspgcd[i].gd.label = &gasplabel[i];
    gaspgcd[i].gd.flags = gg_visible | gg_enabled;
    gaspharray[j++] = &gaspgcd[i];
    gaspgcd[i++].creator = GLabelCreate;

    for ( g=0; gaspversions[g].text!=NULL; ++g )
	gaspversions[g].selected = false;
    g = ( sf->gasp_version>=0 && sf->gasp_version<g ) ? sf->gasp_version : 0;

    gaspversions[g].selected = true;
    gaspgcd[i].gd.flags = gg_visible | gg_enabled;
    gaspgcd[i].gd.cid = CID_GaspVersion;
    gaspgcd[i].gd.label = &gaspversions[g];
    gaspgcd[i].gd.u.list = gaspversions;
    gaspharray[j++] = &gaspgcd[i];
    gaspgcd[i].gd.handle_controlevent = GFI_GaspVersion;
    gaspgcd[i++].creator = GListButtonCreate;
    gaspharray[j++] = GCD_HPad10;

    gasplabel[i].text = (unichar_t *) _("Optimized For ClearType");
    gasplabel[i].text_is_1byte = true;
    gasplabel[i].text_in_resource = true;
    gaspgcd[i].gd.label = &gasplabel[i];
    gaspgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gaspgcd[i].gd.cid = CID_HeadClearType;
    if ( sf->head_optimized_for_cleartype )
	gaspgcd[i].gd.flags |= gg_cb_on;
    gaspgcd[i].gd.popup_msg = (unichar_t *) _("Actually a bit in the 'head' table.\nIf unset then certain East Asian fonts will not be hinted");
    gaspharray[j++] = &gaspgcd[i];
    gaspgcd[i++].creator = GCheckBoxCreate;
    gaspharray[j++] = NULL;

    gaspboxes[2].gd.flags = gg_enabled|gg_visible;
    gaspboxes[2].gd.u.boxelements = gaspharray;
    gaspboxes[2].creator = GHBoxCreate;

    gaspvarray[0] = &gaspboxes[2];

    gaspgcd[i].gd.pos.width = 300; gaspgcd[i].gd.pos.height = 200;
    gaspgcd[i].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gaspgcd[i].gd.cid = CID_Gasp;
    gaspgcd[i].gd.u.matrix = &gaspmi;
    gaspgcd[i].gd.popup_msg = (unichar_t *) _(
	"The 'gasp' table gives you control over when grid-fitting and\n"
	"anti-aliased rasterizing are done.\n"
	"The table consists of an (ordered) list of pixel sizes each with\n"
	"a set of flags. Those flags apply to all pixel sizes bigger than\n"
	"the previous table entry but less than or equal to the current.\n"
	"The list must be terminated with a pixel size of 65535.\n"
	"Version 1 of the table contains two additional flags that\n"
	"apply to MS's ClearType rasterizer.\n\n"
	"The 'gasp' table only applies to truetype fonts.");
    gaspgcd[i].data = d;
    gaspgcd[i].creator = GMatrixEditCreate;
    gaspvarray[1] = &gaspgcd[i];
    gaspvarray[2] = NULL;

    gaspboxes[0].gd.flags = gg_enabled|gg_visible;
    gaspboxes[0].gd.u.boxelements = gaspvarray;
    gaspboxes[0].creator = GVBoxCreate;

    gaspgcd_def[0].gd.flags = gg_visible | gg_enabled;
    gasplabel[4].text = (unichar_t *) S_("Gasp|_Default");
    gasplabel[4].text_is_1byte = true;
    gasplabel[4].text_in_resource = true;
    gaspgcd_def[0].gd.label = &gasplabel[4];
    gaspgcd_def[0].gd.handle_controlevent = Gasp_Default;
    gaspgcd_def[0].creator = GButtonCreate;
/******************************************************************************/
    memset(&tnlabel,0,sizeof(tnlabel));
    memset(&tngcd,0,sizeof(tngcd));
    memset(&tnboxes,0,sizeof(tnboxes));

    TNMatrixInit(&mi,d);

    tngcd[0].gd.pos.x = 5; tngcd[0].gd.pos.y = 6;
    tngcd[0].gd.flags = gg_visible | gg_enabled;
    tnlabel[0].text = (unichar_t *) _("Sort By:");
    tnlabel[0].text_is_1byte = true;
    tngcd[0].gd.label = &tnlabel[0];
    tngcd[0].creator = GLabelCreate;
    tnharray[0] = &tngcd[0];

    tngcd[1].gd.pos.x = 50; tngcd[1].gd.pos.y = tngcd[0].gd.pos.y-4;
    tngcd[1].gd.flags = gg_enabled | gg_visible;
    tngcd[1].gd.cid = CID_TNLangSort;
    tnlabel[1].text = (unichar_t *) _("_Language");
    tnlabel[1].text_is_1byte = true;
    tnlabel[1].text_in_resource = true;
    tngcd[1].gd.label = &tnlabel[1];
    tngcd[1].creator = GRadioCreate;
    tngcd[1].gd.handle_controlevent = GFI_SortBy;
    tnharray[1] = &tngcd[1];

    tngcd[2].gd.pos.x = 120; tngcd[2].gd.pos.y = tngcd[1].gd.pos.y;
    tngcd[2].gd.flags = gg_visible | gg_enabled | gg_rad_continueold;
    tngcd[2].gd.cid = CID_TNStringSort;
    tnlabel[2].text = (unichar_t *) _("_String Type");
    tnlabel[2].text_is_1byte = true;
    tnlabel[2].text_in_resource = true;
    tngcd[2].gd.label = &tnlabel[2];
    tngcd[2].creator = GRadioCreate;
    tngcd[2].gd.handle_controlevent = GFI_SortBy;
    tnharray[2] = &tngcd[2];

    tngcd[3].gd.pos.x = 195; tngcd[3].gd.pos.y = tngcd[1].gd.pos.y;
    tngcd[3].gd.flags = gg_visible | gg_enabled | gg_cb_on | gg_rad_continueold;
    tnlabel[3].text = (unichar_t *) S_("SortingScheme|Default");
    tnlabel[3].text_is_1byte = true;
    tngcd[3].gd.label = &tnlabel[3];
    tngcd[3].creator = GRadioCreate;
    tngcd[3].gd.handle_controlevent = GFI_SortBy;
    tnharray[3] = &tngcd[3]; tnharray[4] = GCD_Glue; tnharray[5] = NULL;

    tngcd[4].gd.pos.x = 10; tngcd[4].gd.pos.y = tngcd[1].gd.pos.y+14;
    tngcd[4].gd.pos.width = 300; tngcd[4].gd.pos.height = 200;
    tngcd[4].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    tngcd[4].gd.cid = CID_TNames;
    tngcd[4].gd.u.matrix = &mi;
    tngcd[4].gd.popup_msg = (unichar_t *) _(
	"To create a new name, left click on the <New> button, and select a locale.\n"
	"To change the locale, left click on it.\n"
	"To change the string type, left click on it.\n"
	"To change the text, left click in it and then type.\n"
	"To delete a name, right click on the name & select Delete from the menu.\n"
	"To associate or disassociate a truetype name to its postscript equivalent\n"
	"right click and select the appropriate menu item." );
    tngcd[4].data = d;
    tngcd[4].creator = GMatrixEditCreate;

    tngcd[5].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
/* GT: when translating this please leave the "SIL Open Font License" in */
/* GT: English (possibly translating it in parentheses). I believe there */
/* GT: are legal reasons for this. */
/* GT: So "Añadir SIL Open Font License (licencia de fuentes libres)" */
    tnlabel[5].text = (unichar_t *) S_("Add OFL");
    tnlabel[5].image_precedes = false;
    tnlabel[5].image = &OFL_logo;
    tnlabel[5].text_is_1byte = true;
    tnlabel[5].text_in_resource = true;
    tngcd[5].gd.label = &tnlabel[5];
    tngcd[5].gd.handle_controlevent = GFI_AddOFL;
    tngcd[5].gd.popup_msg = (unichar_t *) _(
	"Click here to add the OFL metadata to your own font in the License and License URL fields. \n"
	"Then click on the License field to fill in the placeholders in sync with OFL.txt. \n");
    tngcd[5].creator = GButtonCreate;

    tngcd[6].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    tnlabel[6].text = (unichar_t *) S_("scripts.sil.org/OFL");
    tnlabel[6].text_is_1byte = true;
    tnlabel[6].text_in_resource = true;
    tngcd[6].gd.label = &tnlabel[6];
    tngcd[6].gd.handle_controlevent = GFI_HelpOFL;
    tngcd[6].gd.popup_msg = (unichar_t *) _(
	"Click here for more information about the OFL (SIL Open Font License) \n"
	"including the corresponding FAQ. \n");
    tngcd[6].creator = GButtonCreate;
    tnharray2[0] = &tngcd[5]; tnharray2[1] = &tngcd[6]; tnharray2[2] = GCD_Glue; tnharray2[3] = NULL;
    tnvarray[0] = &tnboxes[2]; tnvarray[1] = &tngcd[4]; tnvarray[2] = &tnboxes[3]; tnvarray[3] = NULL;

    tnboxes[0].gd.flags = gg_enabled|gg_visible;
    tnboxes[0].gd.u.boxelements = tnvarray;
    tnboxes[0].creator = GVBoxCreate;

    tnboxes[2].gd.flags = gg_enabled|gg_visible;
    tnboxes[2].gd.u.boxelements = tnharray;
    tnboxes[2].creator = GHBoxCreate;

    tnboxes[3].gd.flags = gg_enabled|gg_visible;
    tnboxes[3].gd.u.boxelements = tnharray2;
    tnboxes[3].creator = GHBoxCreate;
/******************************************************************************/
    memset(&ssnlabel,0,sizeof(ssnlabel));
    memset(&ssngcd,0,sizeof(ssngcd));
    memset(&ssboxes,0,sizeof(ssboxes));

    SSMatrixInit(&ssmi,d);

    ssngcd[0].gd.flags = gg_visible | gg_enabled;
    ssnlabel[0].text = (unichar_t *) _("The OpenType Style Set features ('ss01'-'ss20') may\n"
				      "be assigned human readable names here.");
    ssnlabel[0].text_is_1byte = true;
    ssngcd[0].gd.label = &ssnlabel[0];
    ssngcd[0].creator = GLabelCreate;
    ssvarray[0] = &ssngcd[0];

    ssngcd[1].gd.pos.width = 300; ssngcd[1].gd.pos.height = 200;
    ssngcd[1].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    ssngcd[1].gd.cid = CID_SSNames;
    ssngcd[1].gd.u.matrix = &ssmi;
    ssngcd[1].gd.popup_msg = (unichar_t *) _(
	"To create a new name, left click on the <New> button, and select a locale (language).\n"
	"To change the locale, left click on it.\n"
	"To change the feature, left click on it.\n"
	"To change the text, left click in it and then type.\n"
	);
    ssngcd[1].data = d;
    ssngcd[1].creator = GMatrixEditCreate;
    ssvarray[1] = &ssngcd[1]; ssvarray[2] = NULL;

    ssboxes[0].gd.flags = gg_enabled|gg_visible;
    ssboxes[0].gd.u.boxelements = ssvarray;
    ssboxes[0].creator = GVBoxCreate;
/******************************************************************************/
    memset(&comlabel,0,sizeof(comlabel));
    memset(&comgcd,0,sizeof(comgcd));

    comgcd[0].gd.flags = gg_visible | gg_enabled;
    comlabel[0].text = (unichar_t *) _("The font comment can contain whatever you feel it should");
    comlabel[0].text_is_1byte = true;
    comgcd[0].gd.label = &comlabel[0];
    comgcd[0].creator = GLabelCreate;

    comgcd[1].gd.pos.x = 10; comgcd[1].gd.pos.y = 10;
    comgcd[1].gd.pos.width = ngcd[11].gd.pos.width; comgcd[1].gd.pos.height = 230;
    comgcd[1].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap | gg_text_xim;
    comgcd[1].gd.cid = CID_Comment;
    comlabel[1].text = (unichar_t *) sf->comments;
    comlabel[1].text_is_1byte = true;
    if ( comlabel[1].text==NULL ) comlabel[1].text = (unichar_t *) "";
    comgcd[1].gd.label = &comlabel[1];
    comgcd[1].creator = GTextAreaCreate;

    comarray[0] = &comgcd[0]; comarray[1] = &comgcd[1]; comarray[2] = NULL;
    memset(combox,0,sizeof(combox));
    combox[0].gd.flags = gg_enabled|gg_visible;
    combox[0].gd.u.boxelements = comarray;
    combox[0].creator = GVBoxCreate;

/******************************************************************************/
    memset(&floglabel,0,sizeof(floglabel));
    memset(&floggcd,0,sizeof(floggcd));

    floggcd[0].gd.flags = gg_visible | gg_enabled;
    floglabel[0].text = (unichar_t *) _("The FONTLOG contains some description of the \n font project, a detailed changelog, and a list of contributors");
    floglabel[0].text_is_1byte = true;
    floggcd[0].gd.label = &floglabel[0];
    floggcd[0].creator = GLabelCreate;

    floggcd[1].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap | gg_text_xim;
    floggcd[1].gd.cid = CID_FontLog;
    floglabel[1].text = (unichar_t *) sf->fontlog;
    floglabel[1].text_is_1byte = true;
    if ( floglabel[1].text==NULL ) floglabel[1].text = (unichar_t *) "";
    floggcd[1].gd.label = &floglabel[1];
    floggcd[1].creator = GTextAreaCreate;

    flogarray[0] = &floggcd[0]; flogarray[1] = &floggcd[1]; flogarray[2] = NULL;
    memset(flogbox,0,sizeof(flogbox));
    flogbox[0].gd.flags = gg_enabled|gg_visible;
    flogbox[0].gd.u.boxelements = flogarray;
    flogbox[0].creator = GVBoxCreate;

/******************************************************************************/
    memset(&mklabel,0,sizeof(mklabel));
    memset(&mkgcd,0,sizeof(mkgcd));

    mklabel[0].text = (unichar_t *) _(
	"These are not Anchor Classes. For them see the \"Lookups\" pane.\n"
	"(Mark Classes can control when lookups are active, they do NOT\n"
	" position glyphs.)");
    mklabel[0].text_is_1byte = true;
    mkgcd[0].gd.label = &mklabel[0];
    mkgcd[0].gd.flags = gg_visible | gg_enabled;
    mkgcd[0].creator = GLabelCreate;

    memset(&markc_mi,0,sizeof(markc_mi));
    markc_mi.col_cnt = 2;
    markc_mi.col_init = markc_ci;

    /* Class 0 is unused */
    markc_md = calloc(sf->mark_class_cnt+1,2*sizeof(struct matrix_data));
    for ( i=1; i<sf->mark_class_cnt; ++i ) {
	markc_md[2*(i-1)+0].u.md_str = copy(sf->mark_class_names[i]);
	markc_md[2*(i-1)+1].u.md_str = SFNameList2NameUni(sf,sf->mark_classes[i]);
    }
    markc_mi.matrix_data = markc_md;
    markc_mi.initial_row_cnt = sf->mark_class_cnt>0?sf->mark_class_cnt-1:0;
    markc_mi.finishedit = GFI_Mark_FinishEdit;

    mkgcd[1].gd.pos.x = 10; mkgcd[1].gd.pos.y = 10;
    mkgcd[1].gd.pos.width = ngcd[15].gd.pos.width; mkgcd[1].gd.pos.height = 200;
    mkgcd[1].gd.flags = gg_visible | gg_enabled;
    mkgcd[1].gd.cid = CID_MarkClasses;
    mkgcd[1].gd.u.matrix = &markc_mi;
    mkgcd[1].creator = GMatrixEditCreate;

    mkgcd[2].gd.pos.x = 10; mkgcd[2].gd.pos.y = mkgcd[1].gd.pos.y+mkgcd[1].gd.pos.height+4;
    mkgcd[2].gd.pos.width = -1;
    mkgcd[2].gd.flags = gg_visible | gg_enabled;

    mkarray[0] = &mkgcd[0]; mkarray[1] = &mkgcd[1];
      mkarray[2] = NULL;
    memset(mkbox,0,sizeof(mkbox));
    mkbox[0].gd.flags = gg_enabled|gg_visible;
    mkbox[0].gd.u.boxelements = mkarray;
    mkbox[0].creator = GVBoxCreate;
/******************************************************************************/
    memset(&mslabel,0,sizeof(mslabel));
    memset(&msgcd,0,sizeof(msgcd));

    mslabel[0].text = (unichar_t *) _(
	"These are not Anchor Classes. For them see the \"Lookups\" pane.\n"
	"(Mark Sets, like Mark Classes can control when lookups are active,\n"
	" they do NOT position glyphs.)");
    mslabel[0].text_is_1byte = true;
    msgcd[0].gd.label = &mslabel[0];
    msgcd[0].gd.flags = gg_visible | gg_enabled;
    msgcd[0].creator = GLabelCreate;

    memset(&marks_mi,0,sizeof(marks_mi));
    marks_mi.col_cnt = 2;
    marks_mi.col_init = marks_ci;

    /* Set 0 is used */
    marks_md = calloc(sf->mark_set_cnt+1,2*sizeof(struct matrix_data));
    for ( i=0; i<sf->mark_set_cnt; ++i ) {
	marks_md[2*i+0].u.md_str = copy(sf->mark_set_names[i]);
	marks_md[2*i+1].u.md_str = SFNameList2NameUni(sf,sf->mark_sets[i]);
    }
    marks_mi.matrix_data = marks_md;
    marks_mi.initial_row_cnt = sf->mark_set_cnt;
    marks_mi.finishedit = GFI_Mark_FinishEdit;

    msgcd[1].gd.pos.x = 10; msgcd[1].gd.pos.y = 10;
    msgcd[1].gd.pos.width = ngcd[15].gd.pos.width; msgcd[1].gd.pos.height = 200;
    msgcd[1].gd.flags = gg_visible | gg_enabled;
    msgcd[1].gd.cid = CID_MarkSets;
    msgcd[1].gd.u.matrix = &marks_mi;
    msgcd[1].creator = GMatrixEditCreate;

    msarray[0] = &msgcd[0]; msarray[1] = &msgcd[1];
      msarray[2] = NULL;
    memset(msbox,0,sizeof(msbox));
    msbox[0].gd.flags = gg_enabled|gg_visible;
    msbox[0].gd.u.boxelements = msarray;
    msbox[0].creator = GVBoxCreate;
/******************************************************************************/
    memset(&wofflabel,0,sizeof(wofflabel));
    memset(&woffgcd,0,sizeof(woffgcd));

    wofflabel[0].text = (unichar_t *) _("Version, Major:");
    wofflabel[0].text_is_1byte = true;
    woffgcd[0].gd.label = &wofflabel[0];
    woffgcd[0].gd.flags = gg_visible | gg_enabled;
    woffgcd[0].creator = GLabelCreate;

    woffmajorbuf[0]='\0';
    woffminorbuf[0]='\0';
    if ( sf->woffMajor!=woffUnset ) {
	sprintf( woffmajorbuf, "%d", sf->woffMajor );
	sprintf( woffminorbuf, "%d", sf->woffMinor );
    }
    woffgcd[1].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    wofflabel[1].text = (unichar_t *) woffmajorbuf;
    wofflabel[1].text_is_1byte = true;
    woffgcd[1].gd.label = &wofflabel[1];
    woffgcd[1].gd.cid = CID_WoffMajor;
    woffgcd[1].gd.popup_msg = (unichar_t *) _("If you leave this field blank FontForge will use a default based on\neither the version string, or one in the 'name' table." );
    woffgcd[1].creator = GTextFieldCreate;

    wofflabel[2].text = (unichar_t *) _("Minor:");
    wofflabel[2].text_is_1byte = true;
    woffgcd[2].gd.label = &wofflabel[2];
    woffgcd[2].gd.flags = gg_visible | gg_enabled;
    woffgcd[2].creator = GLabelCreate;

    woffgcd[3].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    wofflabel[3].text = (unichar_t *) woffminorbuf;
    wofflabel[3].text_is_1byte = true;
    woffgcd[3].gd.label = &wofflabel[3];
    woffgcd[3].gd.cid = CID_WoffMinor;
    woffgcd[3].gd.popup_msg = (unichar_t *) _("If you leave this field blank FontForge will use a default based on\neither the version string, or one in the 'name' table." );
    woffgcd[3].creator = GTextFieldCreate;

    woffarray[0] = &woffgcd[0];
    woffarray[1] = &woffgcd[1];
    woffarray[2] = &woffgcd[2];
    woffarray[3] = &woffgcd[3];
    woffarray[4] = NULL;

    wofflabel[4].text = (unichar_t *) _("Metadata (xml):");
    wofflabel[4].text_is_1byte = true;
    woffgcd[4].gd.label = &wofflabel[4];
    woffgcd[4].gd.flags = gg_visible | gg_enabled;
    woffgcd[4].creator = GLabelCreate;

    woffarray[5] = &woffgcd[4];
    woffarray[6] = GCD_ColSpan;
    woffarray[7] = GCD_ColSpan;
    woffarray[8] = GCD_ColSpan;
    woffarray[9] = NULL;

    woffgcd[5].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    if ( sf->woffMetadata!=NULL ) {
	wofflabel[5].text = (unichar_t *) sf->woffMetadata;
	wofflabel[5].text_is_1byte = true;
	woffgcd[5].gd.label = &wofflabel[5];
    }
    woffgcd[5].gd.cid = CID_WoffMetadata;
    woffgcd[5].creator = GTextAreaCreate;

    woffarray[10] = &woffgcd[5];
    woffarray[11] = GCD_ColSpan;
    woffarray[12] = GCD_ColSpan;
    woffarray[13] = GCD_ColSpan;
    woffarray[14] = NULL;
    woffarray[15] = NULL;

    memset(woffbox,0,sizeof(woffbox));
    woffbox[0].gd.flags = gg_enabled|gg_visible;
    woffbox[0].gd.u.boxelements = woffarray;
    woffbox[0].creator = GHVBoxCreate;
/******************************************************************************/
    memset(&txlabel,0,sizeof(txlabel));
    memset(&txgcd,0,sizeof(txgcd));

    k=0;
    txlabel[k].text = (unichar_t *) U_("ΤεΧ General");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = 10;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.cid = CID_TeXText;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    txlabel[k].text = (unichar_t *) U_("ΤεΧ Math Symbol");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 80; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y;
    txgcd[k].gd.flags = gg_visible | gg_enabled | gg_rad_continueold;
    txgcd[k].gd.cid = CID_TeXMathSym;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    txlabel[k].text = (unichar_t *) U_("ΤεΧ Math Extension");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 155; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y;
    txgcd[k].gd.flags = gg_visible | gg_enabled | gg_rad_continueold;
    txgcd[k].gd.cid = CID_TeXMathExt;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    for ( i=j=0; texparams[i]!=0; ++i ) {
	txlabel[k].text = (unichar_t *) texparams[i];
	txlabel[k].text_is_1byte = true;
	txgcd[k].gd.label = &txlabel[k];
	txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = txgcd[k-2].gd.pos.y+26;
	txgcd[k].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
	txgcd[k].gd.popup_msg = (unichar_t *) texpopups[i];
	txarray2[j++] = &txgcd[k];
	txgcd[k++].creator = GLabelCreate;

	txgcd[k].gd.pos.x = 70; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y-4;
	txgcd[k].gd.flags = gg_visible | gg_enabled;
	txgcd[k].gd.cid = CID_TeX + i;
	txarray2[j++] = &txgcd[k];
	txgcd[k++].creator = GTextFieldCreate;
	txarray2[j++] = GCD_Glue;
	txarray2[j++] = NULL;
    }
    txgcd[k-2].gd.cid = CID_TeXExtraSpLabel;
    txarray2[j++] = NULL;

/* GT: More Parameters */
    txlabel[k].text = (unichar_t *) _("More Params");
    txlabel[k].text_is_1byte = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 20; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y+26;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.handle_controlevent = GFI_MoreParams;
    txgcd[k].gd.cid = CID_MoreParams;
    txgcd[k++].creator = GButtonCreate;

    txarray[0] = &txbox[2]; txarray[1] = &txbox[3]; txarray[2] = &txbox[4]; txarray[3] = GCD_Glue; txarray[4] = NULL;
    txarray3[0] = &txgcd[0]; txarray3[1] = GCD_Glue; txarray3[2] = &txgcd[1];
     txarray3[3] = GCD_Glue; txarray3[4] = &txgcd[2]; txarray3[5] = NULL;
    txarray4[0] = GCD_Glue; txarray4[1] = &txgcd[k-1];
     txarray4[2] = txarray4[3] = txarray4[4] = GCD_Glue; txarray4[5] = NULL;
    memset(txbox,0,sizeof(txbox));
    txbox[0].gd.flags = gg_enabled|gg_visible;
    txbox[0].gd.u.boxelements = txarray;
    txbox[0].creator = GVBoxCreate;

    txbox[2].gd.flags = gg_enabled|gg_visible;
    txbox[2].gd.u.boxelements = txarray3;
    txbox[2].creator = GHBoxCreate;

    txbox[3].gd.flags = gg_enabled|gg_visible;
    txbox[3].gd.u.boxelements = txarray2;
    txbox[3].gd.cid = CID_TeXBox;
    txbox[3].creator = GHVBoxCreate;

    txbox[4].gd.flags = gg_enabled|gg_visible;
    txbox[4].gd.u.boxelements = txarray4;
    txbox[4].creator = GHBoxCreate;
/******************************************************************************/
    memset(&szlabel,0,sizeof(szlabel));
    memset(&szgcd,0,sizeof(szgcd));
    SizeMatrixInit(&sizemi,d);

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

    szlabel[k].text = (unichar_t *) S_("Size|Points");
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

    szgcd[k].gd.pos.width = 300; szgcd[k].gd.pos.height = 200;
    szgcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    szgcd[k].gd.cid = CID_StyleName;
    szgcd[k].gd.u.matrix = &sizemi;
    szgcd[k].gd.popup_msg = (unichar_t *) _(
	"To create a new name, left click on the <New> button, and select a locale (language).\n"
	"To change the locale, left click on it.\n"
	"To change the text, left click in it and then type.\n"
	);
    szgcd[k].data = d;
    szgcd[k].creator = GMatrixEditCreate;

    szarray[0] = &szbox[2];
    szarray[1] = &szbox[3];
    szarray[2] = &szbox[4];
    szarray[3] = &szgcd[11];
    szarray[4] = &szgcd[12];
    szarray[5] = NULL;

    szarray2[0] = &szgcd[0]; szarray2[1] = &szgcd[1]; szarray2[2] = &szgcd[2]; szarray2[3] = GCD_Glue; szarray2[4] = NULL;
    szarray3[0] = &szgcd[5]; szarray3[1] = &szgcd[6]; szarray3[2] = &szgcd[7]; szarray3[3] = &szgcd[8]; szarray3[4] = GCD_Glue; szarray3[5] = NULL;
     szarray3[6] = NULL;
    szarray4[0] = &szgcd[9]; szarray4[1] = &szgcd[10]; szarray4[2] = GCD_Glue; szarray4[3] = NULL;

    memset(szbox,0,sizeof(szbox));
    szbox[0].gd.flags = gg_enabled|gg_visible;
    szbox[0].gd.u.boxelements = szarray;
    szbox[0].creator = GVBoxCreate;

    szbox[2].gd.flags = gg_enabled|gg_visible;
    szbox[2].gd.u.boxelements = szarray2;
    szbox[2].creator = GHBoxCreate;

    szbox[3].gd.flags = gg_enabled|gg_visible;
    szbox[3].gd.label = (GTextInfo *) &szgcd[3];
    szbox[3].gd.u.boxelements = szarray3;
    szbox[3].creator = GHVGroupCreate;

    szbox[4].gd.flags = gg_enabled|gg_visible;
    szbox[4].gd.u.boxelements = szarray4;
    szbox[4].creator = GHBoxCreate;

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
    mcgcd[k].gd.flags = (sf->macstyle!=-1) ? (gg_visible | gg_enabled | gg_cb_on | gg_rad_continueold) : (gg_visible | gg_enabled | gg_rad_continueold);
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

    mcarray[0] = &mcgcd[0]; mcarray[1] = GCD_Glue; mcarray[2] = NULL;
    mcarray[3] = &mcbox[2]; mcarray[4] = &mcgcd[3]; mcarray[5] = NULL;
    mcarray[6] = &mcgcd[4]; mcarray[7] = &mcgcd[5]; mcarray[8] = NULL;
    mcarray[9] = GCD_Glue; mcarray[10] = GCD_Glue; mcarray[11] = NULL;
    mcarray[12] = NULL;
    mcarray2[0] = &mcgcd[1]; mcarray2[1] = &mcgcd[2]; mcarray2[2] = NULL;
    mcarray2[3] = GCD_Glue; mcarray2[4] = GCD_Glue; mcarray2[5] = NULL;
    mcarray2[6] = NULL;

    memset(mcbox,0,sizeof(mcbox));
    mcbox[0].gd.flags = gg_enabled|gg_visible;
    mcbox[0].gd.u.boxelements = mcarray;
    mcbox[0].creator = GHVBoxCreate;

    mcbox[2].gd.flags = gg_enabled|gg_visible;
    mcbox[2].gd.u.boxelements = mcarray2;
    mcbox[2].creator = GHVBoxCreate;

/******************************************************************************/
    memset(&lksubgcd,0,sizeof(lksubgcd));
    memset(&lkgcd,0,sizeof(lkgcd));
    memset(&lkaspects,'\0',sizeof(lkaspects));
    memset(&lkbox,'\0',sizeof(lkbox));
    memset(&lkbuttonsgcd,'\0',sizeof(lkbuttonsgcd));
    memset(&lkbuttonslabel,'\0',sizeof(lkbuttonslabel));

    LookupSetup(&d->tables[0],sf->gsub_lookups);
    LookupSetup(&d->tables[1],sf->gpos_lookups);

    i=0;
    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Moves the currently selected lookup to be first in the lookup ordering\nor moves the currently selected subtable to be first in its lookup.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Top");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_LookupTop;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupOrder;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Moves the currently selected lookup before the previous lookup\nor moves the currently selected subtable before the previous subtable.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Up");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_LookupUp;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupOrder;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Moves the currently selected lookup after the next lookup\nor moves the currently selected subtable after the next subtable.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Down");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_LookupDown;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupOrder;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Moves the currently selected lookup to the end of the lookup chain\nor moves the currently selected subtable to be the last subtable in the lookup");
    lkbuttonslabel[i].text = (unichar_t *) _("_Bottom");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_LookupBottom;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupOrder;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Sorts the lookups in a default ordering based on feature tags");
    lkbuttonslabel[i].text = (unichar_t *) _("_Sort");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_LookupSort;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupSort;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_enabled ;
    lkbuttonsgcd[i++].creator = GLineCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_enabled | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Adds a new lookup after the selected lookup\nor at the start of the lookup list if nothing is selected.");
    lkbuttonslabel[i].text = (unichar_t *) _("Add _Lookup");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_AddLookup;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupAddLookup;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Adds a new lookup subtable after the selected subtable\nor at the start of the lookup if nothing is selected.");
    lkbuttonslabel[i].text = (unichar_t *) _("Add Sub_table");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_AddSubtable;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupAddSubtable;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Edits a lookup or lookup subtable.");
    lkbuttonslabel[i].text = (unichar_t *) _("Edit _Metadata");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_EditMetadata;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupEditMetadata;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Edits the transformations in a lookup subtable.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Edit Data");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_EditSubtable;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupEditSubtableContents;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Deletes any selected lookups and their subtables, or deletes any selected subtables.\nThis will also delete any transformations associated with those subtables.");
    lkbuttonslabel[i].text = (unichar_t *) _("De_lete");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_DeleteLookup;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupDeleteLookup;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Merges two selected (and compatible) lookups into one,\nor merges two selected subtables of a lookup into one");
    lkbuttonslabel[i].text = (unichar_t *) _("_Merge");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_MergeLookup;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupMergeLookup;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Reverts the lookup list to its original condition.\nBut any changes to subtable data will remain.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Revert");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_RevertLookups;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupRevertLookup;
    lkbuttonsgcd[i++].creator = GButtonCreate;

    lkbuttonsarray[i] = &lkbuttonsgcd[i];
    lkbuttonsgcd[i].gd.flags = gg_visible | gg_utf8_popup ;
    lkbuttonsgcd[i].gd.popup_msg = (unichar_t *) _("Imports a lookup (and all its subtables) from another font.");
    lkbuttonslabel[i].text = (unichar_t *) _("_Import");
    lkbuttonslabel[i].text_is_1byte = true;
    lkbuttonslabel[i].text_in_resource = true;
    lkbuttonsgcd[i].gd.label = &lkbuttonslabel[i];
    lkbuttonsgcd[i].gd.cid = CID_ImportLookups;
    lkbuttonsgcd[i].gd.handle_controlevent = GFI_LookupImportLookup;
    lkbuttonsgcd[i++].creator = GButtonCreate;
    lkbuttonsarray[i] = GCD_Glue;
    lkbuttonsarray[i+1] = NULL;

    for ( i=0; i<2; ++i ) {
	lkaspects[i].text = (unichar_t *) (i?"GPOS":"GSUB");
	lkaspects[i].text_is_1byte = true;
	lkaspects[i].gcd = &lkbox[2*i];

	lksubgcd[i][0].gd.pos.x = 10; lksubgcd[i][0].gd.pos.y = 10;
	lksubgcd[i][0].gd.pos.width = ngcd[15].gd.pos.width;
	lksubgcd[i][0].gd.pos.height = 150;
	lksubgcd[i][0].gd.flags = gg_visible | gg_enabled;
	lksubgcd[i][0].gd.u.drawable_e_h = i ? gposlookups_e_h : gsublookups_e_h;
	lksubgcd[i][0].gd.cid = CID_LookupWin+i;
	lksubgcd[i][0].creator = GDrawableCreate;

	lksubgcd[i][1].gd.pos.x = 10; lksubgcd[i][1].gd.pos.y = 10;
	lksubgcd[i][1].gd.pos.height = 150;
	lksubgcd[i][1].gd.flags = gg_visible | gg_enabled | gg_sb_vert;
	lksubgcd[i][1].gd.cid = CID_LookupVSB+i;
	lksubgcd[i][1].gd.handle_controlevent = LookupsVScroll;
	lksubgcd[i][1].creator = GScrollBarCreate;

	lksubgcd[i][2].gd.pos.x = 10; lksubgcd[i][2].gd.pos.y = 10;
	lksubgcd[i][2].gd.pos.width = 150;
	lksubgcd[i][2].gd.flags = gg_visible | gg_enabled;
	lksubgcd[i][2].gd.cid = CID_LookupHSB+i;
	lksubgcd[i][2].gd.handle_controlevent = LookupsHScroll;
	lksubgcd[i][2].creator = GScrollBarCreate;

	lksubgcd[i][3].gd.pos.x = 10; lksubgcd[i][3].gd.pos.y = 10;
	lksubgcd[i][3].gd.pos.width = lksubgcd[i][3].gd.pos.height = _GScrollBar_Width;
	lksubgcd[i][3].gd.flags = gg_visible | gg_enabled | gg_tabset_nowindow;
	lksubgcd[i][3].creator = GDrawableCreate;

	lkarray[i][0] = &lksubgcd[i][0]; lkarray[i][1] = &lksubgcd[i][1]; lkarray[i][2] = NULL;
	lkarray[i][3] = &lksubgcd[i][2]; lkarray[i][4] = &lksubgcd[i][3]; lkarray[i][5] = NULL;
	lkarray[i][6] = NULL;

	lkbox[2*i].gd.flags = gg_enabled|gg_visible;
	lkbox[2*i].gd.u.boxelements = lkarray[i];
	lkbox[2*i].creator = GHVBoxCreate;
    }

    lkaspects[0].selected = true;

    lkgcd[0].gd.pos.x = 4; lkgcd[0].gd.pos.y = 10;
    lkgcd[0].gd.pos.width = 250;
    lkgcd[0].gd.pos.height = 260;
    lkgcd[0].gd.u.tabs = lkaspects;
    lkgcd[0].gd.flags = gg_visible | gg_enabled;
    lkgcd[0].gd.cid = CID_Lookups;
    lkgcd[0].gd.handle_controlevent = GFI_LookupAspectChange;
    lkgcd[0].creator = GTabSetCreate;

    lkharray[0] = &lkgcd[0]; lkharray[1] = &lkbox[4]; lkharray[2] = NULL;

    lkbox[4].gd.flags = gg_enabled|gg_visible;
    lkbox[4].gd.u.boxelements = lkbuttonsarray;
    lkbox[4].creator = GVBoxCreate;

    lkbox[5].gd.flags = gg_enabled|gg_visible;
    lkbox[5].gd.u.boxelements = lkharray;
    lkbox[5].creator = GHBoxCreate;


/******************************************************************************/
    memset(&mfgcd,0,sizeof(mfgcd));
    memset(&mflabel,'\0',sizeof(mflabel));
    memset(mfbox,0,sizeof(mfbox));

    GCDFillMacFeat(mfgcd,mflabel,250,sf->features, false, mfbox, mfarray);
/******************************************************************************/

    memset(&dlabel,0,sizeof(dlabel));
    memset(&dgcd,0,sizeof(dgcd));

    dlabel[0].text = (unichar_t *) _("Creation Date:");
    dlabel[0].text_is_1byte = true;
    dlabel[0].text_in_resource = true;
    dgcd[0].gd.label = &dlabel[0];
    dgcd[0].gd.pos.x = 12; dgcd[0].gd.pos.y = 6+6;
    dgcd[0].gd.flags = gg_visible | gg_enabled;
    dgcd[0].creator = GLabelCreate;

    t = sf->creationtime;
    tm = localtime(&t);
    if(!tm) strcpy(createtime, "error");
    else    strftime(createtime,sizeof(createtime),"%c",tm);
    tmpcreatetime = def2u_copy(createtime);
    dgcd[1].gd.pos.x = 115; dgcd[1].gd.pos.y = dgcd[0].gd.pos.y;
    dgcd[1].gd.flags = gg_visible | gg_enabled;
    dlabel[1].text = tmpcreatetime;
    dgcd[1].gd.label = &dlabel[1];
    dgcd[1].creator = GLabelCreate;

    dlabel[2].text = (unichar_t *) _("Modification Date:");
    dlabel[2].text_is_1byte = true;
    dlabel[2].text_in_resource = true;
    dgcd[2].gd.label = &dlabel[2];
    dgcd[2].gd.pos.x = 12; dgcd[2].gd.pos.y = dgcd[0].gd.pos.y+14;
    dgcd[2].gd.flags = gg_visible | gg_enabled;
    dgcd[2].creator = GLabelCreate;

    t = sf->modificationtime;
    tm = localtime(&t);
    if(!tm) strcpy(modtime, "error");
    else    strftime(modtime,sizeof(modtime),"%c",tm);
    tmpmodtime = def2u_copy(modtime);
    dgcd[3].gd.pos.x = 115; dgcd[3].gd.pos.y = dgcd[2].gd.pos.y;
    dgcd[3].gd.flags = gg_visible | gg_enabled;
    dlabel[3].text = tmpmodtime;
    dgcd[3].gd.label = &dlabel[3];
    dgcd[3].creator = GLabelCreate;

    darray[0] = &dgcd[0]; darray[1] = &dgcd[1]; darray[2] = NULL;
    darray[3] = &dgcd[2]; darray[4] = &dgcd[3]; darray[5] = NULL;
    darray[6] = GCD_Glue; darray[7] = GCD_Glue; darray[8] = NULL;
    darray[9] = NULL;

    memset(dbox,0,sizeof(dbox));
    dbox[0].gd.flags = gg_enabled|gg_visible;
    dbox[0].gd.u.boxelements = darray;
    dbox[0].creator = GHVBoxCreate;
/******************************************************************************/

    memset(&ulabel,0,sizeof(ulabel));
    memset(&ugcd,0,sizeof(ugcd));
    memset(ubox,0,sizeof(ubox));

    ulabel[0].text = (unichar_t *) _(
	    "This pane is informative only and shows the characters\n"
	    "actually in the font. If you wish to set the OS/2 Unicode\n"
	    "Range field, change the pane to");
    ulabel[0].text_is_1byte = true;
    ugcd[0].gd.label = &ulabel[0];
    ugcd[0].gd.pos.x = 12; ugcd[0].gd.pos.y = 10;
    ugcd[0].gd.flags = gg_visible | gg_enabled;
    ugcd[0].creator = GLabelCreate;

    ulabel[1].text = (unichar_t *) _( "OS/2 -> Charsets");
    ulabel[1].text_is_1byte = true;
    ugcd[1].gd.label = &ulabel[1];
    ugcd[1].gd.pos.x = 12; ugcd[1].gd.pos.y = 10;
    ugcd[1].gd.flags = gg_visible | gg_enabled | gg_dontcopybox;
    ugcd[1].gd.box = &small_blue_box;
    ugcd[1].gd.handle_controlevent = GFI_URangeAspectChange;
    ugcd[1].creator = GButtonCreate;
    uharray[0] = GCD_Glue; uharray[1] = &ugcd[1]; uharray[2] = GCD_Glue; uharray[3] = NULL;

    ubox[2].gd.flags = gg_enabled|gg_visible;
    ubox[2].gd.u.boxelements = uharray;
    ubox[2].creator = GHBoxCreate;

    ulabel[2].text = (unichar_t *) _("Include Empty Blocks");
    ulabel[2].text_is_1byte = true;
    ulabel[2].text_in_resource = true;
    ugcd[2].gd.label = &ulabel[2];
    ugcd[2].gd.pos.x = 12; ugcd[2].gd.pos.y = 10;
    ugcd[2].gd.flags = gg_visible | gg_enabled;
    ugcd[2].gd.handle_controlevent = GFI_UnicodeEmptiesChange;
    ugcd[2].gd.cid = CID_UnicodeEmpties;
    ugcd[2].creator = GCheckBoxCreate;

    ugcd[3].gd.pos.x = 12; ugcd[3].gd.pos.y = 30;
    ugcd[3].gd.pos.width = ngcd[15].gd.pos.width;
    ugcd[3].gd.pos.height = 200;
    ugcd[3].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    ugcd[3].gd.cid = CID_Unicode;
    ugcd[3].gd.handle_controlevent = GFI_UnicodeRangeChange;
    ugcd[3].gd.popup_msg = (unichar_t *) _("Click on a range to select characters in that range.\nDouble click on a range to see characters that should be\nin the range but aren't.");
    ugcd[3].creator = GListCreate;

    uarray[0] = &ugcd[0]; uarray[1] = &ubox[2]; uarray[2] = &ugcd[2]; uarray[3] = &ugcd[3]; uarray[4] = NULL;

    ubox[0].gd.flags = gg_enabled|gg_visible;
    ubox[0].gd.u.boxelements = uarray;
    ubox[0].creator = GVBoxCreate;

/******************************************************************************/

    // This is where the indices for the different tabs get set as referenced by defaspect.
    // If you change something here, it's probably a good idea to grep for calls to FontInfo
    // in order to confirm/adjust the defaspect arguments.

    memset(&mlabel,0,sizeof(mlabel));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&aspects,'\0',sizeof(aspects));

    i = 0;

    aspects[i].text = (unichar_t *) _("PS Names");
    d->old_aspect = 0;
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = nb;

    aspects[i].text = (unichar_t *) _("General");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = psb;

    aspects[i].text = (unichar_t *) _("Layers");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = lbox;

    aspects[i].text = (unichar_t *) _("PS UID");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = xub;

    d->private_aspect = i;
    aspects[i].text = (unichar_t *) _("PS Private");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = ppbox;

    d->ttfv_aspect = i;
    aspects[i].text = (unichar_t *) _("OS/2");
    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = vagcd;

    d->tn_aspect = i;
    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _("TTF Names");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = tnboxes;

    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _("StyleSet Names");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = ssboxes;

    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _("Grid Fitting");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = gaspboxes;

    d->tx_aspect = i;
/* xgettext won't use non-ASCII messages */
    aspects[i].text = (unichar_t *) U_("ΤεΧ");	/* Tau epsilon Chi, in greek */
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = txbox;

    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _("Size");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = szbox;

    aspects[i].text = (unichar_t *) _("Comment");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = combox;

    aspects[i].text = (unichar_t *) _("FONTLOG");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = flogbox;

    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _("Mark Classes");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = mkbox;

    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _("Mark Sets");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = msbox;

/* GT: OpenType GPOS/GSUB lookups */
    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) S_("OpenType|Lookups");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = &lkbox[5];
/* On my system the "lookups line appears indented. That seems to be because */
/*  the letter "L" has more of a left-side bearing than it should. It is not */
/*  a bug in the nesting level. Remember this next time it bugs me */

    aspects[i].text = (unichar_t *) _("WOFF");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = woffbox;

    aspects[i].text = (unichar_t *) _("Mac");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = mcbox;

    aspects[i].text = (unichar_t *) _("Mac Features");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = mfbox;

    aspects[i].text = (unichar_t *) _("Dates");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = dbox;

    d->unicode_aspect = i;
    aspects[i].text = (unichar_t *) _("Unicode Ranges");
    aspects[i].text_is_1byte = true;
    aspects[i++].gcd = ubox;

    aspects[defaspect].selected = true;

    mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
    mgcd[0].gd.u.tabs = aspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled | gg_tabset_vert;
    // We adjusted the following two values so as to make the GPOS and GSUB scrollbars visible.
    mgcd[0].gd.pos.width = 500; // 260+85;
    mgcd[0].gd.pos.height = 380; // 325;
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

    marray2[0] = marray2[2] = marray2[3] = marray2[4] = marray2[6] = GCD_Glue; marray2[7] = NULL;
    marray2[1] = &mgcd[1]; marray2[5] = &mgcd[2];

    marray[0] = &mgcd[0]; marray[1] = NULL;
    marray[2] = &mb2; marray[3] = NULL;
    marray[4] = GCD_Glue; marray[5] = NULL;
    marray[6] = NULL;

    memset(mb,0,sizeof(mb));
    mb[0].gd.pos.x = mb[0].gd.pos.y = 2;
    mb[0].gd.flags = gg_enabled|gg_visible;
    mb[0].gd.u.boxelements = marray;
    mb[0].gd.cid = CID_TopBox;
    mb[0].creator = GHVGroupCreate;

    memset(&mb2,0,sizeof(mb2));
    mb2.gd.flags = gg_enabled|gg_visible;
    mb2.gd.u.boxelements = marray2;
    mb2.creator = GHBoxCreate;

    GGadgetsCreate(gw,mb);
    GMatrixEditSetNewText(tngcd[4].ret,S_("TrueTypeName|New"));
    GGadgetSelectOneListItem(gaspgcd[0].ret,sf->gasp_version);
    GMatrixEditSetNewText(gaspgcd[3].ret,S_("gaspTableEntry|New"));
    GMatrixEditAddButtons(gaspgcd[3].ret,gaspgcd_def);
    if ( sf->gasp_version==0 ) {
	GMatrixEditShowColumn(gaspgcd[3].ret,3,false);
	GMatrixEditShowColumn(gaspgcd[3].ret,4,false);
    }
    GHVBoxSetExpandableCol(gaspboxes[2].ret,2);
    GHVBoxSetExpandableRow(gaspboxes[0].ret,1);

    GMatrixEditAddButtons(pgcd[0].ret,privategcd_def);
    GMatrixEditSetUpDownVisible(pgcd[0].ret, true);
    GMatrixEditSetOtherButtonEnable(pgcd[0].ret, PSPrivate_EnableButtons);
    GMatrixEditSetNewText(pgcd[0].ret,S_("PSPrivateDictKey|New"));

    GHVBoxSetExpandableCol(lbox[2].ret,3);
    GHVBoxSetExpandableCol(lbox[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(lbox[4].ret,gb_expandglue);
    GHVBoxSetExpandableRow(lbox[0].ret,5);
    GMatrixEditEnableColumn(GWidgetGetControl(gw,CID_Backgrounds),1,ltype<0);
    GMatrixEditShowColumn(GWidgetGetControl(gw,CID_Backgrounds),3,false);
	/* This column contains internal state information which the user */
	/* should not see, ever */

    GHVBoxSetExpandableRow(combox[0].ret,1);
    GHVBoxSetExpandableRow(flogbox[0].ret,1);

    GHVBoxSetExpandableRow(mb[0].ret,0);
    GHVBoxSetExpandableCol(mb2.ret,gb_expandgluesame);

    GHVBoxSetExpandableCol(nb[0].ret,1);
    GHVBoxSetExpandableRow(nb[0].ret,8);
    GHVBoxSetExpandableCol(nb2.ret,1);
    GHVBoxSetExpandableCol(nb3.ret,1);

    GHVBoxSetExpandableCol(xub[0].ret,1);
    GHVBoxSetExpandableRow(xub[0].ret,5);

    GHVBoxSetExpandableRow(psb[0].ret,psrow);
    GHVBoxSetExpandableCol(psb2[0].ret,3);
    GHVBoxSetExpandableCol(psb2[1].ret,1);

    GHVBoxSetExpandableRow(vbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(vbox[0].ret,1);
    GHVBoxSetExpandableCol(vbox[2].ret,gb_expandglue);

    GHVBoxSetExpandableRow(metbox[0].ret,10);
    GHVBoxSetExpandableCol(metbox[0].ret,1);

    GHVBoxSetExpandableRow(ssbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(ssbox[0].ret,gb_expandglue);

    GHVBoxSetExpandableRow(panbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(panbox[0].ret,1);

    GHVBoxSetExpandableRow(cbox[0].ret,2);
    GHVBoxSetExpandableCol(cbox[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(cbox[3].ret,gb_expandglue);
    GHVBoxSetExpandableRow(cbox[4].ret,gb_expandglue);

    GHVBoxSetExpandableRow(woffbox[0].ret,2);

    GHVBoxSetExpandableRow(mkbox[0].ret,1);
    GMatrixEditSetBeforeDelete(mkgcd[1].ret, GFI_Mark_DeleteClass);
    GMatrixEditSetColumnCompletion(mkgcd[1].ret,1,GFI_GlyphListCompletion);

    GHVBoxSetExpandableRow(msbox[0].ret,1);
    GMatrixEditSetBeforeDelete(msgcd[1].ret, GFI_Mark_DeleteClass);
    GMatrixEditSetColumnCompletion(msgcd[1].ret,1,GFI_GlyphListCompletion);

    GHVBoxSetExpandableRow(txbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(txbox[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(txbox[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(txbox[4].ret,gb_expandglue);

    GHVBoxSetExpandableRow(szbox[0].ret,4);
    GHVBoxSetPadding(szbox[2].ret,6,2);
    GHVBoxSetExpandableCol(szbox[2].ret,gb_expandglue);
    GHVBoxSetPadding(szbox[3].ret,6,2);
    GHVBoxSetExpandableCol(szbox[3].ret,gb_expandglue);
    GHVBoxSetPadding(szbox[4].ret,6,2);
    GHVBoxSetExpandableCol(szbox[4].ret,gb_expandglue);

    GHVBoxSetExpandableRow(tnboxes[0].ret,1);
    GHVBoxSetExpandableCol(tnboxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(tnboxes[3].ret,gb_expandglue);

    GHVBoxSetExpandableRow(ssboxes[0].ret,1);

    GHVBoxSetExpandableRow(mcbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(mcbox[0].ret,1);
    GHVBoxSetExpandableRow(mcbox[2].ret,gb_expandglue);

    GHVBoxSetExpandableRow(mfbox[0].ret,0);
    GHVBoxSetExpandableRow(mfbox[2].ret,gb_expandglue);

    GHVBoxSetExpandableRow(dbox[0].ret,gb_expandglue);
    GHVBoxSetExpandableCol(dbox[0].ret,1);

    GHVBoxSetExpandableRow(ubox[0].ret,3);
    GHVBoxSetExpandableCol(ubox[2].ret,gb_expandglue);

    GHVBoxSetExpandableCol(lkbox[0].ret,0);
    GHVBoxSetExpandableRow(lkbox[0].ret,0);
    GHVBoxSetPadding(lkbox[0].ret,0,0);
    GHVBoxSetExpandableCol(lkbox[2].ret,0);
    GHVBoxSetExpandableRow(lkbox[2].ret,0);
    GHVBoxSetPadding(lkbox[2].ret,0,0);
    GHVBoxSetExpandableRow(lkbox[4].ret,gb_expandglue);
    GHVBoxSetExpandableCol(lkbox[5].ret,0);

    GFI_LookupEnableButtons(d,true);
    GFI_LookupEnableButtons(d,false);

    OS2_UnicodeChange(GWidgetGetControl(gw,CID_UnicodeRanges),NULL);
    OS2_CodePageChange(GWidgetGetControl(gw,CID_CodePageRanges),NULL);

    if ( fi_font==NULL ) {
	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = 12;
	rq.weight = 400;
	fi_font = GDrawInstanciateFont(gw,&rq);
	fi_font = GResourceFindFont("FontInfo.Font",fi_font);
    }
    d->font = fi_font;
    GDrawWindowFontMetrics(gw,d->font,&as,&ds,&ld);
    d->as = as; d->fh = as+ds;

    GTextInfoListFree(namelistnames);

    for ( i=0; i<mi.initial_row_cnt; ++i )
	free( mi.matrix_data[3*i+2].u.md_str );
    free( mi.matrix_data );

    free(tmpcreatetime);
    free(tmpmodtime);

    free(copied_copyright);

    GWidgetIndicateFocusGadget(ngcd[1].ret);
    PSPrivate_EnableButtons(GWidgetGetControl(d->gw,CID_Private),-1,-1);
    GFI_AspectChange(mgcd[0].ret,NULL);

    GHVBoxFitWindow(mb[0].ret);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);

    if ( sync ) {
	while ( !d->done )
	    GDrawProcessOneEvent(NULL);
    }
}

void FontMenuFontInfo(void *_fv) {
    if ( _fv && collabclient_inSessionFV( _fv) ) {
	gwwv_post_error(_("Feature Not Available"), _("The Font Information Dialog is not available in collaboration mode yet."));
	return;
    }
    FontInfo( ((FontViewBase *) _fv)->sf,((FontViewBase *) _fv)->active_layer,-1,false);
}

void FontInfoDestroy(SplineFont *sf) {
    if ( sf->fontinfo )
	GFI_CancelClose( (struct gfi_data *) (sf->fontinfo) );
}

void FontInfoInit(void) {
    static int done = false;
    int i, j, k;
    static GTextInfo *needswork[] = {
	macstyles, widthclass, weightclass, fstype, pfmfamily, ibmfamily,
	panfamily, panserifs, panweight, panprop, pancontrast, panstrokevar,
	panarmstyle, panletterform, panmidline, panxheight,
	pantool, panspacing, panasprat, pancontrast2, pantopology, panform,
	panfinials, panxascent,
	panclass, panaspect, panserifvar, pantreatment, panlining,
	pantopology2, pancharrange, pancontrast3,
	pankind, panasprat2,
	mslanguages,
	ttfnameids, interpretations, gridfit, antialias, os2versions,
	codepagenames, unicoderangenames, symsmooth, gfsymsmooth, splineorder,
	NULL
    };
    struct titlelist *tl;
    static char **needswork2[] = { texparams, texpopups,
	mathparams, mathpopups, extparams, extpopups,
    NULL };
    static struct {
	int size;
	struct col_init *ci;
    } needswork3[] = {{ sizeof(ci)/sizeof(ci[0]), ci},
			{ sizeof(gaspci)/sizeof(gaspci[0]), gaspci },
			{ sizeof(layersci)/sizeof(layersci[0]), layersci },
			{ sizeof(ssci)/sizeof(ssci[0]), ssci },
			{ sizeof(sizeci)/sizeof(sizeci[0]), sizeci },
			{ sizeof(marks_ci)/sizeof(marks_ci[0]), marks_ci },
			{ sizeof(markc_ci)/sizeof(markc_ci[0]), markc_ci },
			{ 0, NULL }};

    if ( done )
return;
    done = true;
    for ( j=0; needswork[j]!=NULL; ++j ) {
	for ( i=0; needswork[j][i].text!=NULL; ++i )
	    needswork[j][i].text = (unichar_t *) S_((char *) needswork[j][i].text);
    }
    for ( j=0; needswork2[j]!=NULL; ++j ) {
	for ( i=0; needswork2[j][i]!=NULL; ++i )
	    needswork2[j][i] = S_(needswork2[j][i]);
    }

    for ( j=0; needswork3[j].ci!=NULL; ++j ) {
	for ( i=0; i<needswork3[j].size; ++i ) {
	    needswork3[j].ci[i].title = S_(needswork3[j].ci[i].title);
	    if ( needswork3[j].ci[i].enum_vals!=NULL ) {
		for ( k=0; needswork3[j].ci[i].enum_vals[k].text!=NULL; ++k )
		    needswork3[j].ci[i].enum_vals[k].text = (unichar_t *) S_((char *) needswork3[j].ci[i].enum_vals[k].text);
	    }
	}
    }

    for ( tl=&panoses[0][0]; tl->name!=NULL; ++tl )
	tl->name = S_(tl->name);

    LookupUIInit();
    LookupInit();
}
