/* Copyright (C) 2005 by George Williams */
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
#ifndef _STEMDB_H_
# define _STEMDB_H_

# include "pfaedit.h"

struct glyphdata {
    SplineFont *sf;
    BlueData bd;
    SplineChar *sc;
    int fudge;
    int ccnt;
    int *contourends;
    int pcnt;				/* Includes control points, excludes implied points */
    struct pointdata *points;		/* Entries corresponding to control points are empty */
    int stemcnt;
    struct stemdata *stems;
    int linecnt;
    struct linedata *lines;

    /* Temporary values, quickly freed */
    int mcnt;
    struct monotonic *ms;
    struct monotonic **space;
    int scnt;
    Spline **sspace;
    struct st *stspace;
    DBounds size;
    struct pointdata **pspace;
};

struct pointdata {
    SplinePoint *sp;
    BasePoint nextunit, prevunit;	/* unit vectors pointing in the next/prev directions */
    struct linedata *nextline, *prevline;	/* any other points lying on approximately the same line */
    Spline *nextedge, *prevedge;		/* There should always be a matching spline, which may end up as part of a stem, and may not */
    double next_e_t, prev_e_t;			/* Location on other edge where our normal hits it */
    struct stemdata *nextstem, *prevstem;
    double nextlen, prevlen;
    unsigned int nextlinear: 1;
    unsigned int nextzero: 1;
    unsigned int prevlinear: 1;
    unsigned int prevzero: 1;
    unsigned int colinear: 1;
    unsigned int next_hor: 1;
    unsigned int next_ver: 1;
    unsigned int prev_hor: 1;
    unsigned int prev_ver: 1;
};

struct linedata {
    BasePoint unitvector;
    BasePoint online;
    int pcnt;
    struct pointdata **points;
};

struct stemdata {
    BasePoint unit;		/* Unit vector pointing in direction of stem */
    BasePoint left;		/* a point on one side of the stem (not necissarily left, even for vertical stems) */
    BasePoint right;		/* and one on the other */
    double width;
    int chunk_cnt;		/* number of separate point-pairs on this stem */
    struct stem_chunk {
	struct pointdata *l;
	struct pointdata *r;
	struct pointdata *lpotential, *rpotential;
	uint8 lnext, rnext;	/* are we using the next/prev side of the left/right points */
	uint8 ltick, rtick;
    } *chunks;
    uint8 toobig;		/* Stem is fatter than tall, unlikely to be a real stem */
};

extern struct glyphdata *GlyphDataBuild(SplineChar *sc, int only_hv);
extern void GlyphDataFree(struct glyphdata *gd);

#endif		/* _STEMDB_H_ */
