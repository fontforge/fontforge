/* Copyright (C) 2007-2012 by George Williams */
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
#ifndef FONTFORGE_PRINT_H
#define FONTFORGE_PRINT_H

enum { pt_lp, pt_lpr, pt_ghostview, pt_file, pt_other, pt_pdf, pt_unknown=-1 };
extern int pagewidth, pageheight;
extern char *printlazyprinter;
extern char *printcommand;
#include "baseviews.h"

extern int printtype;
extern int use_gv;

static const int printdpi = 600;

enum printtype { pt_fontdisplay, pt_chars, pt_multisize, pt_fontsample };

struct sfbits {
    /* If it's a CID font we'll only have one. Otherwise we might have */
    /*  several different encodings to get all the glyphs we need. Each */
    /*  one counts as a font */
    SplineFont *sf;
    EncMap *map;
    char psfontname[300];
    int *our_font_objs;
    int next_font, max_font;
    int *fonts;	/* An array of sf->charcnt/256 entries indicating */
    		/* the font number of encodings on that page of   */
    		/* the font. -1 => not mapped (no encodings) */
    FILE *fontfile;
    int cidcnt;
    unsigned int twobyte: 1;
    unsigned int istype42cid: 1;
    unsigned int iscid: 1;
    unsigned int wastwobyte: 1;
    unsigned int isunicode: 1;
    unsigned int isunicodefull: 1;
    struct sfmaps *sfmap;
};

typedef struct printinfo {
    FontViewBase *fv;
    struct metricsview *mv;
    SplineChar *sc;
    SplineFont *mainsf;
    EncMap *mainmap;
    enum printtype pt;
    int pointsize;
    int32 *pointsizes;
    int extrahspace, extravspace;
    FILE *out;
    unsigned int showvm: 1;
    unsigned int overflow: 1;
    unsigned int done: 1;
    unsigned int hadsize: 1;
    int ypos;
    int max;		/* max chars per line */
    int chline;		/* High order bits of characters we're outputting */
    int page;
    int lastbase;
    real xoff, yoff, scale;
    char *printer;
    int copies;
    int pagewidth, pageheight, printtype;
  /* data for pdf files */
    int *object_offsets;
    int *page_objects;
    int next_object, max_object;
    int next_page, max_page;
    /* In most print styles sfcnt==1 and we only print one font, but with */
    /*  sample text there may be many logical fonts. And each one may need to */
    /*  be represented by many actual fonts to encode all our glyphs */
    int sfcnt, sfmax, sfid;
    struct sfbits *sfbits;
    long start_cur_page;
    int lastfont, intext;
    struct layoutinfo *sample;
    int wassfid, wasfn, wasps;
    int lastx, lasty;
} PI, DI;

extern struct printdefaults {
    Encoding *last_cs;
    enum printtype pt;
    int pointsize;
    unichar_t *text;
} pdefs[];
/* defaults for print from fontview, charview, metricsview */

extern void PI_Init(PI *pi,FontViewBase *fv,SplineChar *sc);
extern void DoPrinting(PI *pi,char *filename);
extern int PdfDumpGlyphResources(PI *pi,SplineChar *sc);
extern void makePatName(char *buffer,
	RefChar *ref,SplineChar *sc,int layer,int isstroke,int isgrad);

#endif /* FONTFORGE_PRINT_H */
