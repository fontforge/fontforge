/* Copyright (C) 2007-2012 by George Williams */
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

#ifndef FONTFORGE_SEARCH_H
#define FONTFORGE_SEARCH_H

#include "baseviews.h"
#include "splinefont.h"

typedef struct searchdata {
	SplineChar sc_srch, sc_rpl;
	SplineSet *path, *revpath, *replacepath, *revreplace;
	int pointcnt, rpointcnt;
	real fudge;
	real fudge_percent;                  /* a value of .05 here represents 5% (we don't store the integer) */
	unsigned int tryreverse: 1;
	unsigned int tryflips: 1;
	unsigned int tryrotate: 1;
	unsigned int tryscale: 1;
	unsigned int endpoints: 1;           /* Don't match endpoints, use them for direction only */
	unsigned int onlyselected: 1;
	unsigned int subpatternsearch: 1;
	unsigned int doreplace: 1;
	unsigned int replaceall: 1;
	unsigned int findall: 1;
	unsigned int searchback: 1;
	unsigned int wrap: 1;
	unsigned int wasreversed: 1;
	unsigned int replacewithref: 1;
	unsigned int already_complained: 1;  /* User has already been alerted to the fact that we've converted splines to refs and lost the instructions */
	SplineSet *matched_spl;
	SplinePoint *matched_sp, *last_sp;
	real matched_rot, matched_scale;
	real matched_x, matched_y;
	double matched_co, matched_si;       /* Precomputed sin, cos */
	enum flipset matched_flip;
	unsigned long long matched_refs;     /* Bit map of which refs in the char were matched */
	unsigned long long matched_ss;       /* Bit map of which splines in the char were matched in multi-path mode */
	unsigned long long matched_ss_start; /* Bit map of which splines we tried to start matches with */
	FontViewBase *fv;
	SplineChar *curchar;
	int last_gid;
} SearchData;

extern void SDDestroy(SearchData *sd);
extern SearchData *SDFillup(SearchData *sd, FontViewBase *fv);
extern int SearchChar(SearchData *sv, int gid,int startafter);
extern int _DoFindAll(SearchData *sv);
extern int DoRpl(SearchData *sv);
extern void SVResetPaths(SearchData *sv);

extern int FVReplaceAll(FontViewBase *fv, SplineSet *find, SplineSet *rpl, double fudge, int flags);
extern SearchData *SDFromContour(FontViewBase *fv, SplineSet *find, double fudge, int flags);
extern SplineChar *SDFindNext(SearchData *sd);
extern void FVBReplaceOutlineWithReference(FontViewBase *fv, double fudge);
extern void FVCorrectReferences(FontViewBase *fv);
extern void SCSplinePointsUntick(SplineChar *sc, int layer);

#endif /* FONTFORGE_SEARCH_H */
