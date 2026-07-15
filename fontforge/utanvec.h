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

#ifndef FONTFORGE_UTANVEC_H
#define FONTFORGE_UTANVEC_H

#include <fontforge-config.h>

#include "splinefont.h"
#include "splineutil2.h"

#include <math.h>

#ifdef __cplusplus
extern "C" {
/* C++ brace initialization */
#define BP_INIT(x, y) BasePoint{x, y}
#else
/* C99 compound literal - works on GCC/Clang but not MSVC in C mode */
#define BP_INIT(x, y) ((BasePoint){x, y})
#endif

#define BPUNINIT BP_INIT(-INFINITY, INFINITY)
#define UTZERO BP_INIT(0.0, 1.0)
#define UTMIN BP_INIT(-1, -DBL_MIN)
#define UTMARGIN (1e-7)     // Arrived at through testing

static inline int BPWithin(BasePoint bp1, BasePoint bp2, bigreal f) {
    return RealWithin(bp1.x, bp2.x, f) && RealWithin(bp1.y, bp2.y, f);
}

static inline bigreal BPLenSq(BasePoint v) {
    return v.x * v.x + v.y * v.y;
}

static inline bigreal BPNorm(BasePoint v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

static inline bigreal BPDist(BasePoint p1, BasePoint p2) {
    bigreal dx = p2.x - p1.x, dy = p2.y - p1.y;
    return sqrt(dx * dx + dy * dy);
}

static inline BasePoint BPRev(BasePoint v) {
    return BP_INIT(-v.x, -v.y);
}

static inline BasePoint BPRevIf(int t, BasePoint v) {
    return t ? BP_INIT(-v.x, -v.y) : v;
}

static inline BasePoint BPAdd(BasePoint v1, BasePoint v2) {
    return BP_INIT(v1.x + v2.x, v1.y + v2.y);
}

static inline BasePoint BPSub(BasePoint v1, BasePoint v2) {
    return BP_INIT(v1.x - v2.x, v1.y - v2.y);
}

static inline BasePoint BPScale(BasePoint v, bigreal f) {
    return BP_INIT(f * v.x, f * v.y);
}

static inline BasePoint BPAvg(BasePoint v1, BasePoint v2) {
    return BP_INIT((v1.x + v2.x) / 2, (v1.y + v2.y)/2);
}

static inline bigreal BPDot(BasePoint v1, BasePoint v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

static inline bigreal BPCross(BasePoint v1, BasePoint v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

static inline int BPIsUninit(BasePoint bp) {
    return bp.x == -INFINITY && bp.y == INFINITY;
}

static inline BasePoint BPRot(BasePoint v, BasePoint ut) {
    return BP_INIT(ut.x * v.x - ut.y * v.y, ut.y * v.x + ut.x * v.y);
}

static inline int BPEq(BasePoint bp1, BasePoint bp2) {
    return bp1.x == bp2.x && bp1.y == bp2.y;
}

static inline BasePoint BP90CCW(BasePoint v) {
    return BP_INIT(-v.y, v.x);
}

static inline BasePoint BP90CW(BasePoint v) {
    return BP_INIT(v.y, -v.x);
}

static inline BasePoint BPNeg(BasePoint v) {
    return BP_INIT(v.x, -v.y);
}

// Not called "BPNear" because this is specific to UTanVecs
static inline int UTNear(BasePoint bp1, BasePoint bp2) {
    return BPWithin(bp1, bp2, UTMARGIN);
}

extern BasePoint MakeUTanVec(bigreal x, bigreal y);
extern BasePoint NormVec(BasePoint v);
extern int UTanVecGreater(BasePoint uta, BasePoint utb);
extern int UTanVecsSequent(BasePoint ut1, BasePoint ut2, BasePoint ut3,
                           int ccw);
extern int JointBendsCW(BasePoint ut_ref, BasePoint ut_vec);
extern BasePoint SplineUTanVecAt(Spline *s, bigreal t);
extern bigreal SplineSolveForUTanVec(Spline *spl, BasePoint ut, bigreal min_t,
                                     bool picky);
extern void UTanVecTests();

#ifdef __cplusplus
}
#endif

#endif // FONTFORGE_UTANVEC_H
