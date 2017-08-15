/* Copyright (C) 2003-2012 by George Williams */
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

#ifndef FONTFORGE_MM_H
#define FONTFORGE_MM_H

#include "baseviews.h"
#include "splinefont.h"

extern void MMWeightsUnMap(real weights[MmMax], real axiscoords[4],
	int axis_count);
extern bigreal MMAxisUnmap(MMSet *mm,int axis,bigreal ncv);
extern SplineFont *_MMNewFont(MMSet *mm,int index,char *familyname,real *normalized);
extern SplineFont *MMNewFont(MMSet *mm,int index,char *familyname);

extern char *MMBlendChar(MMSet *mm, int gid);
extern char *MMExtractArrayNth(char *pt, int ipos);
extern char *MMExtractNth(char *pt, int ipos);
extern char *MMGuessWeight(MMSet *mm, int ipos, char *def);
extern char *MMMakeMasterFontname(MMSet *mm, int ipos, char **fullname);
extern const char *MMAxisAbrev(char *axis_name);
extern FontViewBase *MMCreateBlendedFont(MMSet *mm, FontViewBase *fv, real blends[MmMax], int tonew);
extern int MMReblend(FontViewBase *fv, MMSet *mm);
extern int MMValid(MMSet *mm, int complain);
extern void MMKern(SplineFont *sf, SplineChar *first, SplineChar *second, int diff, struct lookup_subtable *sub, KernPair *oldkp);

#endif /* FONTFORGE_MM_H */
