/* This file written by George Williams to provide a gateway to fontforge */
/* it it a modification of Raph's bezctx_ps.c */
#include <basics.h>
#include <stdio.h>

#ifndef _NO_LIBSPIRO
#include "bezctx_ff.h"
#include "fontforgevw.h"		/* For LogError, else splinefont.h */
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#else
# include <math.h>
#endif

typedef struct {
    bezctx base;
    int is_open;
    int gotnans;
    SplineSet *ss;
} bezctx_ff;

static void
nancheck(bezctx_ff *bc) {

    if ( !bc->gotnans ) {
	LogError("Spiros did not converge" );
	bc->gotnans = true;
    }
}

static void
bezctx_ff_moveto(bezctx *z, double x, double y, int is_open) {
    bezctx_ff *bc = (bezctx_ff *)z;

    if ( !finite(x) || !finite(y)) {
	nancheck(bc);
	x = y = 0;
    }
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
    SplinePoint *sp;

    if ( !finite(x) || !finite(y)) {
	nancheck(bc);
	x = y = 0;
    }
    sp = SplinePointCreate(x,y);
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
    SplinePoint *sp;

    if ( !finite(xm) || !finite(ym) || !finite(x3) || !finite(y3)) {
	nancheck(bc);
	xm = ym = x3 = y3 = 0;
    }
    sp = SplinePointCreate(x3,y3);
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
    SplinePoint *sp;

    if ( !finite(x1) || !finite(y1) || !finite(x2) || !finite(y2) || !finite(x3) || !finite(y3)) {
	nancheck(bc);
	x1 = y1 = x2 = y2 = x3 = y3 = 0;
    }
    sp = SplinePointCreate(x3,y3);
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
    bezctx_ff *result = chunkalloc(sizeof(bezctx_ff));

    result->base.moveto = bezctx_ff_moveto;
    result->base.lineto = bezctx_ff_lineto;
    result->base.quadto = bezctx_ff_quadto;
    result->base.curveto = bezctx_ff_curveto;
    result->base.mark_knot = NULL;
    result->is_open = 0;
    result->gotnans = 0;
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
    chunkfree(bc,sizeof(bezctx_ff));
return( ss );
}
#endif
