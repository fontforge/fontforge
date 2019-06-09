/* Copyright (C) 2009-2012 by George Williams */
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

#ifndef FONTFORGE_DELTA_H
#define FONTFORGE_DELTA_H

#include "baseviews.h"
#include "splinefont.h"

typedef struct {
    SplineChar *sc;
    int size;
    int nearestpt;		/* Point on spline closest to the questionable grid square */
    int x,y;			/* Of the lower left corner of the grid square */
    double distance;
} QuestionableGrid;

struct qgnode {
    QuestionableGrid *first;
    struct qgnode *kids, *parent;
    int kid_cnt, qg_cnt, tot_under;
    uint8 open;
    char *name;
};

enum qg_error { qg_ok, qg_notnumber, qg_badnumber, qg_badrange, qg_nofont };

enum glyph_sort { gs_unicode, gs_alpha, gs_gid };

enum info_sort { is_glyph_size_pt, is_glyph_pt_size, is_size_glyph_pt };

typedef struct qg_data {
    /* Set by dlg */
    FontViewBase *fv;
    struct charview *cv;
    SplineChar *sc;
    int layer;
    double within;		/* Return center points which are less than within from a spline */
    char *pixelsizes;
    int dpi;
    int depth;

/* Used internally */
    void *freetype_context;
    int cur_size;

/* Set internally */
    QuestionableGrid *qg;
    int cur, max, glyph_start;
    enum qg_error error;

/* Dlg internal */
    struct gwindow *gw;
    int done;

/* Second dlg */
    struct font_instance *font;
    int fh,as;
    int vlcnt;			/* # physical lines in "v" window */
    int lcnt;			/* # logical lines currently open */
    int loff_top;
    struct ggadget *vsb;
    struct gwindow *v;
    enum glyph_sort glyph_sort;
    enum info_sort info_sort;

    struct qgnode list;

    uint8 inprocess;
} QGData;

extern void TopFindQuestionablePoints(struct qg_data *data);

#endif /* FONTFORGE_DELTA_H */
