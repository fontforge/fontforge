/*
ppedit - A pattern plate editor for Spiro splines.
Copyright (C) 2007 Raph Levien

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

*/
/* This file written by George Williams to provide a gateway to fontforge */
/* it it a modification of bezctx_ff.c */
#include <stdio.h>

#include "zmisc.h"
#include "bezctx.h"
#include "bezctx_ff.h"
#include "splinefont.h"

typedef struct {
    bezctx base;
    int is_open;
    SplineSet *ss;
} bezctx_ff;

static void
bezctx_ff_moveto(bezctx *z, double x, double y, int is_open) {
    bezctx_ff *bc = (bezctx_ff *)z;

    if (!bc->is_open) {
	SplineSet *ss = chunkalloc(sizeof(SplineSet));
	ss->next = bc->ss;
	bc->ss = ss;
    }
    bc->ss->first = bc->ss->last = SplinePointCreate(x,y);
    bc->is_open = is_open;
}

static void
bezctx_ff_lineto(bezctx *z, double x, double y) {
    bezctx_ff *bc = (bezctx_ff *)z;
    SplinePoint *sp = SplinePointCreate(x,y);

    SplineMake3(bc->ss->last,sp);
    bc->ss->last = sp;
}

static void
bezctx_ff_quadto(bezctx *z, double xm, double ym, double x3, double y3)
{
    bezctx_ff *bc = (bezctx_ff *)z;
    double x0, y0;
    double x1, y1;
    double x2, y2;
    SplinePoint *sp = SplinePointCreate(x3,y3);

    x0 = bc->ss->last->me.x;
    y0 = bc->ss->last->me.y;
    x1 = xm + (1./3) * (x0 - xm);
    y1 = ym + (1./3) * (y0 - ym);
    x2 = xm + (1./3) * (x3 - xm);
    y2 = ym + (1./3) * (y3 - ym);
    bc->ss->last->nextcp.x = x1;
    bc->ss->last->nextcp.y = y1;
    bc->ss->last->nonextcp = false;
    sp->prevcp.x = x2;
    sp->prevcp.y = y2;
    sp->noprevcp = false;
    SplineMake3(bc->ss->last,sp);
    bc->ss->last = sp;
}

static void
bezctx_ff_curveto(bezctx *z, double x1, double y1, double x2, double y2,
		  double x3, double y3)
{
    bezctx_ff *bc = (bezctx_ff *)z;
    SplinePoint *sp = SplinePointCreate(x3,y3);

    bc->ss->last->nextcp.x = x1;
    bc->ss->last->nextcp.y = y1;
    bc->ss->last->nonextcp = false;
    sp->prevcp.x = x2;
    sp->prevcp.y = y2;
    sp->noprevcp = false;
    SplineMake3(bc->ss->last,sp);
    bc->ss->last = sp;
}

bezctx *
new_bezctx_ff(void) {
    bezctx_ff *result = znew(bezctx_ff, 1);

    result->base.moveto = bezctx_ff_moveto;
    result->base.lineto = bezctx_ff_lineto;
    result->base.quadto = bezctx_ff_quadto;
    result->base.curveto = bezctx_ff_curveto;
    result->base.mark_knot = NULL;
    result->is_open = 0;
    result->ss = NULL;
    return &result->base;
}

struct splinepointlist *
bezctx_ff_close(bezctx *z)
{
    bezctx_ff *bc = (bezctx_ff *)z;
    SplineSet *ss = bc->ss;

    if (!bc->is_open && ss!=NULL ) {
	if ( ss->first!=ss->last &&
		RealNear(ss->first->me.x,ss->last->me.x) &&
		RealNear(ss->first->me.y,ss->last->me.y)) {
	    ss->first->prevcp = ss->last->prevcp;
	    ss->first->noprevcp = ss->last->noprevcp;
	    ss->first->prev = ss->last->prev;
	    ss->first->prev->to = ss->first;
	    SplinePointFree(ss->last);
	    ss->last = ss->first;
	} else {
	    SplineMake3(ss->last,ss->first);
	    ss->last = ss->first;
	}
    }
    zfree(bc);
return( ss );
}
