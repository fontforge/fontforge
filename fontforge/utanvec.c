/* Copyright (C) 2019 by Skef Iterum */
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

#include "utanvec.h"
#include "splineutil.h"
#include "splineutil2.h"

#include <math.h>
#include <assert.h>

#define UTMARGIN (1e-7)     // Arrived at through testing
#define UTSOFTMARGIN (1e-5) // Fallback margin for some cases
#define BPNEAR(bp1, bp2) BPWithin(bp1, bp2, UTMARGIN)
#define UTRETRY (1e-9)      // Amount of "t" to walk back from the 
                            // ends of degenerate splines to find
                            // a slope.

BasePoint MakeUTanVec(bigreal x, bigreal y) {
    BasePoint ret = { 0.0, 0.0 };
    bigreal len = x*x + y*y;
    if ( len!=0 ) {
	len = sqrt(len);
	ret.x = x/len;
	ret.y = y/len;
    }
    return ret;
}

BasePoint NormVec(BasePoint v) {
    return MakeUTanVec(v.x, v.y);
}

/* Orders unit tangent vectors on an absolute -PI+e to PI
 * equivalent basis
 */
int UTanVecGreater(BasePoint uta, BasePoint utb) {
    assert(    RealNear(BPLenSq(uta),1)
            && RealNear(BPLenSq(utb),1) );

    if (uta.y >= 0) {
	if (utb.y < 0)
	    return true;
	return uta.x < utb.x && !BPNEAR(uta, utb);
    }
    if (utb.y >= 0)
	return false;
    return uta.x > utb.x && !BPNEAR(uta, utb);
}

/* True if rotating from ut1 to ut3 in the specified direction
 * the angle passes through ut2.
 *
 * Note: Returns true if ut1 is near ut2 but false if ut2 is
 * near ut3
 */
int UTanVecsSequent(BasePoint ut1, BasePoint ut2, BasePoint ut3,
                           int ccw) {
    BasePoint tmp;

    if ( BPNEAR(ut1, ut2) )
	return true;

    if ( BPNEAR(ut2, ut3) || BPNEAR(ut1, ut3) )
	return false;

    if (ccw) {
	tmp = ut1;
	ut1 = ut3;
	ut3 = tmp;
    }

    if ( UTanVecGreater(ut1, ut3) ) {
	return UTanVecGreater(ut1, ut2) && UTanVecGreater(ut2, ut3);
    } else {
	return    (UTanVecGreater(ut1, ut2) && UTanVecGreater(ut3, ut2))
	       || (UTanVecGreater(ut2, ut1) && UTanVecGreater(ut2, ut3));
    }
}

int JointBendsCW(BasePoint ut_ref, BasePoint ut_vec) {
    // magnitude of cross product
    bigreal r = ut_ref.x*ut_vec.y - ut_ref.y*ut_vec.x;
    if ( RealWithin(r, 0.0, UTMARGIN) )
	return false;
    return r > 0;
}

BasePoint SplineUTanVecAt(Spline *s, bigreal t) {
    BasePoint raw;

    if ( SplineIsLinearish(s) ) {
	raw.x = s->to->me.x - s->from->me.x;
	raw.y = s->to->me.y - s->from->me.y;
    } else {
	// If one control point is colocated with its on-curve point the 
	// slope will be undefined at that end, so walk back a bit for
	// consistency
	if (   RealWithin(t, 0, UTRETRY)
	    && BPWithin(s->from->me, s->from->nextcp, 1e-13) )
	    t = UTRETRY;
	else if (   RealWithin(t, 1, UTRETRY)
	    && BPWithin(s->to->me, s->to->prevcp, 1e-13) )
	    t = 1-UTRETRY;
	raw = SPLINEPTANVAL(s, t);
	// For missing slopes take a very small step away and try again.
	if ( raw.x==0 && raw.y==0 )
	    raw = SPLINEPTANVAL(s, (t+UTRETRY <= 1.0) ? t+UTRETRY : t-UTRETRY);
    }
    return MakeUTanVec(raw.x, raw.y);
}

/* Return the lowest t value greater than min_t such that the
 * tangent at the point is parallel to ut, or -1 if the tangent
 * never has that slope.
 */
#define ROTY(p, ut) (-(ut).y*(p).x + (ut).x*(p).y)
#define ZCHECK(v) (RealWithin((v),0,1e-10) ? 0 : (v))

bigreal SplineSolveForUTanVec(Spline *spl, BasePoint ut, bigreal min_t,
                              bool picky) {
    bigreal tmp, yto;
    extended te1, te2;
    Spline1D ys1d;

    // Nothing to "solve" for; lines should be handled by different means
    // because you can't "advance" along them based on slope changes
    if ( SplineIsLinear(spl) )
        return -1;

    // Only check t==0 if min_t is negative
    if ( min_t<0 && BPNEAR(ut, SplineUTanVecAt(spl, 0.0)) )
       return 0.0;

    // This is probably over-optimized
    ys1d.d = ROTY(spl->from->me, ut);
    yto = ROTY(spl->to->me, ut);
    if ( spl->order2 ) {
        ys1d.c = ZCHECK(2*(ROTY(spl->from->nextcp, ut)-ys1d.d));
        ys1d.b = ZCHECK(yto-ys1d.d-ys1d.c);
        ys1d.a = 0;
    } else {
	tmp = ROTY(spl->from->nextcp, ut);
	ys1d.c = ZCHECK(3*(tmp-ys1d.d));
	ys1d.b = ZCHECK(3*(ROTY(spl->to->prevcp, ut)-tmp)-ys1d.c);
	ys1d.a = ZCHECK(yto-ys1d.d-ys1d.c-ys1d.b);
    }
    if ( ys1d.a!=0 && (   Within16RoundingErrors(ys1d.a+ys1d.d,ys1d.d)
                       || Within16RoundingErrors(ys1d.a+yto,yto)))
	ys1d.a = 0;

    // After rotating by theta the desired angle will be at a y extrema
    SplineFindExtrema(&ys1d, &te1, &te2);

    if (   te1 != -1 && te1 > (min_t+1e-9)
        && BPNEAR(ut, SplineUTanVecAt(spl, te1)) )
	return te1;
    if (   te2 != -1 && te2 > (min_t+1e-9)
        && BPNEAR(ut, SplineUTanVecAt(spl, te2)) )
	return te2;

    // Check t==1
    if ( (min_t+UTRETRY) < 1 && BPNEAR(ut, SplineUTanVecAt(spl, 1.0)) )
        return 1.0;

    if ( picky )
	return -1.0;

    if (   te1 != -1 && te1 > (min_t+1e-9)
        && BPWithin(ut, SplineUTanVecAt(spl, te1), UTSOFTMARGIN) )
	return te1;
    if (   te2 != -1 && te2 > (min_t+1e-9)
        && BPWithin(ut, SplineUTanVecAt(spl, te2), UTSOFTMARGIN) )
	return te2;

    // Check t==1
    if (    (min_t+UTRETRY) < 1
        && BPWithin(ut, SplineUTanVecAt(spl, 1.0), UTSOFTMARGIN) )
        return 1.0;

    // Nothing found
    return -1.0;
}

#if 0
void UTanVecTests() {
    int i, j, k, a, b;
    bigreal r, x, y, z;
    BasePoint ut[361], utmax = { -1, 0 }, utmin = UTMIN;

    for (i = 0; i<=360; i++) {
	r = ((180-i) * FF_PI / 180);
	ut[i] = MakeUTanVec(cos(r), sin(r));
	// printf("i:%d, i.x:%lf, i.y:%lf\n", i, ut[i].x, ut[i].y);
    }

    for (i=0; i<360; ++i) {
	for (j = 0; j<360; ++j) {
	    if ( i==j )
		continue;
	    if ( i<j )
		assert(    UTanVecGreater(ut[i], ut[j])
		       && !UTanVecGreater(ut[j], ut[i]));
	    else
		assert(    UTanVecGreater(ut[j], ut[i])
		       	&& !UTanVecGreater(ut[i], ut[j]));
	    assert(!BPNEAR(ut[i], ut[j]));
	    x = atan2(ut[i].x, ut[i].y);
	    y = atan2(ut[j].x, ut[j].y);
	    z = x-y;
	    z = atan2(sin(z), cos(z));
	    if ( RealNear(z, FF_PI) || RealNear(z, -FF_PI) )
		continue;
	    assert( (z>0) == JointBendsCW(ut[i], ut[j]) );
	}
	assert( i==0 || UTanVecGreater(utmax, ut[i]) );
	assert( UTanVecGreater(ut[i], utmin) );
    }

    for (i=0; i<360; ++i)
	for (j = 0; j<360; ++j)
	    for (k = 0; k<360; ++k) {
		a = UTanVecsSequent(ut[i], ut[j], ut[k], false);
		b = UTanVecsSequent(ut[i], ut[j], ut[k], true);
		if ( i==j )
		    assert(a && b);
		else if ( j==k || i==k )
		    assert(!a && !b);
		else {
		    x = atan2(ut[i].x, ut[i].y);
		    y = atan2(ut[j].x, ut[j].y);
		    z = atan2(ut[k].x, ut[k].y);
		    y = fmod(4*FF_PI+y-x,2*FF_PI);
		    z = fmod(4*FF_PI+z-x,2*FF_PI);
		    if ( y<z )
			assert( a && !b );
		    else
			assert( !a && b );
		}
	    }
}
#endif
