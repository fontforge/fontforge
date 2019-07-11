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

#ifndef FONTFORGE_SPLINEFIT_H
#define FONTFORGE_SPLINEFIT_H

#include <fontforge-config.h>

#include "splinefont.h"

typedef struct fitpoint {
    BasePoint p;
    BasePoint ut;
    bigreal t;
} FitPoint;

#define FITPOINT_EMPTY { {0.0, 0.0}, {0.0, 0.0}, 0.0 }

typedef int (*GenPointsP)(void *vinfo, bigreal t_start, bigreal t_end, FitPoint **fpp);

extern Spline *ApproximateSplineFromPoints(SplinePoint *from, SplinePoint *to,
                                           FitPoint *mid, int cnt, int order2);
extern Spline *ApproximateSplineFromPointsSlopes(SplinePoint *from, SplinePoint *to,
                                                 FitPoint *mid, int cnt, int order2);
extern Spline *ApproximateSplineFromPointsSlopes(SplinePoint *from, SplinePoint *to,
                                                 FitPoint *mid, int cnt, int order2);
extern SplinePoint *ApproximateSplineSetFromGen(SplinePoint *from, SplinePoint *to,
                                                bigreal start_t, bigreal end_t, 
                                                bigreal toler, int toler_is_sumsq,
                                                GenPointsP genp, void *tok, int order2);

#endif /* FONTFORGE_SPLINEFIT_H */
