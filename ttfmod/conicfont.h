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
#ifndef _CONICFONT_H
#define _CONICFONT_H

#include "ttffont.h"
#include "config.h"

typedef struct ipoint {
    int x;
    int y;
} IPoint;

typedef struct basepoint {
    real x;
    real y;
    int pnum;		/* TrueType points/control points are numbered. -1 => hidden point */
} BasePoint;

typedef struct dbounds {
    real minx, maxx;
    real miny, maxy;
} DBounds;

enum pointtype { pt_curve, pt_corner, pt_tangent };
typedef struct conicpoint {
    BasePoint me;
    BasePoint *nextcp;		/* control point (NULL for lines) */
    BasePoint *prevcp;		/* control point, shared with pt at other end of conic spline */
    unsigned int selected:1;	/* for UI */
    unsigned int pointtype:2;
    unsigned int ishidden:1;	/* a point which is at the average of the two control points around it */
    struct conic *next;
    struct conic *prev;
} ConicPoint;

typedef struct linelist {
    IPoint here;
    struct linelist *next;
    /* The first two fields are constant for the linelist, the next ones */
    /*  refer to a particular screen. If some portion of the line from */
    /*  this point to the next one is on the screen then set cvli_onscreen */
    /*  if this point needs to be clipped then set cvli_clipped */
    /*  asend and asstart are the actual screen locations where this point */
    /*  intersects the clip edge. */
    enum { cvli_onscreen=0x1, cvli_clipped=0x2 } flags;
    IPoint asend, asstart;
} LineList;

typedef struct linearapprox {
    real scale;
    unsigned int oneline: 1;
    unsigned int onepoint: 1;
    unsigned int any: 1;		/* refers to a particular screen */
    struct linelist *lines;
    struct linearapprox *next;
} LinearApprox;

typedef struct conic1d {
    real a, b, c;
} Conic1D;

typedef struct conic {
    unsigned int islinear: 1;		/* No control points */
    unsigned int isticked: 1;
    unsigned int touched: 1;
    ConicPoint *from, *to;
    Conic1D conics[2];		/* conics[0] is the x conic, conics[1] is y */
    struct linearapprox *approx;
} Conic;

typedef struct conicpointlist {
    ConicPoint *first, *last;
    struct conicpointlist *next;
} ConicPointList, ConicSet;

typedef struct refchar {
    unsigned int checked: 1;
    unsigned int selected: 1;
    unsigned int use_my_metrics: 1;
    unsigned int round: 1;
    int glyph;
    real transform[6];		/* transformation matrix (first 2 rows of a 3x3 matrix, missing row is 0,0,1) */
    ConicPointList *conics;
    struct refchar *next;
    DBounds bb;
    struct conicchar *cc;
} RefChar;

struct instrdata {
    uint8 *instrs;
    int instr_cnt;
    uint8 *bts;
};

typedef struct conicchar {
    int glyph;
    int width;
    ConicPointList *conics;
    RefChar *refs;
    int point_cnt;
    struct instrdata instrdata;
    struct charview *views;
    struct conicfont *parent;
    unsigned int loaded: 1;
    unsigned int changed: 1;
    unsigned int ticked: 1;	/* For reference character processing */
    struct coniccharlist { struct conicchar *cc; struct coniccharlist *next;} *dependents;
	    /* The dependents list is a list of all characters which refenence*/
	    /*  the current character directly */
    int32 glyph_offset, glyph_len;
} ConicChar;

typedef struct conicfont {
    int glyph_cnt;
    ConicChar **chars;
#if TT_RASTERIZE_FONTVIEW
    void **raster;
    void *ttf_handle;
#endif
    unsigned int changed: 1;
    struct fontview *fv;
    TtfFont *tfont;
    TtfFile *tfile;
    Table *glyphs, *loca;
    int em;
} ConicFont;

void *chunkalloc(int size);
void chunkfree(void *item,int size);
int RealNear(real a,real b);
int RealNearish(real a,real b);
int RealApprox(real a,real b);
void LineListFree(LineList *ll);
void LinearApproxFree(LinearApprox *la);
void ConicFree(Conic *conic);
void ConicPointFree(ConicPoint *sp);
void ConicPointListFree(ConicPointList *spl);
void ConicPointListsFree(ConicPointList *head);
void RefCharFree(RefChar *ref);
void RefCharsFree(RefChar *ref);
void ConicCharFree(ConicChar *cc);
void ConicRefigure(Conic *conic);
Conic *ConicMake(ConicPoint *from, ConicPoint *to);
ConicPointList *ConicPointListCopy(ConicPointList *base);
ConicPointList *ConicPointListTransform(ConicPointList *base, real transform[6]);
void RefCharRefigure(RefChar *ref);
void RefCharAddDependant(RefChar *ref,ConicChar *referer);
ConicChar *LoadGlyph(ConicFont *cf, int glyph);
ConicFont *LoadConicFont(TtfFont *tfont);
LinearApprox *ConicApproximate(Conic *conic, real scale);
void ConicSetFindBounds(ConicPointList *spl, DBounds *bounds);
void ConicCharFindBounds(ConicChar *cc,DBounds *bounds);
void ConicFontFindBounds(ConicFont *sf,DBounds *bounds);
void ConicPointCatagorize(ConicPoint *sp);
int ConicPointIsACorner(ConicPoint *sp);
void CCCatagorizePoints(ConicChar *cc);

#endif
