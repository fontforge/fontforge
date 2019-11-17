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

#define BP_LENGTHSQ(v) (pow((v).x,2)+pow((v).y,2))
#define BP_REV(v) (BasePoint) { -(v).x, -(v).y }
#define BP_REV_IF(t, v) ((t) ? (BasePoint) { -(v).x, -(v).y } : (v))
#define BP_ADD(v1, v2) (BasePoint) { (v1).x + (v2).x, (v1).y + (v2).y } 
#define BP_SCALE(v, f) (BasePoint) { (f)*(v).x, (f)*(v).y } 
#define BP_AVG(v1, v2) (BasePoint) { ((v1).x + (v2).x)/2, ((v1).y + (v2).y)/2 }
#define BP_DOT(v1, v2) ( ((v1).x * (v2).x) + ((v1).y * (v2).y) )
#define BP_UNINIT ((BasePoint) { -INFINITY, INFINITY })
#define BP_IS_UNINIT(v) ((v).x==-INFINITY && (v).y==INFINITY)
#define BP_ROT(v, ut) (BasePoint) { (ut).x * (v).x - (ut).y * (v).y, (ut).y * (v).x + (ut).x * (v).y }
#define UTZERO ((BasePoint) { 0.0, 1.0 })
#define UTMIN ((BasePoint) { -1, -DBL_MIN })
#define UT_90CCW(v) (BasePoint) { -(v).y, (v).x }
#define UT_90CW(v) (BasePoint) { (v).y, -(v).x }
#define UT_NEG(v) (BasePoint) { (v).x, -(v).y }

extern BasePoint MakeUTanVec(bigreal x, bigreal y);
extern BasePoint NormVec(BasePoint v);
extern int UTanVecGreater(BasePoint uta, BasePoint utb);
extern int UTanVecsSequent(BasePoint ut1, BasePoint ut2, BasePoint ut3,
                           int ccw);
extern int JointBendsCW(BasePoint ut_ref, BasePoint ut_vec);
extern BasePoint SplineUTanVecAt(Spline *s, bigreal t);
extern bigreal SplineSolveForUTanVec(Spline *spl, BasePoint ut, bigreal min_t);
extern void UTanVecTests();

#endif // FONTFORGE_UTANVEC_H
