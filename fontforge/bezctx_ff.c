/* This file written by George Williams to provide a gateway to fontforge */
/* it it a modification of Raph's bezctx_ps.c */
#include <basics.h>
#include <stdio.h>

#ifndef _NO_LIBSPIRO
#include "bezctx_ff.h"
#include "fontforgevw.h"	/* For LogError, else splinefont.h */
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif
#include <math.h>

typedef struct {
    bezctx base;		/* This is a superclass of bezctx, and this is the entry for the base */
    int is_open;
    int gotnans;		/* Sometimes spiro fails to converge and we get NaNs. */
				/* Complain the first time this happens, but not thereafter */
    SplineSet *ss;		/* The fontforge contour which we build up as we go along */
} bezctx_ff;

static void 
nancheck(bezctx_ff *bc) {

    if ( !bc->gotnans ) {	/* Called when we get passed a NaN. Complain the first time that happens */
	LogError(_("Spiros did not converge") );
	bc->gotnans = true;
    }
}

/* This routine starts a new contour */
/* So we allocate a new SplineSet, and then add the first point to it */
static void
bezctx_ff_moveto(bezctx *z, double x, double y, int is_open) {
    bezctx_ff *bc = (bezctx_ff *)z;

    if ( !finite(x) || !finite(y)) {	/* Protection against NaNs */
	nancheck(bc);
	x = y = 0;
    }
    if (!bc->is_open) {
	SplineSet *ss;
	if ( (ss=(SplineSet *)calloc(1,sizeof(SplineSet)))==NULL )
	    return;
	ss->next = bc->ss;
	bc->ss = ss;
    }
    bc->ss->first = bc->ss->last = SplinePointCreate(x,y);
    bc->is_open = is_open;
}

/* This routine creates a linear spline from the previous point specified to this one */
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

/* This could create a quadratic spline, except FontForge only is prepared */
/* to deal with cubics, so convert the quadratic into the equivalent cubic */
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

/* And this creates a cubic */
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

/* Allocates and initializes a new FontForge bezier context */
bezctx *
new_bezctx_ff(void) {
    bezctx_ff *result;

    if ( (result=(bezctx_ff *)calloc(1,sizeof(bezctx_ff)))==NULL )
	return NULL;

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

/* Finishes an old FontForge bezier context, and returns the contour which was created */
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
    free(bc);
    return( ss );
}
#endif
