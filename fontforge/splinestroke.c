/* Copyright (C) 2000-2012 by George Williams, 2019 by Skef Iterum */
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

#include <fontforge-config.h>

#include "splinestroke.h"

#include "baseviews.h"
#include "cvundoes.h"
#include "fontforge.h"
#include "splinefit.h"
#include "splinefont.h"
#include "splineorder2.h"
#include "splineoverlap.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "utanvec.h"

#include <assert.h>
#ifndef _MSC_VER
#include <complex.h>
#endif
#include <float.h>
#include <math.h>

#define CIRCOFF 0.551915
#define LOG_MAXDIM_ADJ (1.302585)

// Rounding makes even slope-continuous splines only approximately so.
#define INTERSPLINE_MARGIN (1e-1)
#define INTRASPLINE_MARGIN (1e-8)
#define FIXUP_MARGIN (1e-1)
#define CUSPD_MARGIN (1e-5)
// About .25 degrees
#define COS_MARGIN (1.5e-5)
#define MIN_ACCURACY (1e-5)

static inline bigreal NormAngle(bigreal a) {
    if ( a > FF_PI )
	return a-2*FF_PI;
    else if (a <= -FF_PI)
	return a+2*FF_PI;
    else
	return a;
}

static inline int BPNear(BasePoint bp1, BasePoint bp2) {
    return BPWithin(bp1, bp2, INTRASPLINE_MARGIN);
}

enum nibtype { nib_ellip, nib_rect, nib_convex };

// c->nibcorners is a structure that models the "corners" of the current nib
// treated as if all splines were linear, and therefore as a convex polygon.

#define NC_IN_IDX 0
#define NC_OUT_IDX 1

#define N_NEXTI(n, i) (((i)+1)%(n))
#define N_PREVI(n, i) (((n)+(i)-1)%(n))
#define NC_NEXTI(c, i) N_NEXTI((c)->n, i)
#define NC_PREVI(c, i) N_PREVI((c)->n, i)

typedef struct nibcorner {
    SplinePoint *on_nib;
    BasePoint utv[2];    // Unit tangents entering and leaving corner
    unsigned int linear: 1;
} NibCorner;

typedef struct strokecontext {
    enum nibtype nibtype;
    enum linejoin join;
    enum linecap cap;
    enum stroke_rmov rmov;
    bigreal joinlimit;
    bigreal extendcap;
    bigreal acctarget;
    SplineSet *nib;
    int n;
    NibCorner *nibcorners;
    BasePoint pseudo_origin;
    bigreal log_maxdim;
    unsigned int jlrelative: 1;
    unsigned int ecrelative: 1;
    unsigned int remove_inner: 1;
    unsigned int remove_outer: 1;
    unsigned int leave_users_center: 1;	// Don't center the nib when stroking
    unsigned int simplify:1;
    unsigned int extrema:1;
    unsigned int ratio_al:1;
    unsigned int contour_was_ccw:1;
} StrokeContext;

#define NIBOFF_CW_IDX 0
#define NIBOFF_CCW_IDX 1

typedef struct niboffset {
    BasePoint utanvec;        // The reference angle
    int nci[2];
    BasePoint off[2];
    bigreal nt;               // The t value on the nib spline for the angle
    unsigned int at_line: 1;  // Whether the angle is ambiguous
    unsigned int curve: 1;    // Whether the angle is on a curve or line
    unsigned int reversed: 1; // Whether the calculations were reversed for
                              // tracing the right side
} NibOffset;

/******************************************************************************/
/* ****************************** Configuration ***************************** */
/******************************************************************************/

StrokeInfo *InitializeStrokeInfo(StrokeInfo *sip) {
    if ( sip==NULL )
	sip = malloc(sizeof(StrokeInfo));

    memset(sip, 0, sizeof(StrokeInfo));

    sip->width = 50.0;
    sip->join = lj_nib;
    sip->cap = lc_nib;
    sip->stroke_type = si_round;
    sip->rmov = srmov_layer;
    sip->simplify = true;
    sip->extrema = true;
    sip->jlrelative = true;
    sip->ecrelative = true;
    sip->leave_users_center = true;
    sip->joinlimit = 10.0;
    sip->accuracy_target = 0.25;

    return sip;
}

void SITranslatePSArgs(StrokeInfo *sip, enum linejoin lj, enum linecap lc) {
    sip->extendcap = 0;
    switch (lc) {
	case lc_round:
	    sip->cap = lc_nib;
	    break;
	case lc_square:
	    sip->cap = lc_butt;
	    sip->extendcap = 0.5;
	    sip->ecrelative = true;
	    break;
	default:
	    sip->cap = lc;
    }
    switch (lj) {
	case lj_round:
	    sip->join = lj_nib;
	    break;
	default:
	    sip->join = lj;
    }
}

#define CONVEX_SLOTS 1
#define FREEHAND_TOKNUM -10
#define CVSTROKE_TOKNUM -11

static SplineSet *convex_nibs[CONVEX_SLOTS];

int ConvexNibID(const char *tok) {
    if ( tok!=NULL ) {
	if ( strcmp(tok, "default")==0 )
	    return 0;
	else if ( strcmp(tok, "freehand")==0 )
	    return FREEHAND_TOKNUM;
	else if ( strcmp(tok, "ui")==0 )
	    return CVSTROKE_TOKNUM;
	else {
	    return -1;
	}
    } else
	return -1;
}

int StrokeSetConvex(SplineSet *ss, int toknum) {
    StrokeInfo *si = NULL;

    if ( toknum>=0 && toknum<CONVEX_SLOTS ) {
	if ( convex_nibs[toknum]!=NULL )
	    SplinePointListFree(convex_nibs[toknum]);
	convex_nibs[toknum] = ss;
	return true;
    } else if ( no_windowing_ui )
	return false;
    else if ( toknum==CVSTROKE_TOKNUM )
	si = CVStrokeInfo();
    else if ( toknum==FREEHAND_TOKNUM )
	si = CVFreeHandInfo();
    else
	return false;

    if ( si->nib!=NULL )
	SplinePointListFree(si->nib);
    si->nib = ss;

    return true;
}

SplineSet *StrokeGetConvex(int toknum, int cpy) {
    SplineSet *ss = NULL;

    if ( toknum>=0 && toknum<CONVEX_SLOTS )
	ss = convex_nibs[toknum];
    else if ( no_windowing_ui )
	return NULL;
    else if ( toknum==CVSTROKE_TOKNUM )
	ss = CVStrokeInfo()->nib;
    else if ( toknum==FREEHAND_TOKNUM )
	ss = CVFreeHandInfo()->nib;
    else
	return NULL;

    if ( ss==NULL )
	return NULL;

    if ( cpy )
	return SplinePointListCopy(ss);
    else
	return ss;
}

StrokeInfo *CVStrokeInfo() {
    static StrokeInfo *cv_si;
    if ( cv_si==NULL ) {
	cv_si = InitializeStrokeInfo(NULL);
	cv_si->height = cv_si->width;
	cv_si->penangle = FF_PI/4;
    }
    return cv_si;
}

StrokeInfo *CVFreeHandInfo() {
    static StrokeInfo *fv_si = NULL;

    if ( fv_si==NULL ) {
	fv_si = InitializeStrokeInfo(NULL);
	fv_si->cap = lc_butt;
	fv_si->stroke_type = si_centerline;
	fv_si->height = fv_si->width;
	fv_si->penangle = FF_PI/4;
    }
    return fv_si;
}

/******************************************************************************/
/* ********************************* Utility ******************************** */
/******************************************************************************/

static int LineSameSide(BasePoint l1, BasePoint l2, BasePoint p, BasePoint r,
                        int on_line_ok) {
    bigreal tp, tr;

    tp = (l2.x-l1.x)*(p.y-l1.y) - (p.x-l1.x)*(l2.y-l1.y);
    tr = (l2.x-l1.x)*(r.y-l1.y) - (r.x-l1.x)*(l2.y-l1.y);

    if ( RealWithin(tp, 0, 1e-5) )
	return on_line_ok;
    // Rounding means we should be tolerant when on_line_ok is true.
    // Being that tolerant when on_line_ok is false would 
    // yield too many false negatives. 
    if ( on_line_ok && RealWithin(tp, 0, 1) )
	return true;
    return signbit(tr)==signbit(tp);
}

static BasePoint ProjectPointOnLine(BasePoint p, BasePoint lp, BasePoint lut) {

    BasePoint t = BPSub(p, lp);
    return BPAdd(lp, BPScale(lut, BPDot(t, lut)));
}

/* Return values:
 * > 0 -> number of intersections
 *   0 -> No overlap
 *  -1 -> Circle 2 is strictly inside circle 1
 *  -2 -> Circle 1 is strictly inside circle 2
 */
static int CirclesTest(BasePoint c1, bigreal r1, BasePoint c2, bigreal r2) {
    bigreal c_dist = BPDist(c1, c2);

    assert( r1 >= 0 && r2 >= 0 );
    if (fabs(c_dist - r1 - r2) < 1e-3)
	return 1;
    else if (c_dist > r1 + r2)
	return 0;
    else if (c_dist < fabs(r1 - r2)) {
	if (r1 > r2)
	    return -1;
	else
	    return -2;
    } else
	return 2;
}

static int LineCircleTest(BasePoint lp, BasePoint lut, BasePoint c1,
                          bigreal r1) {
    BasePoint t = ProjectPointOnLine(c1, lp, lut);
    bigreal cl_dist = BPDist(c1, t);

    assert( r1 >= 0 );

    if ( fabs(cl_dist-r1) < 1e-3 )
	return 1;
    else if ( cl_dist > r1 )
	return 0;
    else
	return 2;
}

static int PVPCircle(BasePoint p1, BasePoint ut1, BasePoint p2, BasePoint *c,
                     bigreal *r) {
    BasePoint cv1, p3, cv3;

    assert( c!=NULL && r!=NULL );

    cv1 = BP90CCW(ut1);
    p3 = BPAvg(p1, p2);
    cv3 = BP90CCW(BPSub(p2, p1));
    if ( !IntersectLinesSlopes(c, &p1, &cv1, &p3, &cv3) )
	return 0;
    cv3 = BPSub(p1,*c);
    *r = sqrt(BPLenSq(cv3));
    return BPDot(cv3, cv1) < 0 ? -1 : 1;
}

static BasePoint CloserPoint(BasePoint r, BasePoint p1, BasePoint p2) {
    BasePoint d1 = BPSub(p1, r), d2 = BPSub(p2, r);

    return BPLenSq(d1) > BPLenSq(d2) ? p2 : p1;
}

static BasePoint CloserIntersection(BasePoint p1, BasePoint p2, BasePoint i1,
                                    BasePoint i2, BasePoint refp) {
    if ( LineSameSide(p1, p2, i1, refp, false) )
	if ( LineSameSide(p1, p2, i2, refp, false) )
	    return CloserPoint(p1, i1, i2);
	else
	    return i1;
    else
	return i2;
}

static void IntersectLineCircle(BasePoint lp, BasePoint lut, BasePoint c1,
                                bigreal r1, BasePoint *i1, BasePoint *i2) {
    bigreal sqdiff;
    BasePoint pp, t, perp;

    assert( i1!=NULL && i2!=NULL );
    *i1 = *i2 = BPUNINIT;

    pp = ProjectPointOnLine(c1, lp, lut);
    t = BPSub(c1, pp);
    sqdiff = r1*r1 - BPLenSq(t);
    if ( fabs(sqdiff) < 1e-4 ) {
	*i1 = *i2 = pp;
        return;
    }
    perp = BPScale(lut, sqrt(sqdiff));
    *i1 = BPAdd(pp, perp);
    *i2 = BPSub(pp, perp);
}

static void IntersectCircles(BasePoint c1, bigreal r1, BasePoint c2, bigreal r2,
                             BasePoint *i1, BasePoint *i2) {
    BasePoint t1, t2;
    bigreal tmp, c_dist = BPDist(c1, c2), r2s = r2*r2, r1s = r1*r1;
    bigreal cds = c_dist*c_dist;

    assert( i1!=NULL && i2!=NULL );
    *i1 = *i2 = BPUNINIT;

    t1 = BPScale(BPSub(c1, c2), (r2s - r1s)/(2.0*cds));
    t1 = BPAdd(t1, BPAvg(c2, c1));
    tmp = 2.0 * (r2s + r1s)/cds - pow(r2s - r1s, 2.0)/(cds*cds) - 1;
    if ( RealWithin(tmp, 0.0, 1e-8) ) {
	*i1 = t1;
	return;
    }
    t2 = BPScale(((BasePoint) { c1.y-c2.y, c2.x-c1.x }), sqrt(tmp)/2.0);
    *i1 = BPAdd(t1,t2);
    *i2 = BPSub(t1,t2);
}

static void SplineSetLineTo(SplineSet *cur, BasePoint xy) {
    SplinePoint *sp = SplinePointCreate(xy.x, xy.y);
    SplineMake3(cur->last, sp);
    cur->last = sp;
}

/* A hack for cleaning up cusps and intersections on isolated
 * counter-clockwise contours -- builds an enclosing clockwise
 * rectangle, runs remove-overlap, and removes the rectangle.
 */
static SplineSet *SplineContourOuterCCWRemoveOverlap(SplineSet *ss) {
    DBounds b;
    SplineSet *ss_tmp, *ss_last = NULL;
    int trash;

    SplineSetQuickBounds(ss,&b);
    b.minx -= 100;
    b.miny -= 100;
    b.maxx += 100;
    b.maxy += 100;

    ss_tmp = chunkalloc(sizeof(SplineSet));
    ss_tmp->first = ss_tmp->last = SplinePointCreate(b.minx,b.miny);
    SplineSetLineTo(ss_tmp, (BasePoint) { b.minx, b.maxy } );
    SplineSetLineTo(ss_tmp, (BasePoint) { b.maxx, b.maxy } );
    SplineSetLineTo(ss_tmp, (BasePoint) { b.maxx, b.miny } );
    SplineMake3(ss_tmp->last, ss_tmp->first);
    ss_tmp->last = ss_tmp->first;
    ss->next = ss_tmp;
    ss=SplineSetRemoveOverlap(NULL,ss,over_remove);
    SplineSetsCorrect(ss, &trash);
    for ( ss_tmp=ss; ss_tmp!=NULL; ss_last=ss_tmp, ss_tmp=ss_tmp->next ) {
	if ( ss_tmp->first->me.x==b.minx || ss_tmp->first->me.x==b.maxx ) {
	    if ( ss_last==NULL )
		ss = ss->next;
    	    else
		ss_last->next = ss_tmp->next;
	    ss_tmp->next = NULL;
	    SplinePointListFree(ss_tmp);
	    return ss;
	}
    }
    assert(0);
    return ss;
}

/******************************************************************************/
/* ****************************** Nib Geometry ****************************** */
/******************************************************************************/

/* A contour is a valid convex polygon if:
 *  1. It contains something (not a line or a single point
 *     but a closed piecewise spline containing area)
 *  2. All edges/splines are lines
 *  3. It is convex
 *  4. There are no extraneous points on the edges
 *
 * A contour is a valid nib shape if:
 *  5. It's on-curve points would form a valid convex
 *     polygon if there were no control points.
 *  6. Every edge/spline is either:
 *     6a. A line, or
 *     6b. A spline where *both* control points are:
 *         6bi.  Inside the triangle formed by the spline's from/to edge
 *               and extending the lines of the adjacent edges**, and
 *         6bii. The line segment of each control point does not
 *               cross the extended line of the other control point.
 *         ** Simplification -- see documentation and lref_cp below.
 */
enum ShapeType NibIsValid(SplineSet *ss) {
    Spline *s;
    BasePoint lref_cp;
    bigreal d, anglesum = 0, angle, last_angle;
    int n = 1;

    if ( ss->first==NULL )
	return Shape_TooFewPoints;
    if ( ss->first->prev==NULL )
	return Shape_NotClosed;
    if ( SplinePointListIsClockwise(ss)!=1 )
	return Shape_CCW;
    if ( ss->first->next->order2 )
	return Shape_Quadratic;

    s = ss->first->prev;
    last_angle = atan2(s->to->me.y - s->from->me.y,
                       s->to->me.x - s->from->me.x);
    if ( SplineIsLinear(s) )
	lref_cp = s->from->me;
    else
	lref_cp = s->to->prevcp;

    s = ss->first->next;
    SplinePointListSelect(ss, false);
    SplinePointListClearCPSel(ss);
    // Polygonal checks
    while ( true ) {
	s->from->selected = true;
	if ( BPWithin(s->from->me, s->to->me,1e-2) )
	    return Shape_TinySpline;
	angle = atan2(s->to->me.y - s->from->me.y, s->to->me.x - s->from->me.x);
	if ( RealWithin(angle, last_angle, 1e-4) )
	    return Shape_PointOnEdge;
	d = last_angle-angle;
	d = NormAngle(d);
	if ( d<0 )
	    return Shape_CCWTurn;
	anglesum += d;
	s->from->selected = false;
	last_angle = angle;
	s=s->to->next;
	if ( s==ss->first->next )
	    break;
	++n;
    }
    if ( n<3 )
	return Shape_TooFewPoints;
    if ( !RealWithin(anglesum, 2*FF_PI, 1e-1) )
	return Shape_SelfIntersects;

    assert( s==ss->first->next );
    // Curve/control point checks
    while ( true ) {
	if ( SplineIsLinear(s) )
	    lref_cp = s->from->me;
	else {
	    if ( s->from->nonextcp || BPNear(s->from->nextcp, s->from->me) ||
	         s->to->noprevcp || BPNear(s->to->prevcp, s->to->me) )
		return Shape_HalfLinear;
	    s->from->nextcpselected = true;
	    if ( LineSameSide(s->from->me, s->to->me, s->from->nextcp,
	                      s->from->prev->from->me, false) )
		return Shape_BadCP_R1;
	    if ( !LineSameSide(lref_cp, s->from->me,
	                       s->from->nextcp, s->to->me, true) )
		return Shape_BadCP_R2;
	    if ( !LineSameSide(s->to->me, s->to->prevcp,
	                       s->from->nextcp, s->from->me, true) )
		return Shape_BadCP_R3;
	    s->from->nextcpselected = false;
	    s->from->selected = false;
	    s->to->selected = true;
	    s->to->prevcpselected = true;
	    if ( LineSameSide(s->from->me, s->to->me, s->to->prevcp,
	                      s->to->next->to->me, false) )
		return Shape_BadCP_R1;
	    if ( !LineSameSide(s->to->me, s->to->next->to->me,
	                       s->to->prevcp, s->from->me, true) )
		return Shape_BadCP_R2;
	    if ( !LineSameSide(s->from->me, s->from->nextcp,
	                       s->to->prevcp, s->to->me, true) )
		return Shape_BadCP_R3;
	    s->to->prevcpselected = false;
	    s->to->selected = false;
	    lref_cp = s->to->prevcp;
	}
	s=s->to->next;
	if ( s==ss->first->next )
	    break;
    }
    return Shape_Convex;
}

const char *NibShapeTypeMsg(enum ShapeType st) {
    switch (st) {
      case Shape_CCW:
	return _("The contour winds counter-clockwise; "
	         "a nib must wind clockwise.");
      case Shape_CCWTurn:
	return _("The contour bends or curves counter-clockwise "
	         "at the selected point; "
	         "all on-curve points must bend or curve clockwise.");
      case Shape_PointOnEdge:
	return _("The selected point is on a line; "
	         "all on-curve points must bend or curve clockwise.");
      case Shape_TooFewPoints:
	return _("A nib must have at least three on-curve points.");
      case Shape_Quadratic:
	return _("The contour is quadratic; a nib must be a cubic contour.");
      case Shape_NotClosed:
	return _("The contour is open; a nib must be closed.");
      case Shape_TinySpline:
	return _("The selected point is the start of a 'tiny' spline; "
	         "splines that small may cause inaccurate calculations.");
      case Shape_HalfLinear:
	return _("The selected point starts a spline with one control point; "
	         "nib splines need a defined slope at both points.");
      case Shape_BadCP_R1:
	return _("The selected control point's position violates Rule 1 "
	         "(see documentation).");
      case Shape_BadCP_R2:
	return _("The selected control point's position violates Rule 2 "
	         "(see documentation).");
      case Shape_BadCP_R3:
	return _("The selected control point's position violates Rule 3 "
	         "(see documentation).");
      case Shape_SelfIntersects:
	return _("The contour intersects itself; a nib must non-intersecting.");
      case Shape_Convex:
	return _("Unrecognized nib shape error.");
    }

    return NULL;
}

static void BuildNibCorners(NibCorner **ncp, SplineSet *nib, int *maxp,
                            int *n) {
    int i, max_utan_index = -1;
    BasePoint max_utanangle = UTMIN;
    SplinePoint *sp;
    NibCorner *nc = *ncp, *tpc;

    if ( nc==NULL )
	nc = calloc(*maxp,sizeof(NibCorner));

    for ( sp=nib->first, i=0; ; ) {
	if ( i==*maxp ) { // We guessed wrong
	    *maxp *= 2;
	    nc = realloc(nc, *maxp * sizeof(NibCorner));
	    memset(nc+i, 0, (*maxp-i) * sizeof(NibCorner));
	}
	nc[i].on_nib = sp;
	nc[i].linear = SplineIsLinear(sp->next);
	nc[i].utv[NC_IN_IDX] = SplineUTanVecAt(sp->prev, 1.0);
	nc[i].utv[NC_OUT_IDX] = SplineUTanVecAt(sp->next, 0.0);
	if ( JointBendsCW(nc[i].utv[NC_IN_IDX], nc[i].utv[NC_OUT_IDX]) )
	    // Flatten potential LineSameSide permissiveness
	    nc[i].utv[NC_OUT_IDX] = nc[i].utv[NC_IN_IDX];
	if (    UTanVecGreater(nc[i].utv[NC_IN_IDX], max_utanangle)
	     || UTNear(nc[i].utv[NC_IN_IDX], max_utanangle) ) {
	    max_utan_index = i;
	    max_utanangle = nc[i].utv[NC_IN_IDX];
	}
	++i;
	sp=sp->next->to;
	if ( sp==nib->first )
	    break;
    }
    *n = i;

    // Put in order of decreasing utanvec[NC_IN_IDX]
    assert( max_utan_index != -1 );
    if (max_utan_index != 0) {
	tpc = malloc(i*sizeof(NibCorner));
	memcpy(tpc, nc+max_utan_index, (i-max_utan_index)*sizeof(NibCorner));
	memcpy(tpc+(i-max_utan_index), nc, max_utan_index*sizeof(NibCorner));
	free(nc);
	nc = tpc;
	*maxp = i;
    }

    *ncp = nc;
}

/* The index into c->nibcorners associated with unit tangent ut
 */
static int _IndexForUTanVec(NibCorner *nc, int n, BasePoint ut, int nci_hint) {
    int nci;

    assert( nci_hint == -1 || (nci_hint >= 0 && nci_hint < n ) );

    if (   nci_hint != -1
        && UTanVecsSequent(nc[nci_hint].utv[NC_IN_IDX], ut,
	                   nc[N_NEXTI(n, nci_hint)].utv[NC_IN_IDX],
                           false) )
	nci = nci_hint;
    else // XXX Replace with binary search
	for (nci = 0; nci<n; ++nci)
	    if ( UTanVecsSequent(nc[nci].utv[NC_IN_IDX], ut,
	                         nc[N_NEXTI(n, nci)].utv[NC_IN_IDX],
	                         false) )
		break;
    assert(nci < n);
    return nci;
}

static int IndexForUTanVec(StrokeContext *c, BasePoint ut, int nci_hint) {
    return _IndexForUTanVec(c->nibcorners, c->n, ut, nci_hint);
}

/* The offset from the stroked curve associated with unit tangent ut
 * (or it's reverse).
 */
static NibOffset *_CalcNibOffset(NibCorner *nc, int n, BasePoint ut,
                                 int reverse, NibOffset *no, int nci_hint) {
    int nci, ncni, ncpi;
    Spline *ns;

    if ( no==NULL )
	no = malloc(sizeof(NibOffset));

    memset(no,0,sizeof(NibOffset));
    no->utanvec = ut; // Store the unreversed value for reference

    if ( reverse ) {
	ut = BPRev(ut);
	no->reversed = 1;
    }

    nci = no->nci[0] = no->nci[1] = _IndexForUTanVec(nc, n, ut, nci_hint);
    ncpi = N_PREVI(n, nci);
    ncni = N_NEXTI(n, nci);

    // The two cases where one of two points might draw the same angle
    // and therefore the only cases where the array values differ
    if (   nc[nci].linear
	// "Capsule" case
        && UTNear(ut, nc[ncni].utv[NC_IN_IDX])
        && UTNear(ut, nc[nci].utv[NC_IN_IDX]) ) {
	no->nt = 0.0;
	no->off[NIBOFF_CCW_IDX] = nc[nci].on_nib->me;
	no->off[NIBOFF_CW_IDX] = nc[ncni].on_nib->me;
	no->nci[NIBOFF_CW_IDX] = ncni;
	no->at_line = true;
	no->curve = false;
    } else if ( nc[ncpi].linear && UTNear(ut, nc[ncpi].utv[NC_OUT_IDX]) ) {
	// Other lines
	no->nt = 0.0;
	no->off[NIBOFF_CCW_IDX] = nc[ncpi].on_nib->me;
	no->nci[NIBOFF_CCW_IDX] = ncpi;
	no->off[NIBOFF_CW_IDX] = nc[nci].on_nib->me;
	no->at_line = true;
	no->curve = false;
    // When ut is (effectively) between IN and OUT the point
    // draws the offset curve
    } else if (   UTanVecsSequent(nc[nci].utv[NC_IN_IDX], ut,
                                  nc[nci].utv[NC_OUT_IDX], false)
               || UTNear(ut, nc[nci].utv[NC_OUT_IDX] ) ) {
	no->nt = 0.0;
	no->off[0] = no->off[1] = nc[nci].on_nib->me;
	no->curve = false;
    // Otherwise ut is on a spline curve clockwise from point nci
    } else {
	assert( UTanVecsSequent(nc[nci].utv[NC_OUT_IDX], ut,
	                        nc[ncni].utv[NC_IN_IDX], false) );
	// Nib splines are locally convex and therefore have t value per slope
	ns = nc[nci].on_nib->next;
	no->nt = SplineSolveForUTanVec(ns, ut, 0.0, false);
	if ( no->nt<0 ) {
	    // At more extreme nib control point angles the solver may fail.
	    // In such cases the tangent angle should be near one of the 
	    // endpoints, so pick the closer one
	    if (   BPLenSq(BPSub(nc[nci].utv[NC_OUT_IDX], ut))
	         < BPLenSq(BPSub(nc[ncni].utv[NC_IN_IDX], ut)) )
		no->nt = 0;
	    else
		no->nt = 1;
	}
	no->off[0] = no->off[1] = SPLINEPVAL(ns, no->nt);
	no->curve = true;
    }
    return no;
}

static NibOffset *CalcNibOffset(StrokeContext *c, BasePoint ut, int reverse,
                                NibOffset *no, int nci_hint) {
    return _CalcNibOffset(c->nibcorners, c->n, ut, reverse, no, nci_hint);
}

static BasePoint SplineStrokeNextAngle(StrokeContext *c, BasePoint ut,
                                       int is_ccw, int *curved, int reverse,
				       int nci_hint) {
    int nci, ncni, ncpi, inout;

    ut = BPRevIf(reverse, ut);
    nci = IndexForUTanVec(c, ut, nci_hint);
    ncni = NC_NEXTI(c, nci);
    ncpi = NC_PREVI(c, nci);

    if ( UTNear(ut, c->nibcorners[nci].utv[NC_IN_IDX]) ) {
	if ( is_ccw ) {
	    if ( UTNear(c->nibcorners[nci].utv[NC_IN_IDX],
	                c->nibcorners[ncpi].utv[NC_OUT_IDX]) ) {
		if ( UTNear(c->nibcorners[nci].utv[NC_IN_IDX],
		            c->nibcorners[ncpi].utv[NC_IN_IDX]) ) {
		    assert(c->nibcorners[ncpi].linear);
		    *curved = true;
		    inout = NC_OUT_IDX;
		    nci = NC_PREVI(c, ncpi);
		} else {
		    assert(c->nibcorners[ncpi].linear);
		    inout = NC_IN_IDX;
		    *curved = false;
		    nci = ncpi;
		}
	    } else {
		inout = NC_OUT_IDX;
		*curved = true;
		nci = ncpi;
	    }
	} else { // CW
	    if ( UTNear(c->nibcorners[nci].utv[NC_IN_IDX],
	                c->nibcorners[nci].utv[NC_OUT_IDX]) ) {
		if ( UTNear(c->nibcorners[nci].utv[NC_IN_IDX],
		            c->nibcorners[ncni].utv[NC_IN_IDX]) ) {
		    assert(c->nibcorners[nci].linear);
		    inout = NC_OUT_IDX;
		    *curved = true;
		} else {
		    inout = NC_IN_IDX;
		    *curved = !c->nibcorners[nci].linear;
		}
		nci = ncni;
	    } else {
		inout = NC_OUT_IDX;
		*curved = false;
	    }
	}
    } else if ( UTNear(ut, c->nibcorners[nci].utv[NC_OUT_IDX]) ) {
	assert( ! UTNear(c->nibcorners[nci].utv[NC_IN_IDX],
	                 c->nibcorners[nci].utv[NC_OUT_IDX]) );
	if ( is_ccw ) {
	    inout = NC_IN_IDX;
	    *curved = false;
	} else { // CW
	    if ( UTNear(c->nibcorners[nci].utv[NC_OUT_IDX],
	                c->nibcorners[ncni].utv[NC_IN_IDX]) ) {
		assert(c->nibcorners[nci].linear);
		inout = NC_OUT_IDX;
		*curved = !c->nibcorners[ncni].linear;
	    } else {
		inout = NC_IN_IDX;
		*curved = true;
	    }
	    nci = ncni;
	}
    } else if ( UTanVecsSequent(c->nibcorners[nci].utv[NC_IN_IDX], ut,
                c->nibcorners[nci].utv[NC_OUT_IDX], false) ) {
	if ( is_ccw )
	    inout = NC_IN_IDX;
	else
	    inout = NC_OUT_IDX;
	*curved = false;
    } else {
	assert( UTanVecsSequent(c->nibcorners[nci].utv[NC_OUT_IDX], ut,
	        c->nibcorners[ncni].utv[NC_IN_IDX], false) );
	if ( is_ccw )
	    inout = NC_OUT_IDX;
	else {
	    nci = ncni;
	    inout = NC_IN_IDX;
	}
	*curved = true;
    }
    return BPRevIf(reverse, c->nibcorners[nci].utv[inout]);
}

/******************************************************************************/
/* *************************** Spline Manipulation ************************** */
/******************************************************************************/

/* This is a convenience function that also concisely demonstrates
 * the calculation of an offsetted point.
 */
BasePoint SplineOffsetAt(StrokeContext *c, Spline *s, bigreal t, int is_right) {
    int is_ccw;
    BasePoint xy, ut;
    NibOffset no;

    // The coordinate of the spline at t
    xy = SPLINEPVAL(s, t);
    // The turning direction of the spline at t
    is_ccw = SplineTurningCCWAt(s, t);
    // The tangent angle of the spline at t
    ut = SplineUTanVecAt(s, t);

    // The offsets of the nib tracing at that angle (where is_right
    // reverses the angle and therefore the side of the nib)
    CalcNibOffset(c, ut, is_right, &no, -1);

    // (The spline coordinate at t) + (the offset). If the angle is on a
    // nib line, use the turning direction to pick the corner that will be
    // continuous with the next points to be drawn based on the turning
    // direction (and therefore the next angle).
    return BPAdd(xy, no.off[is_ccw]);
}

/* Copies the portion of s from t_fm to t_to and then translates
 * and appends it to tailp. The new end point is returned. "Reversing"
 * t_fm and t_to reverses the copy's direction.
 *
 * Direct calculations cribbed from https://stackoverflow.com/a/879213
 */
SplinePoint *AppendCubicSplinePortion(Spline *s, bigreal t_fm, bigreal t_to,
                                      SplinePoint *tailp) {
    extended u_fm = 1-t_fm, u_to = 1-t_to;
    SplinePoint *sp;
    BasePoint v, qf, qcf, qct, qt;

    // XXX maybe this should be length based
    if ( RealWithin(t_fm, t_to, 1e-4) )
	return tailp;

    // Intermediate calculations
    qf.x =    s->from->me.x*u_fm*u_fm
            + s->from->nextcp.x*2*t_fm*u_fm
            + s->to->prevcp.x*t_fm*t_fm;
    qcf.x =   s->from->me.x*u_to*u_to
            + s->from->nextcp.x*2*t_to*u_to
            + s->to->prevcp.x*t_to*t_to;
    qct.x =   s->from->nextcp.x*u_fm*u_fm
            + s->to->prevcp.x*2*t_fm*u_fm
            + s->to->me.x*t_fm*t_fm;
    qt.x =    s->from->nextcp.x*u_to*u_to
            + s->to->prevcp.x*2*t_to*u_to
            + s->to->me.x*t_to*t_to;

    qf.y =    s->from->me.y*u_fm*u_fm
	    + s->from->nextcp.y*2*t_fm*u_fm
            + s->to->prevcp.y*t_fm*t_fm;
    qcf.y =   s->from->me.y*u_to*u_to
            + s->from->nextcp.y*2*t_to*u_to
            + s->to->prevcp.y*t_to*t_to;
    qct.y =   s->from->nextcp.y*u_fm*u_fm
            + s->to->prevcp.y*2*t_fm*u_fm
            + s->to->me.y*t_fm*t_fm;
    qt.y =    s->from->nextcp.y*u_to*u_to
            + s->to->prevcp.y*2*t_to*u_to
            + s->to->me.y*t_to*t_to;

    // Difference vector to offset other points
    v.x = tailp->me.x - (qf.x*u_fm + qct.x*t_fm);
    v.y = tailp->me.y - (qf.y*u_fm + qct.y*t_fm);

    sp = SplinePointCreate(qcf.x*u_to + qt.x*t_to + v.x,
                            qcf.y*u_to + qt.y*t_to + v.y);

    tailp->nextcp.x = qf.x*u_to + qct.x*t_to + v.x;
    tailp->nextcp.y = qf.y*u_to + qct.y*t_to + v.y;
    sp->prevcp.x = qcf.x*u_fm + qt.x*t_fm + v.x;
    sp->prevcp.y = qcf.y*u_fm + qt.y*t_fm + v.y;

    SplineMake3(tailp,sp);

    if ( SplineIsLinear(tailp->next)) { // Linearish instead?
        tailp->nextcp = tailp->me;
        sp->prevcp = sp->me;
        SplineRefigure(tailp->next);
    }
    return sp;
}

/* Copies the splines between spline_fm (at t_fm) and spline_to (at t_to)
 * after point tailp. "backward" (and therefore forward) is relative
 * to next/prev rather than clockwise/counter-clockwise.
 */
SplinePoint *AppendCubicSplineSetPortion(Spline *spline_fm, bigreal t_fm,
                                         Spline *spline_to, bigreal t_to,
					 SplinePoint *tailp, int backward) {
    Spline *s;

    if (    backward && RealWithin(t_fm, 0.0, 1e-4)
         && spline_fm!=spline_to ) {
	t_fm = 1;
	spline_fm = spline_fm->from->prev;
    } else if (    !backward && RealWithin(t_fm, 1.0, 1e-4)
                && spline_fm != spline_to ) {
	t_fm = 0;
	spline_fm = spline_fm->to->next;
    }
    if (    backward && RealWithin(t_to, 1.0, 1e-4)
         && spline_fm != spline_to ) {
	t_to = 0.0;
	spline_to = spline_to->to->next;
    } else if (    !backward && RealWithin(t_to, 0.0, 1e-4)
                && spline_fm != spline_to ) {
	t_to = 1.0;
	spline_to = spline_to->from->prev;
    }
    s = spline_fm;

    // Handle the single spline case
    if (    s==spline_to
         && (( t_fm<=t_to && !backward ) || (t_fm>=t_to && backward))) {
	tailp = AppendCubicSplinePortion(s, t_fm, t_to, tailp);
	return tailp;
    }

    tailp = AppendCubicSplinePortion(s, t_fm, backward ? 0 : 1, tailp);

    while ( 1 ) {
        s = backward ? s->from->prev : s->to->next;
	if ( s==spline_to )
	    break;
	assert( s!=NULL && s!=spline_fm ); // XXX turn into runtime warning?
        tailp = AppendCubicSplinePortion(s, backward ? 1 : 0,
	                                  backward ? 0 : 1, tailp);
    }
    tailp = AppendCubicSplinePortion(s, backward ? 1 : 0, t_to, tailp);
    return tailp;
}

/******************************************************************************/
/* *********************** Tracing, Caps and Joins ************************** */
/******************************************************************************/

static SplinePoint *AddNibPortion(NibCorner *nc, SplinePoint *tailp,
                                  NibOffset *no_fm, int is_ccw_fm,
				  NibOffset *no_to, int is_ccw_to, int bk) {
    SplinePoint *sp, *nibp_fm, *nibp_to;

    nibp_fm = nc[no_fm->nci[is_ccw_fm]].on_nib;
    nibp_to = nc[no_to->nci[is_ccw_to]].on_nib;
    sp = AppendCubicSplineSetPortion(nibp_fm->next, no_fm->nt, nibp_to->next,
                                     no_to->nt, tailp, bk);
    return sp;
}

void SSAppendArc(SplineSet *cur, bigreal major, bigreal minor, 
                 BasePoint ang, BasePoint ut_fm, BasePoint ut_to,
                 int bk, int limit) {
    SplineSet *ellip;
    real trans[6];
    SplinePoint *sp;
    NibOffset no_fm, no_to;
    NibCorner *nc = NULL;
    int n, mn=4;

    if ( ang.y==0 )
	ang.x = 1;
    if ( minor==0 )
	minor = major;

    ellip = UnitShape(0);
    trans[0] = ang.x * major;
    trans[1] = ang.y * major;
    trans[2] = -ang.y * minor;
    trans[3] = ang.x * minor;
    trans[4] = trans[5] = 0;
    SplinePointListTransformExtended(ellip, trans, tpt_AllPoints,
	                             tpmask_dontTrimValues);
    BuildNibCorners(&nc, ellip, &mn, &n);
    assert( n==4 && nc!=NULL );

    _CalcNibOffset(nc, n, ut_fm, false, &no_fm, -1);
    _CalcNibOffset(nc, n, ut_to, false, &no_to, -1);
    if ( limit ) {
	if (    !bk && no_fm.nci[0] == no_to.nci[0]
	     && no_fm.nt > no_to.nt )
	    bk = true;
	else if ( !bk && no_fm.nci[0] == (no_to.nci[0]+1)%4 )
	    bk = true;
	else if (    bk && no_fm.nci[0] == no_to.nci[0]
	     && no_fm.nt < no_to.nt )
	    bk = false;
	else if ( bk && (no_fm.nci[0]+1)%4 == no_to.nci[0] )
	    bk = false;
    }
    sp = AddNibPortion(nc, cur->last, &no_fm, false, &no_to, false, bk);
    cur->last = sp;
    SplinePointListFree(ellip);
    free(nc);
}

void SplineStrokeSimpleFixup(SplinePoint *tailp, BasePoint p) {
    BasePoint dxy = BPSub(p, tailp->me);
    tailp->prevcp = BPAdd(tailp->prevcp, dxy);
    tailp->me = p;
    SplineRefigure(tailp->prev);
}

static BasePoint SplineStrokeVerifyCorner(BasePoint sxy, BasePoint txy,
                                          NibOffset *no, int is_ccw,
				          BasePoint *dxyp, bigreal *mgp) {
    BasePoint oxy;

    oxy = BPAdd(sxy, no->off[is_ccw]);
    *dxyp = BPSub(oxy, txy);
    *mgp = fmax(fabs(dxyp->x), fabs(dxyp->y));
    return oxy;
}

static int SplineStrokeFindCorner(BasePoint sxy, BasePoint txy, NibOffset *no) {
    bigreal mg, mg2;
    BasePoint tmp;

    SplineStrokeVerifyCorner(sxy, txy, no, 0, &tmp, &mg);
    SplineStrokeVerifyCorner(sxy, txy, no, 1, &tmp, &mg2);
    return mg > mg2 ? 1 : 0;
}

/* Put the new endpoint exactly where the NibOffset calculation says it
 * should be to avoid cumulative append errors.
 */
static int SplineStrokeAppendFixup(SplinePoint *tailp, BasePoint sxy,
                                   NibOffset *no, int is_ccw, bigreal fudge) {
    bigreal mg, mg2;
    BasePoint oxy, oxy2, dxy, dxy2;

    if ( is_ccw==-1 ) {
	oxy  = SplineStrokeVerifyCorner(sxy, tailp->me, no, 0, &dxy, &mg);
	oxy2 = SplineStrokeVerifyCorner(sxy, tailp->me, no, 1, &dxy2, &mg2);
	if ( mg2<mg ) {
	    mg = mg2;
	    oxy = oxy2;
	    dxy = dxy2;
	    is_ccw = 1;
	} else
	    is_ccw = 0;
    } else
	oxy = SplineStrokeVerifyCorner(sxy, tailp->me, no, is_ccw, &dxy, &mg);

    // assert( mg < 1 );
    if ( mg > FIXUP_MARGIN*fudge ) {
	LogError(_("Warning: Coordinate diff %lf greater than margin %lf"),
	         mg, FIXUP_MARGIN*fudge);
    }

    tailp->prevcp = BPAdd(tailp->prevcp, dxy);
    tailp->me = oxy;
    SplineRefigure(tailp->prev);
    return is_ccw;
}

/* Note that is_ccw relates to the relevant NibOffset entries, and not
 * necessarily the direction of the curve at t.
 */
static int OffsetOnCuspAt(StrokeContext *c, Spline *s, bigreal t,
                          NibOffset *nop, int is_right, int is_ccw)
{
    bigreal cs, cn;
    NibOffset no;

    cs = SplineCurvature(s, t);

    // Cusps aren't a problem when increasing the radius of the curve
    if ( (cs>0)==is_right )
	return false;

    if ( nop==NULL ) {
	nop = &no;
	CalcNibOffset(c, SplineUTanVecAt(s, t), is_right, nop, -1);
    }
    cn = SplineCurvature(c->nibcorners[nop->nci[is_ccw]].on_nib->next, nop->nt);

    return RealWithin(cn, 0, 1e-6) ? false : is_right ? (cn >= cs) : (cn >= -cs);
}

/* I didn't find or derive a closed form of this calculation, so this just
 * does a binary search within the range, which is assumed (here) to contain
 * a single transition.
 */
static bigreal SplineFindCuspSing(StrokeContext *c, Spline *s, bigreal t_fm,
                                  bigreal t_to, int is_right, int is_ccw,
				  bigreal margin, int on_cusp)
{
    bigreal t_mid;
    int cusp_mid;
    assert( t_fm >= 0.0 && t_fm <= 1.0 );
    assert( t_to >= 0.0 && t_to <= 1.0 );
    assert( t_fm < t_to );
    assert( OffsetOnCuspAt(c, s, t_fm, NULL, is_right, is_ccw) == on_cusp );
    assert( OffsetOnCuspAt(c, s, t_to, NULL, is_right, is_ccw) != on_cusp );
    while ( t_fm-t_to > margin ) {
	t_mid = (t_fm+t_to)/2;
	cusp_mid = OffsetOnCuspAt(c, s, t_mid, NULL, is_right, is_ccw);
	if ( cusp_mid==on_cusp )
	    t_fm = t_mid;
	else
	    t_to = t_mid;
    }
    return t_to;
}

typedef struct stroketraceinfo {
    StrokeContext *c;
    Spline *s;
    bigreal cusp_trans;
    int nci_hint;
    int num_points;
    unsigned int is_right: 1;
    unsigned int starts_on_cusp: 1;
    unsigned int first_pass: 1;
    unsigned int found_trans: 1;
} StrokeTraceInfo;

int GenStrokeTracePoints(void *vinfo, bigreal t_fm, bigreal t_to,
                         FitPoint **fpp) {
    StrokeTraceInfo *stip = (StrokeTraceInfo *)vinfo;
    int i, nib_ccw, on_cusp;
    NibOffset no;
    FitPoint *fp;
    bigreal nidiff, t;
    BasePoint xy;

    *fpp = NULL;
    fp = calloc(stip->num_points, sizeof(FitPoint));
    nidiff = (t_to - t_fm) / (stip->num_points-1);

    nib_ccw = SplineTurningCCWAt(stip->s, t_fm);
    no.nci[0] = no.nci[1] = stip->nci_hint;
    for ( i=0, t=t_fm; i<stip->num_points; ++i, t+=nidiff ) {
	if ( i==(stip->num_points-1) ) {
	    nib_ccw = !nib_ccw; // Stop at the closer corner
	    t = t_to; // side-step nidiff rounding errors
	}
	xy = SPLINEPVAL(stip->s, t);
	fp[i].ut = SplineUTanVecAt(stip->s, t);
	CalcNibOffset(stip->c, fp[i].ut, stip->is_right, &no, no.nci[nib_ccw]);
	fp[i].p = BPAdd(xy, no.off[nib_ccw]);
	fp[i].t = t;
	if ( stip->first_pass ) {
	    on_cusp = OffsetOnCuspAt(stip->c, stip->s, t, &no,
	                             stip->is_right, nib_ccw);
	    if ( on_cusp!=stip->starts_on_cusp ) {
		stip->found_trans = true;
		stip->cusp_trans = SplineFindCuspSing(stip->c, stip->s,
		                                      t-nidiff, t,
		                                      stip->is_right,
		                                      nib_ccw, CUSPD_MARGIN,
						      stip->starts_on_cusp);
		free(fp);
		return 0;
	    }
	} else {
#ifndef NDEBUG
	; // Could add consistency asserts for shorter passes here
#else
	;
#endif // NDEBUG
	}
	if ( stip->starts_on_cusp ) // Cusp tangent point the other way
	    fp[i].ut = BPRev(fp[i].ut);
    }
    *fpp = fp;
    stip->first_pass = false;
    return stip->num_points;
}

#define TRACE_CUSPS false
SplinePoint *TraceAndFitSpline(StrokeContext *c, Spline *s, bigreal t_fm,
                               bigreal t_to, SplinePoint *tailp,
			       int nci_hint, int is_right, int on_cusp) {
    SplinePoint *sp = NULL;
    StrokeTraceInfo sti;
    FitPoint *fpp;
    BasePoint xy;

    sti.c = c;
    sti.s = s;
    sti.nci_hint = nci_hint;
    sti.num_points = 10;
    sti.is_right = is_right;
    sti.first_pass = true;
    sti.starts_on_cusp = on_cusp;
    sti.found_trans = false;

    if ( on_cusp && !TRACE_CUSPS ) {
	GenStrokeTracePoints((void *)&sti, t_fm, t_to, &fpp);
	free(fpp);
    } else
	sp = ApproximateSplineSetFromGen(tailp, NULL, t_fm, t_to, c->acctarget,
	                                 false, &GenStrokeTracePoints,
	                                 (void *) &sti, false);
    if ( !sti.found_trans ) {
	if ( sp==NULL ) {
	    assert( on_cusp && !TRACE_CUSPS );
	    xy = SplineOffsetAt(c, s, t_to, is_right);
	    sp = SplinePointCreate(xy.x, xy.y);
	    SplineMake3(tailp, sp);
	}
   	return sp;
    }

    assert ( sti.found_trans && sp==NULL );
    assert ( sti.cusp_trans >= t_fm && sti.cusp_trans <= t_to );
    if ( on_cusp && !TRACE_CUSPS ) {
	xy = SplineOffsetAt(c, s, sti.cusp_trans, is_right);
	sp = SplinePointCreate(xy.x, xy.y);
	SplineMake3(tailp, sp);
    } else {
	sti.first_pass = false;
	sp = ApproximateSplineSetFromGen(tailp, NULL, t_fm, sti.cusp_trans,
	                                 c->acctarget, false,
	                                 &GenStrokeTracePoints, (void *) &sti,
	                                 false);
    }
    assert( sp!=NULL );
    sp->pointtype = pt_corner;
    on_cusp = !on_cusp;
    if ( RealWithin(sti.cusp_trans, t_to, CUSPD_MARGIN) )
	return sp;
    return TraceAndFitSpline(c, s, sti.cusp_trans, t_to, sp, nci_hint,
                             is_right, on_cusp);
}
#undef TRACE_CUSPS

static bigreal SplineStrokeNextT(StrokeContext *c, Spline *s, bigreal cur_t,
		                 int is_ccw, BasePoint *cur_ut,
				 int *curved, int reverse, int nci_hint) {
    int next_curved, icnt, i;
    bigreal next_t;
    extended poi[2];
    BasePoint next_ut;

    assert( cur_ut!=NULL && curved!=NULL );

    next_ut = SplineStrokeNextAngle(c, *cur_ut, is_ccw, &next_curved,
                                    reverse, nci_hint);
    next_t = SplineSolveForUTanVec(s, next_ut, cur_t, false);

    // If there is an inflection point before next_t the spline will start
    // curving in the opposite direction, so stop and trace the next section
    // separately. An alternative would be find the next stop angle in the
    // other direction by returning:
    //
    // return SplineStrokeNextT(c, s, poi[i], !is_ccw, cur_ut, curved,
    //                          reverse, nci_hint);
    //
    // This would have one main advantage, which is that the offset inflection
    // points will not exactly correspond to the position on s. Therefore
    // stopping at the s inflection leads to an offset point frustratingly close
    // to but not at the inflection point. The disadvantage is that we would
    // need to track how many times the direction changes to feed the right
    // value to SplineStrokeAppendFixup, leading to more code complexity.
    if ( (icnt = Spline2DFindPointsOfInflection(s, poi)) ) {
	assert ( icnt < 2 || poi[0] <= poi[1] );
	for ( i=0; i<2; ++i )
	    if (    poi[i] > cur_t
	         && !RealNear(poi[i], cur_t)
	         && (next_t==-1 || poi[i] < next_t) ) {
		next_t = poi[i];
		next_ut = SplineUTanVecAt(s, next_t);
		break;
	    }
    }
    if ( RealWithin(next_t, 1.0, 1e-4) ) // XXX distance-based?
	next_t = 1.0;

    if ( next_t==-1 ) {
	next_t = 1.0;
	*cur_ut = SplineUTanVecAt(s, next_t);
    } else
	*cur_ut = next_ut;

    *curved = next_curved;
    return next_t;
}

static void HandleFlat(SplineSet *cur, BasePoint sxy, NibOffset *noi,
                       int is_ccw) {
    BasePoint oxy;

    oxy = BPAdd(sxy, noi->off[is_ccw]);
    if ( !BPNear(cur->last->me, oxy) ) {
	assert( BPNear(cur->last->me, (BPAdd(sxy, noi->off[!is_ccw]))) );
	SplineSetLineTo(cur, oxy);
    }
}

static bigreal FalseStrokeWidth(StrokeContext *c, BasePoint ut) {
    SplineSet *tss;
    DBounds b;
    real trans[6];

    assert( RealNear(BPLenSq(ut),1) );
    // Rotate nib and grab height as span
    trans[0] = trans[3] = ut.x;
    trans[1] = ut.y;
    trans[2] = -ut.y;
    trans[4] = trans[5] = 0;
    tss = SplinePointListCopy(c->nib);
    SplinePointListTransformExtended(tss, trans, tpt_AllPoints,
	                             tpmask_dontTrimValues);
    SplineSetFindBounds(tss,&b);
    SplinePointListFree(tss);
    return b.maxy - b.miny;
}

static bigreal CalcJoinLength(bigreal fsw, BasePoint ut1, BasePoint ut2) {
    bigreal costheta = BPDot(ut1, BPRev(ut2));

    // Formula is fsw/sin(theta/2) but sin(theta/2) = sqrt((1-costheta)/2)
    return fsw / sqrt((1 - costheta)/2);
}

static bigreal CalcJoinLimit(StrokeContext *c, bigreal fsw) {
    if (c->joinlimit <= 0)
	return DBL_MAX;

    if (!c->jlrelative)
	return c->joinlimit;

    return c->joinlimit * fsw;
}

static bigreal CalcCapExtend(StrokeContext *c, bigreal fsw) {
    if (c->extendcap <= 0)
	return 0;

    if (!c->ecrelative)
	return c->extendcap;

    return c->extendcap * fsw;
}

static bigreal LineDist(BasePoint l1, BasePoint l2, BasePoint p) {
    assert (l1.x!=l2.x || l1.y!=l2.y);
    return fabs((l2.y-l1.y)*p.x - (l2.x-l1.x)*p.y + l2.x*l1.y -l2.y*l1.x)
           / BPDist(l1, l2);
}

static bigreal SplineEndCurvature(Spline *s, int at_end) {
    BasePoint v1, v2;
    bigreal l1_3, crs, tmp;

    if (SplineIsLinearish(s))
	return 0;

    if (at_end) {
	v1 = BPSub(s->to->me, s->to->prevcp);
	v2 = BPSub(s->from->nextcp, s->to->prevcp);
    } else {
	v1 = BPSub(s->from->nextcp, s->from->me);
	v2 = BPSub(s->to->prevcp, s->from->nextcp);
    }

    l1_3 = pow(BPLenSq(v1), 1.5);
    crs = BPCross(v1, v2);

    if ( RealWithin(l1_3, 0, 1e-15) )
	return CURVATURE_ERROR;

    tmp = (2.0/3.0) * crs / l1_3;

    if ( RealWithin(tmp, 0, 1e-5) )
	return 0;
    return tmp;
}

static bigreal AdjustLineCircle(BasePoint lp, BasePoint lut, BasePoint cp1,
                                BasePoint cv1) {
    bigreal D, E, a, b, c, d, r;

    D = BPCross(lut, BPSub(lp, cp1));
    E = BPCross(cv1, lut);
    a = 1 - E*E;
    b = -2*D*E;
    c = -D*D;
    d = b*b - 4*a*c;
    if (fabs(d*a*a) < 1e-4)
	d = 0;
    else
	d = sqrt(d);
    r = (-b + d)/(2*a);
    if (r < 0)
	r = (-b - d)/(2*a);
    return r;
}

static void AdjustCircles(BasePoint cp1, BasePoint cv1, bigreal *r1,
                          BasePoint cp2, BasePoint cv2, bigreal *r2, int ins) {
    bigreal fac = (ins ? -1.0 : 1.0), C, D, a, b, c, d, e;
    BasePoint A, B;

    A = BPAdd(BPSub(cp1, cp2),
               BPSub(BPScale(cv1, *r1), BPScale(cv2, *r2)));
    B = BPSub(BPScale(cv1, fac), cv2);
    C = *r1 + *r2*fac;
    D = 2*fac;
    a = BPLenSq(B) - D*D;
    b = 2*(BPDot(A, B) - C*D);
    c = BPLenSq(A) - C*C;
    d = b*b - 4*a*c;
    if (fabs(d*a*a) < 1e-4)
	d = 0;
    else
	d = sqrt(d);
    e = (-b + d)/(2*a);
    if (e < 0)
	e = (-b - d)/(2*a);
    *r1 = *r1 + fac*e;
    *r2 = *r2 + e;
}

BasePoint SVGArcClip(BasePoint lp, BasePoint lut, BasePoint center,
                     bigreal r, BasePoint p1, BasePoint p2, BasePoint pr) {
    BasePoint i, i2;
    int cnt = LineCircleTest(lp, lut, center, r);
    if (cnt > 0) {
	IntersectLineCircle(lp, lut, center, r, &i, &i2);
	i = CloserPoint(lp, i, i2);
	if ( !LineSameSide(p1, p2, i, pr, false) )
	    i = p1;
    } else {
	i = p1;
    }
    return i;
}

BasePoint ArcClip(BasePoint center, bigreal r, int neg,
                  BasePoint p, BasePoint i, bigreal clip_ratio) {
    int s = neg ? -1 : 1;
    bigreal start_angle, end_angle, angle_diff;
    BasePoint v;

    start_angle = atan2(p.y-center.y, p.x-center.x);
    end_angle = atan2(i.y-center.y, i.x-center.x);
    angle_diff = NormAngle(s*(end_angle - start_angle));
    if ( angle_diff < 0 ) // Renormalize angle difference to 0->2*pi
	angle_diff += 2*FF_PI;
    end_angle = NormAngle(start_angle + s * angle_diff * clip_ratio);

    v = (BasePoint) { cos(end_angle), sin(end_angle) };
    return BPAdd(center, BPScale(v, r));
}

/*static int IntersectLinesSlopes2(BasePoint *i, BasePoint lp1, BasePoint lut1,
                                 BasePoint lp2, BasePoint lut2) {
    bigreal cl1 = BPCross(lut1, lp1), cl2 = BPCross(lut2, lp2);
    bigreal cd = BPCross(lut1, lut2);
    if (fabs(cd) < 1e-12)
	return 0;
    i->x = (cl1*lut2.x - cl2*lut1.x)/cd;
    i->y = (cl1*lut2.y - cl2*lut1.y)/cd;
    return 1;
}

static void CalcDualBasis(BasePoint v1, BasePoint v2, BasePoint *q1,
                          BasePoint *q2) {
    if ( v1.x==0 ) {
	assert( v1.y!=0 && v2.x!=0 );
	q2->y = 0;
	q2->x = 1/v2.x;
	q1->y = 1/v1.y;
	q1->x = -v2.y / ( v1.y * v2.x );
    } else if ( v1.y==0 ) {
	assert( v2.y!=0 );
	q2->x = 0;
	q2->y = 1/v2.y;
	q1->x = 1/v1.x;
	q1->y = -v2.x / ( v1.x * v2.y );
    } else if ( v2.x==0 ) {
	assert( v2.y!=0 );
	q1->y = 0;
	q1->x = 1/v1.x;
	q2->y = 1/v2.y;
	q2->x = -v1.y / ( v2.y * v1.x );
    } else if ( v2.y==0 ) {
	q1->x = 0;
	q1->y = 1/v1.y;
	q2->x = 1/v2.x;
	q2->y = -v1.x / ( v2.x * v1.y );
    } else {
	q1->y = 1 / (v1.y - (v1.x * v2.y / v2.x));
	q2->y = 1 / (v2.y - (v2.x * v1.y / v1.x));
	q1->x = -v2.y * q1->y / v2.x;
	q2->x = -v1.y * q2->y / v1.x;
    }
    assert(    RealNear(BPDot(v1,*q1), 1) && RealNear(BPDot(v2,*q2), 1)
            && RealNear(BPDot(v1,*q2), 0) && RealNear(BPDot(v2,*q1), 0) );
}
*/

/* (When necessary) extends the splines drawn by an arbitrary nib at angle
 * ut (so either a cap or a 180 degree join) so have a butt geometry (for
 * a butt/square end or to add a semi-circle on to).
 */
static void CalcExtend(BasePoint refp, BasePoint ut, BasePoint op1,
                       BasePoint op2, bigreal min,
                       BasePoint *np1, BasePoint *np2) {
    bigreal extra=0, tmp;
    int intersects;
    BasePoint cop1 = BPAdd(op1, ut), cop2 = BPAdd(op2, ut);
    BasePoint clip1 = BPAdd(refp, BPScale(ut, min)), cliput = BP90CW(ut);
    BasePoint clip2 = BPAdd(clip1, cliput);

    if ( !LineSameSide(clip1, clip2, op1, refp, false) )
	extra = LineDist(clip1, clip2, op1);
    if ( !LineSameSide(clip1, clip2, op2, refp, false) ) {
	tmp = LineDist(clip1, clip2, op2);
	if (tmp > extra)
	    extra = tmp;
    }
    if (extra > 0) {
	clip1 = BPAdd(refp, BPScale(ut, min+extra));
	clip2 = BPAdd(clip1, cliput);
    }
    intersects = IntersectLines(np1, &op1, &cop1, &clip1, &clip2);
    VASSERT(intersects);
    intersects = IntersectLines(np2, &op2, &cop2, &clip1, &clip2);
    VASSERT(intersects);
}

static void DoubleBackJC(StrokeContext *c, SplineSet *cur, BasePoint sxy, 
                         BasePoint oxy, BasePoint ut, int bk, int is_cap) {
    bigreal fsw, ex;
    BasePoint p0, p1, p2, arcut;

    fsw = FalseStrokeWidth(c, BPNeg(ut));
    if ( is_cap )
        ex = CalcCapExtend(c, fsw);
    else if ( c->join==lj_miterclip )
	ex = CalcJoinLimit(c, fsw);
    else
	ex = 0;

    p0 = BPAdd(sxy, c->pseudo_origin);
    CalcExtend(p0, ut, cur->last->me, oxy, ex, &p1, &p2);
    if ( BPNear(cur->last->me, p1) )
	cur->last->me = p1;
    else
	SplineSetLineTo(cur, p1);
    if ( (is_cap && c->cap==lc_round) || (!is_cap && c->join==lj_round) ) {
	arcut = BPRevIf(bk, ut);
	SSAppendArc(cur, fsw/2, 0, UTZERO, arcut, BPRev(arcut), bk, false);
	assert( BPWithin(cur->last->me, p2,
	                 c->log_maxdim*INTERSPLINE_MARGIN*5) );
	cur->last->me = p2;
    } else
	SplineSetLineTo(cur, p2);
    if ( !BPNear(oxy, p2) )
	SplineSetLineTo(cur, oxy);
}

typedef struct joinparams {
    StrokeContext *c;
    Spline *s;
    SplineSet *cur;
    BasePoint sxy, oxy, ut_fm;
    NibOffset  *no_to;
    int ccw_fm, ccw_to, is_right, bend_is_ccw;
} JoinParams;

static void BevelJoin(JoinParams *jpp) {
    SplineSetLineTo(jpp->cur, jpp->oxy);
}

static void NibJoin(JoinParams *jpp) {
    NibOffset no_fm;
    SplinePoint *sp;
    int ccw_fm;
    bigreal lmd = jpp->c->log_maxdim;

    CalcNibOffset(jpp->c, jpp->ut_fm, jpp->is_right, &no_fm, -1);
    ccw_fm = SplineStrokeAppendFixup(jpp->cur->last, jpp->sxy, &no_fm, -1, lmd);
    sp = AddNibPortion(jpp->c->nibcorners, jpp->cur->last, &no_fm, ccw_fm,
                       jpp->no_to, jpp->ccw_to, !jpp->bend_is_ccw);
    SplineStrokeAppendFixup(sp, jpp->sxy, jpp->no_to, jpp->ccw_to, lmd);
    jpp->cur->last = sp;
}

static void MiterJoin(JoinParams *jpp);
/* With non-circular nibs it would be more accurate to call this
 * "RoundishJoin". A join geometry is determined by the last 
 * point of the previous offset spline, the first point of the next
 * offset spline, and the starting and ending tangent angles. This
 * function closes the join with the arc of the least eccentric ellipse 
 * compatible with those points and angles, which is the most commonly
 * suggested solution.
 *
 * There are a number of stack-exchange-type proposed solutions to this
 * problem, some with pseudocode, but I had a hard time getting any of
 * them working. The problem became much simpler in light of two papers.
 *
 * One is "Least eccentric ellipses for geometric Hermite interpolation" by
 * Femiani, Chuang, and Razdan (Computer Aided Geometric Design 29, 2012,
 * pp. 141-9). The paper models the ellipse as a quadratic *rational* bezier
 * curve and calculates the "weight" corresponding to the least eccentric
 * ellipse.  The first part of the code below calculates x1 and y1 from 
 * section 3.1 of the paper (by different means, and possibly with different
 * signs) and uses those to calculate w. Any potential problems should be
 * checked against the paper's method. 
 *
 * Once the correct ellipse is identified it is easiest (in terms of code
 * reuse) to produce a contour of that shape and append the arc with 
 * AddNibPortion, and this is what SSAppendArc does. In order to use it that
 * way one must convert the rational bezier curve parameters into major and
 * minor axis lengths and an angle. Happily the second paper "Characteristics
 * of conic segments in Bezier form" by Javier-Sanchez-Reyes (Proceedings 
 * of the IMProVe 2011, pp. 231-4) makes this quite easy. That paper and
 * the implementation below use complex arithmetic to calculate the center,
 * foci, and axes. 
 */
#ifdef _MSC_VER
/* MSVC doesn't support C99 complex numbers, fall back to miter join */
static void RoundJoin(JoinParams *jpp, int rv) {
    if ( rv ) {
	DoubleBackJC(jpp->c, jpp->cur, jpp->sxy, jpp->oxy, jpp->ut_fm,
	             jpp->is_right, 0);
	return;
    }
    MiterJoin(jpp);
}
#else
static void RoundJoin(JoinParams *jpp, int rv) {
    BasePoint p0, p1, p2, p1p, p12a, angle, h0, h2;
    bigreal p12d, x1, y1, w, major, minor;
    complex double b0, b1, b2, alpha, m, C, d, c, F1, F2;
    int intersects;

    p0 = jpp->cur->last->me;
    p2 = jpp->oxy;
    h0 = BPRevIf(jpp->is_right, jpp->ut_fm);
    h2 = BPRevIf(jpp->is_right, jpp->no_to->utanvec);

    if ( rv ) {
	DoubleBackJC(jpp->c, jpp->cur, jpp->sxy, jpp->oxy, jpp->ut_fm,
	             jpp->is_right, 0);
	return;
    }

    // First paper
    intersects = IntersectLinesSlopes(&p1, &p0, &h0, &p2, &h2);
    VASSERT(intersects);
    p1p = ProjectPointOnLine(p1, p0, NormVec(BPSub(p2,p0)));
    p12a = BPAvg(p0, p2);
    p12d = BPDist(p0, p2);
    x1 = 2 * BPDist(p1p, p12a) / p12d;
    y1 = 2 * BPDist(p1p, p1) / p12d;
    w = 1 / sqrt( x1*x1 + y1*y1 + 1);

    // Second paper
    b0 = p0.x + p0.y * I;
    b1 = p1.x + p1.y * I;
    b2 = p2.x + p2.y * I;
    alpha = 1/(1 - w*w);
    m = (b0 + b2)/2;
    C = (1 - alpha)*b1 + alpha*m;
    d = ((1-alpha)*(b1*b1)) + alpha*(b0*b2);
    c = csqrt(C*C - d);
    F1 = C + c;
    F2 = C - c;
    major = (cabs(F1-b0) + cabs(F2-b0))/2;
    minor = major * major - pow(cabs(c), 2); // note change of order
    if ( RealWithin(minor, 0, 1e-8) || minor < 0 ) {
	MiterJoin(jpp);
	return;
    }
    minor = sqrt(minor);
    angle = MakeUTanVec(creal(c), cimag(c));

    SSAppendArc(jpp->cur, major, minor, angle, h0, h2, jpp->is_right, true);
}
#endif

static void MiterJoin(JoinParams *jpp) {
    BasePoint ixy, refp, cow, coi, clip1, clip2, ut;
    int intersects;
    SplineSet *cur = jpp->cur;
    bigreal fsw, jlen, jlim;

    cow = BPAdd(cur->last->me, jpp->ut_fm);
    coi = BPAdd(jpp->oxy, jpp->no_to->utanvec);
    intersects = IntersectLines(&ixy, &cur->last->me, &cow, &coi, &jpp->oxy);
    assert(intersects); // Shouldn't be called with parallel tangents
    fsw = (  FalseStrokeWidth(jpp->c, BPNeg(jpp->ut_fm)) 
           + FalseStrokeWidth(jpp->c, BPNeg(jpp->no_to->utanvec)))/2;
    jlim = CalcJoinLimit(jpp->c, fsw);
    jlen = CalcJoinLength(fsw, jpp->ut_fm, jpp->no_to->utanvec);

    if ( jlen<=jlim ) {
	// Normal miter join
	SplineSetLineTo(cur, ixy);
	SplineSetLineTo(cur, jpp->oxy);
    } else {
	if ( jpp->c->join==lj_miter ) {
	    BevelJoin(jpp);
	    return;
	}
	refp = BPAdd(jpp->sxy, jpp->c->pseudo_origin);
	ut = NormVec(BPSub(cur->last->me, jpp->oxy));
	ut = jpp->bend_is_ccw ? BP90CW(ut) : BP90CCW(ut);
	clip1 = BPAdd(refp, BPScale(ut, jlim/2));
	clip2 = BPAdd(clip1, BP90CW(ut));
	if ( !LineSameSide(clip1, clip2, jpp->oxy, refp, false) ) {
	    // Don't trim past bevel
	    BevelJoin(jpp);
	    return;
	}
	if ( LineSameSide(clip1, clip2, ixy, refp, false) ) {
	    // Backup normal miter join (rounding problems)
	    SplineSetLineTo(cur, ixy);
	    SplineSetLineTo(cur, jpp->oxy);
	    return;
	}
	// Clipped miter join
	intersects = IntersectLines(&ixy, &cur->last->me, &cow, &clip1, &clip2);
	VASSERT(intersects);
	SplineSetLineTo(cur, ixy);
	intersects = IntersectLines(&ixy, &jpp->oxy, &coi, &clip1, &clip2);
	VASSERT(intersects);
	SplineSetLineTo(cur, ixy);
	SplineSetLineTo(cur, jpp->oxy);
    }
}

static void ArcsJoin(JoinParams *jpp) {
    bigreal t, K_fm, K_to, fsw_fm, fsw_to, fsw_avg, r_fm, r_to, i_dist;
    bigreal jlim, r_clip, clip_ratio, start_angle, end_angle;
    bigreal angle_diff, max_angle_diff, bevel_angle_diff;
    BasePoint ut_to, p_fm, p_to, cv_fm, cv_to, center_fm, center_to;
    BasePoint i, i2, cut_fm, cut_to;
    BasePoint orig_p, ut_avg, iut, p_cref, ut_cref, center_clip;
    BasePoint ci_fm, ci_to, ci2, p_ref;
    SplinePoint *tmp_st, *tmp_end;
    int curved = false, cnt, note, neg_fm, neg_to, clipped = false, s_clip;
    int flat_fm, flat_to;

    p_fm = jpp->cur->last->me;
    p_to = jpp->oxy;
    ut_to = jpp->no_to->utanvec;

    /* Arcs is the only join style with parameters beyond the position and
     * slope at the offset spline endpoints. The "from" spline is available
     * as jpp->cur->last->from but the "to" spline is not yet built.
     *
     * We could calculate its curvature by the curvature of the
     * source spline and the nib point, but the point fitting/estimation
     * process may yield a visibly different result. We could instead
     * reorganize the code so that joins are retroactively applied, but
     * that would both take effort and leave the code less readable.
     * Instead we build the "to" spline here (when necessary), extract the
     * parameters, and delete it.
     *
     * Regardless if either "join point" is on a cusp the curvature of the
     * source spline has been "overwhelmed" to the opposite sign by the 
     * curvature of the nib. In that case we just use the unadjusted source
     * curvature, which (because the radius is relatively small) will either
     * be adjusted to an appropriate larger value or subject to the Round
     * join fallback. 
     */

    t = SplineStrokeNextT(jpp->c, jpp->s, 0.0, jpp->ccw_to, &ut_to, &curved,
                          jpp->is_right, -1);
    ut_to = jpp->no_to->utanvec; // Set back after call

    if ( curved ) {
	if ( OffsetOnCuspAt(jpp->c, jpp->s, 0.0, jpp->no_to,
	                    jpp->is_right, jpp->ccw_to) ) {
	    // printf("Outgoing on cusp - using source curvature\n");
	    K_to = SplineEndCurvature(jpp->s, false);
	} else {
	    tmp_st = SplinePointCreate(p_to.x, p_to.y);
	    tmp_end = TraceAndFitSpline(jpp->c, jpp->s, 0.0, t, tmp_st, -1,
	                                jpp->is_right, false);
	    K_to = SplineEndCurvature(tmp_st->next, false);
	    while ( tmp_st!=tmp_end ) {
		tmp_st = tmp_st->next->to;
		free(tmp_st->prev->from);
		free(tmp_st->prev);
	    }
	    free(tmp_end);
	}
    } else {
	// If the spline will be drawn by a nib point as opposed to a nib curve
	// then the curvature will just be that of the source spline.
	K_to = SplineEndCurvature(jpp->s, false);
    }
    if ( OffsetOnCuspAt(jpp->c, jpp->s->from->prev, 1.0, NULL,
	                jpp->is_right, jpp->ccw_fm) ) {
	//printf("Incoming on cusp - using source curvature\n");
	K_fm = SplineEndCurvature(jpp->s->from->prev, false);
    } else
	K_fm = SplineEndCurvature(jpp->cur->last->prev, true);

    fsw_fm = FalseStrokeWidth(jpp->c, BPNeg(jpp->ut_fm));
    fsw_to = FalseStrokeWidth(jpp->c, BPNeg(jpp->no_to->utanvec));
    fsw_avg = (fsw_fm + fsw_to) / 2.0;

    flat_fm = K_fm==0;
    flat_to = K_to==0;

    // printf(" K_fm: %.15lf, K_to: %.15lf\n", K_fm, K_to);
    if ( flat_fm && flat_to ) {
	MiterJoin(jpp);
	return;
    }

    // Calculate derived parameters
    neg_fm = (K_fm < 0);
    neg_to = (K_to < 0);

    if ( !flat_fm ) {
	r_fm = fabs(1.0/K_fm);
	cv_fm = BPRevIf(neg_fm, BP90CCW(jpp->ut_fm));
	center_fm = BPAdd(p_fm, BPScale(cv_fm, r_fm));
    }
    if ( !flat_to ) {
	r_to = fabs(1.0/K_to);
	cv_to = BPRevIf(neg_to, BP90CCW(ut_to));
	center_to = BPAdd(p_to, BPScale(cv_to, r_to));
    }

    // Find unclipped intersection
    if ( flat_fm ) {
	cnt = LineCircleTest(p_fm, jpp->ut_fm, center_to, r_to);
	if ( cnt > 0 )
	    IntersectLineCircle(p_fm, jpp->ut_fm, center_to, r_to, &i, &i2);
	else {
	    r_to = AdjustLineCircle(p_fm, jpp->ut_fm, p_to, cv_to);
	    center_to = BPAdd(p_to, BPScale(cv_to, r_to));
	    cnt = 1;
	    i = ProjectPointOnLine(center_to, p_fm, jpp->ut_fm);
	}
    } else if ( flat_to ) {
	cnt = LineCircleTest(p_to, ut_to, center_fm, r_fm);
	if ( cnt > 0 )
	    IntersectLineCircle(p_to, ut_to, center_fm, r_fm, &i, &i2);
	else {
	    r_fm = AdjustLineCircle(p_to, ut_to, p_fm, cv_fm);
	    center_fm = BPAdd(p_fm, BPScale(cv_fm, r_fm));
	    cnt = 1;
	    i = ProjectPointOnLine(center_fm, p_to, ut_to);
	}
    } else {
	note = cnt = CirclesTest(center_fm, r_fm, center_to, r_to);
	if ( cnt > 0 )
	    IntersectCircles(center_fm, r_fm, center_to, r_to, &i, &i2);
	else {
	    if ( note==-2 )
		AdjustCircles(p_to, cv_to, &r_to, p_fm, cv_fm, &r_fm, 1);
	    else
		AdjustCircles(p_fm, cv_fm, &r_fm, p_to, cv_to, &r_to, note<0);
	    center_fm = BPAdd(p_fm, BPScale(cv_fm, r_fm));
	    center_to = BPAdd(p_to, BPScale(cv_to, r_to));
	    cnt = 1;
	    if ( note==-2 )
		i = BPAdd(center_to,
		           BPScale(NormVec(BPSub(center_fm, center_to)),
		                    r_to));
	    else
		i = BPAdd(center_fm,
		           BPScale(NormVec(BPSub(center_to, center_fm)),
		                    r_fm));
	}
    }

    // A point definitely on the far side of the bevel line relative to
    // the source spline
    p_ref = BPAdd(p_fm, BPRevIf(jpp->bend_is_ccw,
                                   BP90CW(BPSub(p_to, p_fm))));

    if ( cnt > 1 )
	i = CloserIntersection(p_fm, p_to, i, i2, p_ref);

    // A variant of the SVG round fallback. It is different in that a) we 
    // fall back after the intersection adjustment, b) we check each 
    // curvature against its own false stroke width (as these can be 
    // different with arbitrary nibs), and c) we also fall back if both
    // intersection points are behind the bevel line (which can happen
    // in certain unusual situations). Only the first difference applies
    // in the case of a circular nib.
    if (   ( !flat_fm && r_fm<fsw_fm/2 ) || ( !flat_to && r_to<fsw_to/2 )
        || !LineSameSide(p_fm, p_to, i, p_ref, false) ) {
	// printf("Falling back to Round\n", r_fm, r_to);
	RoundJoin(jpp, false);
	return;
    }

    // Clipping pre-calculations
    orig_p = BPAdd(jpp->sxy, jpp->c->pseudo_origin);
    ut_avg = NormVec(BPScale(BPSub(jpp->ut_fm, ut_to), 0.5)),
    iut = BPSub(i, orig_p);
    i_dist = sqrt(BPLenSq(iut));
    iut = BPScale(iut, 1.0/i_dist);
    jlim = CalcJoinLimit(jpp->c, fsw_avg);

    // There are two clipping styles. SVG 2 specifies a clip line normal
    // to a circle at an arc distance away from the source join point.
    //
    // The alternate style uses the same circle to measure the total length
    // of the join and the distance from the source join point to the bevel
    // line, and uses these to calculate the portion of each line between 
    // the offset join points and the calculated intersection that should 
    // not be clipped. The latter style works for any nib while the former
    // will fail for some combinations of non-circular nib and source contour.
    s_clip = PVPCircle(orig_p, ut_avg, i, &center_clip, &r_clip);
    if ( s_clip==0 ) {
	// If there is no clipping circle use a line instead
	if ( jlim/2.0 < i_dist ) { // Linear test
	    clipped = true;
	    if ( !jpp->c->ratio_al ) {
		p_cref = BPAdd(orig_p, BPScale(iut, jlim/2.0));
		ut_cref = BP90CCW(iut);
	    } else {
		p_cref = ProjectPointOnLine(orig_p, p_fm,
		                            NormVec(BPSub(p_to, p_fm)));
		t = BPDist(orig_p, p_cref);
		if ( t >= jlim/2.0 )
		    clip_ratio = 0;
		else
		    clip_ratio = (jlim/2.0 - t)/(i_dist - t);
	    }
	}
    } else if ( jlim/2.0 < i_dist*FF_PI/2.0 ) { // Conservative half-circle test
	// Could probably do this with utanvecs but radian angles are less
	// confusing to add and subtract
	max_angle_diff = jlim/(r_clip*2);
	start_angle = atan2(orig_p.y-center_clip.y, orig_p.x-center_clip.x);
	end_angle = atan2(i.y-center_clip.y, i.x-center_clip.x);
	angle_diff = NormAngle(s_clip*(start_angle - end_angle));
	if ( angle_diff < 0 ) // Renormalize angle difference to 0->2*pi
	    angle_diff += 2*FF_PI;

	// Accurate test
	if ( angle_diff > max_angle_diff ) {
	    clipped = true;
	    if ( !jpp->c->ratio_al ) {
		end_angle = NormAngle(start_angle - s_clip * max_angle_diff);
		ut_cref = (BasePoint) { cos(end_angle), sin(end_angle) };
		p_cref = BPAdd(center_clip, BPScale(ut_cref, r_clip));
	    } else {
		IntersectLineCircle(p_fm, NormVec(BPSub(p_to, p_fm)),
		                    center_clip, r_clip, &ci_fm, &ci2);
		ci_fm = CloserPoint(p_fm, ci_fm, ci2);
		end_angle = atan2(ci_fm.y-center_clip.y, ci_fm.x-center_clip.x);
		bevel_angle_diff = NormAngle(s_clip*(start_angle - end_angle));
		if (bevel_angle_diff >= max_angle_diff)
		    clip_ratio = 0;
		else
		    clip_ratio =   (max_angle_diff - bevel_angle_diff)
		                 / (angle_diff - bevel_angle_diff);
	    }
	}
    }

    if ( flat_fm ) {
	if ( clipped && !jpp->c->ratio_al ) {
	    IntersectLinesSlopes(&ci_fm, &p_fm, &jpp->ut_fm, &p_cref, &ut_cref);
	    if ( !LineSameSide(p_fm, p_to, ci_fm, p_ref, false) )
		ci_fm = p_fm;
	} else if ( clipped ) {
	    i_dist = BPDist(i, p_fm);
	    ci_fm = BPAdd(p_fm, BPScale(jpp->ut_fm, i_dist * clip_ratio));
	} else
	    ci_fm = i;
	if ( !BPEq(ci_fm, p_fm) )
	    SplineSetLineTo(jpp->cur, ci_fm);
    } else {
	if ( clipped && !jpp->c->ratio_al )
	    ci_fm = SVGArcClip(p_cref, ut_cref, center_fm, r_fm, p_fm, p_to,
	                       p_ref);
	else if ( clipped )
	    ci_fm = ArcClip(center_fm, r_fm, neg_fm, p_fm, i, clip_ratio);
	else
	    ci_fm = i;
	if ( !BPEq(ci_fm, p_fm) ) {
	    cut_fm = BP90CW(NormVec(BPSub(ci_fm, center_fm)));
	    SSAppendArc(jpp->cur, r_fm, r_fm, UTZERO,
	                BPRevIf(!neg_fm, jpp->ut_fm), cut_fm, !neg_fm, true);
	    SplineStrokeSimpleFixup(jpp->cur->last, ci_fm);
	}
    }
    if ( flat_to ) {
	if ( clipped ) {
	    if ( !jpp->c->ratio_al ) {
		IntersectLinesSlopes(&ci_to, &p_to, &ut_to, &p_cref, &ut_cref);
		if ( !LineSameSide(p_fm, p_to, ci_to, p_ref, false) )
		    ci_to = p_to;
	    } else {
		i_dist = BPDist(i, p_to);
		ci_to = BPAdd(p_to, BPScale(BPRev(ut_to),
		                            i_dist * clip_ratio));
	    }
	    SplineSetLineTo(jpp->cur, ci_to);
	} else
	    ci_to = i;
	if ( !BPEq(ci_to, p_to) )
	    SplineSetLineTo(jpp->cur, p_to);
    } else {
	if ( clipped ) {
	    if ( !jpp->c->ratio_al )
		ci_to = SVGArcClip(p_cref, ut_cref, center_to, r_to, p_to, p_fm,
		                   p_ref);
	    else
		ci_to = ArcClip(center_to, r_to, !neg_to, p_to, i, clip_ratio);
	    SplineSetLineTo(jpp->cur, ci_to);
	} else
	    ci_to = i;
	if ( !BPEq(ci_to, p_to) ) {
	    cut_to = BP90CW(NormVec(BPSub(ci_to, center_to)));
	    SSAppendArc(jpp->cur, r_to, r_to, UTZERO, cut_to,
	                BPRevIf(!neg_to, ut_to), !neg_to, true);
	    SplineStrokeSimpleFixup(jpp->cur->last, p_to);
	}
    }
}

static int _HandleJoin(JoinParams *jpp) {
    SplineSet *cur = jpp->cur;
    BasePoint oxy = jpp->oxy;
    StrokeContext *c = jpp->c;
    int is_flat = false;
    bigreal costheta = BPDot(jpp->ut_fm, jpp->no_to->utanvec);

    assert( cur->first!=NULL );

    if ( BPWithin(cur->last->me, oxy, jpp->c->log_maxdim*INTERSPLINE_MARGIN) ) {
	// Close enough to just move the point
	SplineStrokeAppendFixup(cur->last, jpp->sxy, jpp->no_to, jpp->ccw_to,
	                        c->log_maxdim);
	is_flat = true;
    } else if ( RealWithin(costheta, 1, COS_MARGIN) ) {
	SplineSetLineTo(cur, oxy);
	is_flat = true;
    } else if ( !jpp->bend_is_ccw == !jpp->is_right ) {
	// Join is under nib, just connect for later removal
	BevelJoin(jpp);
    } else if ( c->join==lj_bevel ) {
	BevelJoin(jpp);
    } else if ( c->join==lj_round ) {
	RoundJoin(jpp, RealWithin(costheta, -1, COS_MARGIN));
    } else if (    c->join==lj_miter || c->join==lj_miterclip
                || c->join==lj_arcs ) {
	if ( RealWithin(costheta, -1, COS_MARGIN) ) {
	    if ( c->join==lj_miter )
		BevelJoin(jpp);
	    else
		DoubleBackJC(jpp->c, jpp->cur, jpp->sxy, jpp->oxy, jpp->ut_fm,
		             jpp->is_right, 0);
	} else if (c->join==lj_arcs && !RealWithin(costheta, 1, 30*COS_MARGIN))
	    ArcsJoin(jpp);
	else
	    MiterJoin(jpp);
    } else {
	if ( c->join!=lj_nib && c->join!=lj_inherited )
	    LogError( _("Warning: Unrecognized or unsupported join type, defaulting to 'nib'.") );
	NibJoin(jpp);
    }
    return is_flat;
}

static int HandleJoin(StrokeContext *c, Spline *s, SplineSet *cur,
                      BasePoint sxy, NibOffset *no_to, int ccw_to,
                      BasePoint ut_fm, int ccw_fm, int is_right) {
    JoinParams jp = { c, s, cur, sxy, BPUNINIT, ut_fm, no_to,
                      ccw_fm, ccw_to, is_right, false };
    jp.oxy = BPAdd(sxy, no_to->off[ccw_to]);
    if ( cur->first==NULL ) {
	// Create initial spline point
	cur->first = SplinePointCreate(jp.oxy.x, jp.oxy.y);
	cur->last = cur->first;
	return false;
    }
    jp.bend_is_ccw = !JointBendsCW(ut_fm, no_to->utanvec);
    return _HandleJoin(&jp);
}


static void HandleCap(StrokeContext *c, SplineSet *cur, BasePoint sxy,
                      BasePoint ut, BasePoint oxy, int is_right) {
    NibOffset no_fm, no_to;
    SplinePoint *sp;
    int corner_fm, corner_to;

    if ( c->cap==lc_bevel ) {
	SplineSetLineTo(cur, oxy);
	return;
    }
    if ( c->cap==lc_butt || c->cap==lc_round ) {
	DoubleBackJC(c, cur, sxy, oxy, BPRevIf(!is_right, ut), !is_right, 1);
    } else {
	if ( c->cap!=lc_nib && c->cap!=lc_inherited )
	    LogError( _("Warning: Unrecognized or unsupported cap type, defaulting to 'nib'.") );

	CalcNibOffset(c, ut, false, &no_fm, -1);
	corner_fm = SplineStrokeFindCorner(sxy, cur->last->me, &no_fm);
	CalcNibOffset(c, ut, true, &no_to, -1);
	corner_to = SplineStrokeFindCorner(sxy, oxy, &no_to);
	sp = AddNibPortion(c->nibcorners, cur->last, &no_fm, corner_fm, &no_to,
	                   corner_to, !is_right);
	SplineStrokeAppendFixup(sp, sxy, &no_to, corner_to, c->log_maxdim);
	cur->last = sp;
    }
}

/******************************************************************************/
/* ******************************* Main Loop ******************************** */
/******************************************************************************/

static SplineSet *OffsetSplineSet(SplineSet *ss, StrokeContext *c) {
    NibOffset no;
    Spline *s, *first=NULL;
    SplineSet *left=NULL, *right=NULL, *cur;
    SplinePoint *sp;
    BasePoint ut_ini = BPUNINIT, ut_start, ut_mid, ut_endlast;
    BasePoint sxy;
    bigreal last_t, t;
    int is_right, linear, curved, on_cusp, trash;
    int is_ccw_ini = false, is_ccw_start, is_ccw_mid, was_ccw = false;
    int closed = ss->first->prev!=NULL;

    if ( (c->contour_was_ccw ? !c->remove_inner : !c->remove_outer) || !closed )
	left = chunkalloc(sizeof(SplineSet));
    if ( (c->contour_was_ccw ? !c->remove_outer : !c->remove_inner) || !closed )
	right = chunkalloc(sizeof(SplineSet));

    for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL )
	    first = s;

	if ( SplineLength(s)==0 ) // Can ignore zero length splines
	    continue;

	ut_start = SplineUTanVecAt(s, 0.0);
	linear = SplineIsLinearish(s);
	if ( linear ) {
	    is_ccw_start = 0;
	} else {
	    is_ccw_start = SplineTurningCCWAt(s, 0.0);
	}
	if ( BPIsUninit(ut_ini) ) {
	    ut_ini = ut_start;
	    is_ccw_ini = is_ccw_start;
	}

	// Left then right
	for ( is_right=0; is_right<=1; ++is_right ) {

	    if ( is_right ) {
		if ( right==NULL )
		    continue;
		cur = right;
	    } else {
		if ( left==NULL )
		    continue;
		cur = left;
	    }

	    sxy = SPLINEPVAL(s, 0.0);
	    CalcNibOffset(c, ut_start, is_right, &no, -1);

	    HandleJoin(c, s, cur, sxy, &no, is_ccw_start,
	               ut_endlast, was_ccw, is_right);

	    on_cusp = OffsetOnCuspAt(c, s, 0.0, &no, is_right, is_ccw_start);

	    // The path for this spline
	    if ( linear ) {
		sxy = SPLINEPVAL(s, 1.0);
		SplineSetLineTo(cur, BPAdd(sxy, no.off[is_ccw_start]));
	    } else {
		t = 0.0;
		ut_mid = ut_start;
		is_ccw_mid = is_ccw_start;
		while ( t < 1.0 ) {
		    last_t = t;
		    t = SplineStrokeNextT(c, s, t, is_ccw_mid, &ut_mid, &curved,
		                          is_right, no.nci[is_ccw_mid]);
		    assert( t > last_t && t <= 1.0 );

		    if ( curved )
			sp = TraceAndFitSpline(c, s, last_t, t, cur->last,
			                       no.nci[is_ccw_mid], is_right,
			                       on_cusp);
		    else
			sp = AppendCubicSplinePortion(s, last_t, t, cur->last);

		    cur->last = sp;

		    sxy = SPLINEPVAL(s, t);
		    CalcNibOffset(c, ut_mid, is_right, &no, no.nci[is_ccw_mid]);
		    is_ccw_mid = SplineTurningCCWAt(s, t);
		    SplineStrokeAppendFixup(cur->last, sxy, &no, -1,
		                            c->log_maxdim);

		    if ( t < 1.0 )
			HandleFlat(cur, sxy, &no, is_ccw_mid);
		    on_cusp = OffsetOnCuspAt(c, s, t, &no, is_right, is_ccw_mid);
		}
	    }
	}
	ut_endlast = SplineUTanVecAt(s, 1.0);
        was_ccw = SplineTurningCCWAt(s, 1.0);
    }
    if (    (left!=NULL && left->first==NULL)
         || (right!=NULL && right->first==NULL) ) {
	// The path (presumably) had only zero-length splines
	LogError( _("Warning: No stroke output for contour") );
	assert(    (left==NULL || left->first==NULL)
	        && (right==NULL || right->first==NULL) );
	chunkfree(left, sizeof(SplineSet));
	chunkfree(right, sizeof(SplineSet));
	return NULL;
    }
    cur = NULL;
    if ( !closed ) {
	HandleCap(c, left, ss->last->me, ut_endlast, right->last->me, true);
	SplineSetReverse(right);
	left->next = right;
	right = NULL;
	SplineSetJoin(left, true, FIXUP_MARGIN*c->log_maxdim, &closed, false);
	if ( !closed )
	     LogError( _("Warning: Contour end did not close") );
	else {
	    HandleCap(c, left, ss->first->me, ut_ini, left->first->me, false);
	    SplineSetJoin(left, true, FIXUP_MARGIN*c->log_maxdim, &closed, true);
	    if ( !closed )
		LogError( _("Warning: Contour start did not close") );
	    else {
		if ( c->rmov==srmov_contour ) {
		    left = SplineSetRemoveOverlap(NULL,left,over_remove);
		    SplineSetsCorrect(left, &trash);
		} else {
		    is_ccw_mid = SplinePointListIsClockwise(left);
		    if ( is_ccw_mid==0 && left->next==NULL ) {
			SplineSetReverse(left);
			is_ccw_mid=1;
		    }
		    if ( is_ccw_mid==-1 && c->rmov!=srmov_none ) {
			left = SplineSetRemoveOverlap(NULL,left,over_remove);
			SplineSetsCorrect(left, &trash);
			is_ccw_mid = SplinePointListIsClockwise(left);
		    }
		    if ( is_ccw_mid==-1 ) {
			LogError( _("Warning: Can't identify contour"
			            " direction, assuming clockwise\n") );
		    }
		}
	    }
	}
	cur = left;
	left = NULL;
    } else {
	// This can fail if the source contour is closed in a strange way
	if ( left!=NULL ) {
	    CalcNibOffset(c, ut_ini, false, &no, -1);
	    HandleJoin(c, ss->first->next, left, ss->first->me, &no,
	               is_ccw_ini, ut_endlast, was_ccw, false);
            left = SplineSetJoin(left, true, FIXUP_MARGIN*c->log_maxdim,
	                         &closed, true);
	    if ( !closed )
		LogError( _("Warning: Left contour did not close") );

	    cur = left;
	}
	if ( right!=NULL ) {
	    CalcNibOffset(c, ut_ini, true, &no, -1);
	    HandleJoin(c, ss->first->next, right, ss->first->me, &no,
	               is_ccw_ini, ut_endlast, was_ccw, true);
            right = SplineSetJoin(right, true, FIXUP_MARGIN*c->log_maxdim,
	                          &closed, true);
	    if ( !closed )
		LogError( _("Warning: Right contour did not close") );
	    else {
		SplineSetReverse(right);
	    }
	    if ( cur != NULL ) {
		cur->next = right;
	    } else
		cur = right;
	    right = NULL;
	}
	if ( c->rmov==srmov_contour ) {
	    if ( left!=NULL ) {
		cur = SplineSetRemoveOverlap(NULL,cur,over_remove);
		SplineSetsCorrect(cur, &trash);
		left = NULL;
	    } else
		cur = SplineContourOuterCCWRemoveOverlap(cur);
	}
    }
    return cur;
}

/******************************************************************************/
/* ******************************* Unit Stuff ******************************* */
/******************************************************************************/

static BasePoint SquareCorners[] = {
    { -1,  1 },
    {  1,  1 },
    {  1, -1 },
    { -1, -1 },
};

static struct shapedescrip {
    BasePoint me, prevcp, nextcp;
}
unitcircle[] = {
    { { -1, 0 }, { -1, -CIRCOFF }, { -1, CIRCOFF } },
    { { 0 , 1 }, { -CIRCOFF, 1 }, { CIRCOFF, 1 } },
    { { 1, 0 }, { 1, CIRCOFF }, { 1, -CIRCOFF } },
    { { 0, -1 }, { CIRCOFF, -1 }, { -CIRCOFF, -1 } },
    { { 0, 0 }, { 0, 0 }, { 0, 0 } }
};

static SplinePoint *SpOnCircle(int i,bigreal radius,BasePoint *center) {
    SplinePoint *sp = SplinePointCreate(unitcircle[i].me.x*radius + center->x,
					unitcircle[i].me.y*radius + center->y);
    sp->pointtype = pt_curve;
    sp->prevcp.x = unitcircle[i].prevcp.x*radius + center->x;
    sp->prevcp.y = unitcircle[i].prevcp.y*radius + center->y;
    sp->nextcp.x = unitcircle[i].nextcp.x*radius + center->x;
    sp->nextcp.y = unitcircle[i].nextcp.y*radius + center->y;
return( sp );
}

SplineSet *UnitShape(int n) {
    SplineSet *ret;
    SplinePoint *sp1, *sp2;
    int i;
    BasePoint origin;

    ret = chunkalloc(sizeof(SplineSet));
    if ( n>=3 || n<=-3 ) {
	/* Regular n-gon with n sides */
	/* Inscribed in a unit circle, if n<0 then circumscribed around */
	bigreal angle = 2*FF_PI/(2*n);
	bigreal factor=1;
	if ( n<0 ) {
	    angle = -angle;
	    n = -n;
	    factor = 1/cos(angle);
	}
	angle -= FF_PI/2;
	ret->first = sp1 = SplinePointCreate(factor*cos(angle), factor*sin(angle));
	sp1->pointtype = pt_corner;
	for ( i=1; i<n; ++i ) {
	    angle = 2*FF_PI/(2*n) + i*2*FF_PI/n - FF_PI/2;
	    sp2 = SplinePointCreate(factor*cos(angle),factor*sin(angle));
	    sp2->pointtype = pt_corner;
	    SplineMake3(sp1,sp2);
	    sp1 = sp2;
	}
	SplineMake3(sp1,ret->first);
	ret->last = ret->first;
	SplineSetReverse(ret);		/* Drat, just drew it counter-clockwise */
    } else if ( n ) {
	ret->first = sp1 = SplinePointCreate(SquareCorners[0].x,
			    SquareCorners[0].y);
	sp1->pointtype = pt_corner;
	for ( i=1; i<4; ++i ) {
	    sp2 = SplinePointCreate(SquareCorners[i].x,
				SquareCorners[i].y);
	    sp2->pointtype = pt_corner;
	    SplineMake3(sp1,sp2);
	    sp1 = sp2;
	}
	SplineMake3(sp1,ret->first);
	ret->last = ret->first;
    } else {
	/* Turn into a circle */
	origin.x = origin.y = 0;
	ret->first = sp1 = SpOnCircle(0,1,&origin);
	for ( i=1; i<4; ++i ) {
	    sp2 = SpOnCircle(i,1,&origin);
	    SplineMake3(sp1,sp2);
	    sp1 = sp2;
	}
	SplineMake3(sp1,ret->first);
	ret->last = ret->first;
    }
return( ret );
}

/******************************************************************************/
/* ******************************* Higher-Level ***************************** */
/******************************************************************************/

static SplinePointList *SinglePointStroke(SplinePoint *sp,
                                          struct strokecontext *c) {
    SplineSet *ret;
    real trans[6];

    if ( c->nibtype==nib_ellip && c->cap==lc_butt ) {
	// Leave as a single point
	ret = chunkalloc(sizeof(SplineSet));
	ret->first = ret->last = SplinePointCreate(sp->me.x,sp->me.y);
	ret->first->pointtype = pt_corner;
    } else {
	 // Draw a nib
	memset(&trans, 0, sizeof(trans));
	trans[0] = trans[3] = 1;
	trans[4] = sp->me.x;
	trans[5] = sp->me.y;
	ret = SplinePointListCopy(c->nib);
	SplinePointListTransformExtended(ret, trans, tpt_AllPoints,
	                                 tpmask_dontTrimValues);
    }
    return( ret );
}

static SplineSet *SplineSet_Stroke(SplineSet *ss,struct strokecontext *c,
                                   int order2) {
    SplineSet *base = ss;
    SplineSet *ret;
    int copied = 0;

    c->contour_was_ccw = (   base->first->prev!=NULL
                          && !SplinePointListIsClockwise(base) );

    if ( base->first->next==NULL )
	ret = SinglePointStroke(base->first, c);
    else {
	if ( base->first->next->order2 ) {
	    base = SSPSApprox(ss);
	    copied = 1;
	}
	if ( c->contour_was_ccw )
	    SplineSetReverse(base);
	ret = OffsetSplineSet(base, c);
    }
    if ( order2 )
	ret = SplineSetsConvertOrder(ret,order2);
    if ( copied )
	SplinePointListFree(base);
    else if ( c->contour_was_ccw )
	SplineSetReverse(base);

    return(ret);
}

static SplineSet *SplineSets_Stroke(SplineSet *ss,struct strokecontext *c,
                                    int order2) {
    SplineSet *first=NULL, *last=NULL, *cur;

    while ( ss!=NULL ) {
	cur = SplineSet_Stroke(ss,c,order2);
	if ( first==NULL )
	    first = last = cur;
	else
	    last->next = cur;
	if ( last!=NULL )
	    while ( last->next!=NULL )
		last = last->next;
	ss = ss->next;
    }
return( first );
}

SplineSet *SplineSetStroke(SplineSet *ss,StrokeInfo *si, int order2) {
    int max_pc;
    StrokeContext c;
    SplineSet *nibs, *nib, *first, *last, *cur;
    bigreal sn = 0.0, co = 1.0, mr, wh_ratio, maxdim;
    DBounds b;
    real trans[6];
    struct simplifyinfo smpl = { sf_forcelines | sf_mergelines |
                                 sf_smoothcurves | sf_setstart2extremum,
                                 0.25, 0.1, .005, 0, 0, 0 };

    if ( si->stroke_type==si_centerline )
	IError("centerline not handled");

    memset(&c,0,sizeof(c));
    c.join = si->join;
    c.cap  = si->cap;
    c.joinlimit = si->joinlimit;
    c.extendcap = si->extendcap;
    c.acctarget = si->accuracy_target;
    c.remove_inner = si->removeinternal;
    c.remove_outer = si->removeexternal;
    c.leave_users_center = si->leave_users_center;
    c.extrema = si->extrema;
    c.simplify = si->simplify;
    c.ecrelative = si->ecrelative;
    c.jlrelative = si->jlrelative;
    c.rmov = si->rmov;
    if ( si->height!=0 )
	mr = si->height/2;
    else
	mr = si->width/2;
    if ( si->al==sal_auto ) {
	if ( si->stroke_type==si_round || si->stroke_type==si_calligraphic ) {
	    wh_ratio = si->height/si->width;
	    c.ratio_al = (   c.joinlimit < 4.0 && c.jlrelative
	                  && (wh_ratio >= 2.0 || wh_ratio <= 0.5));
	} else {
	    c.ratio_al = (c.joinlimit < 4.0 && c.jlrelative);
	}
    } else
	c.ratio_al = ( si->al==sal_ratio );

    if ( !c.extrema )
	smpl.flags |= sf_ignoreextremum;

    if ( c.acctarget<MIN_ACCURACY ) {
         LogError( _("Warning: Accuracy target %lf less than minimum %lf, "
	             "using %lf instead\n"), c.acctarget, MIN_ACCURACY );
	 c.acctarget = MIN_ACCURACY;
    }
    smpl.err = c.acctarget;

    if ( si->penangle!=0 ) {
	sn = sin(si->penangle);
	co = cos(si->penangle);
    }
    trans[0] = co;
    trans[1] = sn;
    trans[2] = -sn;
    trans[3] = co;
    trans[4] = trans[5] = 0;

    if ( si->stroke_type==si_round || si->stroke_type==si_calligraphic ) {
	c.nibtype = si->stroke_type==si_round ? nib_ellip : nib_rect;
	max_pc = 4;
	nibs = UnitShape(si->stroke_type==si_round ? 0 : -4);
	trans[0] *= si->width/2;
	trans[1] *= si->width/2;
	trans[2] *= mr;
	trans[3] *= mr;
	maxdim = fmax(si->width/2, mr);
    } else {
	c.nibtype = nib_convex;
	max_pc = 20; // a guess, will be reallocated if needed
	nibs = SplinePointListCopy(si->nib);
	SplineSetFindBounds(nibs,&b);
	if ( !c.leave_users_center ) {
	    trans[4] = -(b.minx+b.maxx)/2;
	    trans[5] = -(b.miny+b.maxy)/2;
	} else {
	    c.pseudo_origin.x = (b.minx+b.maxx)/2;
	    c.pseudo_origin.y = (b.miny+b.maxy)/2;
	}
	// Close enough
	maxdim = sqrt(pow(b.maxx-b.minx, 2) + pow(b.maxy-b.miny, 2))/2.0;
    }
    // Increases at the rate of the natural log, but with the "1 point" set
    // to radius 10
    c.log_maxdim = fmax(1.0, log(fmax(maxdim, mr))-LOG_MAXDIM_ADJ);

    SplinePointListTransformExtended(nibs,trans,tpt_AllPoints,
	                             tpmask_dontTrimValues);

    first = last = NULL;
    for ( nib=nibs; nib!=NULL; nib=nib->next ) {
	assert( NibIsValid(nib)==Shape_Convex );
	c.nib = nib;
	BuildNibCorners(&c.nibcorners, nib, &max_pc, &c.n);
	cur = SplineSets_Stroke(ss,&c,order2);
        if ( first==NULL )
	    first = last = cur;
	else
	    last->next = cur;
	if ( last!=NULL )
	    while ( last->next!=NULL )
	        last = last->next;
    }
    if ( c.rmov==srmov_layer )
	first=SplineSetRemoveOverlap(NULL,first,over_remove);
    if ( c.extrema )
	SplineCharAddExtrema(NULL,first,ae_all,1000);
    if ( c.simplify )
	first = SplineCharSimplify(NULL,first,&smpl);
    else
	SPLCategorizePoints(first);

    free(c.nibcorners);
    SplinePointListFree(nibs);
    nibs = NULL;

    return( first );
}

void FVStrokeItScript(void *_fv, StrokeInfo *si,
                      int UNUSED(pointless_argument)) {
    FontViewBase *fv = _fv;
    int layer = fv->active_layer;
    SplineSet *temp;
    int i, cnt=0, gid;
    SplineChar *sc;

    for ( i=0; i<fv->map->enccount; ++i ) if ( (gid=fv->map->map[i])!=-1 && fv->sf->glyphs[gid]!=NULL && fv->selected[i] )
	++cnt;
    ff_progress_start_indicator(10,_("Stroking..."),_("Stroking..."),0,cnt,1);

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i ) {
	if ( (gid=fv->map->map[i])!=-1 && (sc = fv->sf->glyphs[gid])!=NULL &&
		!sc->ticked && fv->selected[i] ) {
	    sc->ticked = true;
	    if ( sc->parent->multilayer ) {
		SCPreserveState(sc,false);
		for ( layer = ly_fore; layer<sc->layer_cnt; ++layer ) {
		    temp = SplineSetStroke(sc->layers[layer].splines,si,sc->layers[layer].order2);
		    SplinePointListsFree( sc->layers[layer].splines );
		    sc->layers[layer].splines = temp;
		}
		SCCharChangedUpdate(sc,ly_all);
	    } else {
		SCPreserveLayer(sc,layer,false);
		temp = SplineSetStroke(sc->layers[layer].splines,si,sc->layers[layer].order2);
		SplinePointListsFree( sc->layers[layer].splines );
		sc->layers[layer].splines = temp;
		SCCharChangedUpdate(sc,layer);
	    }
	    if ( !ff_progress_next())
    break;
	}
    }
    ff_progress_end_indicator();
}
